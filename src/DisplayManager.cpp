#include "DisplayManager.h"

#include "AXS15231B_Display.h"
#include "AXS15231B_Touch_IDF.h"
#include "AppConfig.h"
#include "ui.h"

#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

namespace {

AXS15231B_Display *gfx = new AXS15231B_Display(TFT_WIDTH, TFT_HEIGHT);
AXS15231B_Touch_IDF touch(TOUCH_SCL, TOUCH_SDA, TOUCH_INT, TOUCH_ADDR, 0);

lv_display_t *main_display = nullptr;
uint32_t touch_enable_after_ms = 0;
uint8_t current_rotation = 0;
uint8_t current_backlight_percent = 50;

static uint16_t* full_fb = nullptr;

constexpr uint8_t BL_PWM_CHANNEL = 0;
constexpr uint32_t BL_PWM_FREQ_HZ = 5000;
constexpr uint8_t BL_PWM_RES_BITS = 8;
constexpr uint32_t BOOT_BLACKOUT_MS = 500;
constexpr uint32_t BOOT_SPLASH_MS = 2200;

uint8_t next_rotation(uint8_t rotation)
{
    return (uint8_t)((rotation + 1U) & 0x03U);
}

void apply_rotation(uint8_t rotation)
{
    current_rotation = rotation;

    // Let LVGL handle the pixel rotation in software dynamically
    lv_display_rotation_t lv_rot = LV_DISPLAY_ROTATION_0;
    if (rotation == 1) lv_rot = LV_DISPLAY_ROTATION_90;
    else if (rotation == 2) lv_rot = LV_DISPLAY_ROTATION_180;
    else if (rotation == 3) lv_rot = LV_DISPLAY_ROTATION_270;

    // LVGL 9 automatically translates touch coordinates to match display rotation.
    // We strictly feed physical coordinates into LVGL.
    lv_display_set_rotation(main_display, lv_rot);

    // Update active screen size and layout to match the new display resolution
    lv_obj_t *active_screen = lv_screen_active();
    if (active_screen != nullptr) {
        int32_t w = lv_display_get_horizontal_resolution(main_display);
        int32_t h = lv_display_get_vertical_resolution(main_display);

        lv_obj_set_size(active_screen, w, h);
        lv_obj_update_layout(active_screen);
        lv_obj_invalidate(active_screen);
    }
}

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    lv_display_rotation_t rot = lv_display_get_rotation(disp);

    int32_t vx1 = area->x1;
    int32_t vy1 = area->y1;
    int32_t vx2 = area->x2;
    int32_t vy2 = area->y2;

    int32_t vw = vx2 - vx1 + 1;
    int32_t vh = vy2 - vy1 + 1;

    if (full_fb == nullptr) {
        lv_display_flush_ready(disp);
        return;
    }

    uint16_t* src = (uint16_t*)px_map;

    if (rot == LV_DISPLAY_ROTATION_270) {
        // Outer loop: physical rows (maps to LVGL X)
        for (int32_t vx = 0; vx < vw; vx++) {
            int32_t py_y = vx1 + vx;
            if (py_y < 0 || py_y >= 480) {
                // Out of range!!
                continue;
            }
            uint16_t* dest_row = &full_fb[py_y * 320];
            
            // Inner loop: physical cols (maps to LVGL Y reversed)
            // Writing sequentially backwards maximizes PSRAM burst cache hits
            for (int32_t vy = 0; vy < vh; vy++) {
                int32_t py_x = 319 - (vy1 + vy);
                if (py_x < 0 || py_x >= 320) {
                    continue; // Out of bounds
                }
                dest_row[py_x] = __builtin_bswap16(src[vy * vw + vx]);
            }
        }
    } else if (rot == LV_DISPLAY_ROTATION_90) {
        for (int32_t vx = 0; vx < vw; vx++) {
            int32_t py_y = 479 - (vx1 + vx);
            if (py_y < 0 || py_y >= 480) continue;
            uint16_t* dest_row = &full_fb[py_y * 320];

            for (int32_t vy = 0; vy < vh; vy++) {
                int32_t py_x = vy1 + vy;
                if (py_x < 0 || py_x >= 320) continue;
                dest_row[py_x] = __builtin_bswap16(src[vy * vw + vx]);
            }
        }
    } else if (rot == LV_DISPLAY_ROTATION_180) {
        for (int32_t vy = 0; vy < vh; vy++) {
            int32_t py_y = 479 - (vy1 + vy);
            if(py_y < 0 || py_y >= 480) continue;
            for (int32_t vx = 0; vx < vw; vx++) {
                int32_t py_x = 319 - (vx1 + vx);
                if(py_x < 0 || py_x >= 320) continue;
                full_fb[py_y * 320 + py_x] = __builtin_bswap16(src[vy * vw + vx]);
            }
        }
    } else {
        // ROTATION_0
        for (int32_t vy = 0; vy < vh; vy++) {
            int32_t py_y = vy1 + vy;
            if(py_y < 0 || py_y >= 480) continue;
            for (int32_t vx = 0; vx < vw; vx++) {
                int32_t py_x = vx1 + vx;
                if(py_x < 0 || py_x >= 320) continue;
                full_fb[py_y * 320 + py_x] = __builtin_bswap16(src[vy * vw + vx]);
            }
        }
    }

    if (lv_display_flush_is_last(disp)) {
        if (gfx != nullptr) {
            gfx->drawBitmapQSPI(0, 0, 320, 480, full_fb);
        } else {
            ESP_LOGE("Disp", "gfx is NULL!");
        }
    }

    lv_display_flush_ready(disp);
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;

    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
    if(now_ms < touch_enable_after_ms) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    if(!touch.touched()) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    uint16_t x = 0;
    uint16_t y = 0;
    touch.readData(&x, &y);

    int32_t mapped_x = (int32_t)x;
    int32_t mapped_y = (int32_t)y;

    int32_t max_x = TFT_WIDTH - 1;
    int32_t max_y = TFT_HEIGHT - 1;

    if(mapped_x < 0) mapped_x = 0;
    if(mapped_y < 0) mapped_y = 0;
    if(mapped_x > max_x) mapped_x = max_x;
    if(mapped_y > max_y) mapped_y = max_y;

    data->point.x = mapped_x;
    data->point.y = mapped_y;
    data->state = LV_INDEV_STATE_PRESSED;
}

}

namespace DisplayManager {

bool begin()
{
    ledc_timer_config_t ledc_timer = {};
    ledc_timer.speed_mode       = LEDC_LOW_SPEED_MODE;
    ledc_timer.duty_resolution  = LEDC_TIMER_8_BIT;
    ledc_timer.timer_num        = LEDC_TIMER_0;
    ledc_timer.freq_hz          = BL_PWM_FREQ_HZ;
    ledc_timer.clk_cfg          = LEDC_AUTO_CLK;
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {};
    ledc_channel.gpio_num       = TFT_BL;
    ledc_channel.speed_mode     = LEDC_LOW_SPEED_MODE;
    ledc_channel.channel        = (ledc_channel_t)BL_PWM_CHANNEL;
    ledc_channel.intr_type      = LEDC_INTR_DISABLE;
    ledc_channel.timer_sel      = LEDC_TIMER_0;
    ledc_channel.duty           = 0;
    ledc_channel.hpoint         = 0;
    ledc_channel_config(&ledc_channel);

    if(!gfx->init(TFT_QSPI_CS, TFT_QSPI_SCK, TFT_QSPI_D0, TFT_QSPI_D1, TFT_QSPI_D2, TFT_QSPI_D3, -1, 40000000)) {
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(BOOT_BLACKOUT_MS));

    setBacklightPercent(40);
    vTaskDelay(pdMS_TO_TICKS(120));

    setBacklightPercent(50);

    touch.begin();

    lv_init();

    main_display = lv_display_create(TFT_WIDTH, TFT_HEIGHT);
    lv_display_set_default(main_display);
    lv_display_set_color_format(main_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(main_display, my_disp_flush);
    
    // Always use internal SRAM for display buffers.
    // PSRAM is too slow for the heavy pixel blending operations during transitions.
    // Allocate a full rotation framebuffer in PSRAM
    full_fb = (uint16_t*)heap_caps_aligned_alloc(64, TFT_WIDTH * TFT_HEIGHT * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (!full_fb) {
        printf("FATAL: Cannot allocate PSRAM for full_fb!\n");
    } else {
        memset(full_fb, 0, TFT_WIDTH * TFT_HEIGHT * sizeof(uint16_t));
    }

    // Try huge 1/4 screen chunks first for max framerate
    size_t actual_buffer_size = (TFT_WIDTH * TFT_HEIGHT / 4) * sizeof(uint16_t);
    void *buf1 = heap_caps_malloc(actual_buffer_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    void *buf2 = heap_caps_malloc(actual_buffer_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    // Fallback to 1/8 if RAM is fragmented
    if (buf1 == nullptr || buf2 == nullptr) {
        if (buf1) heap_caps_free(buf1);
        if (buf2) heap_caps_free(buf2);
        actual_buffer_size = (TFT_WIDTH * TFT_HEIGHT / 8) * sizeof(uint16_t);
        buf1 = heap_caps_malloc(actual_buffer_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        buf2 = heap_caps_malloc(actual_buffer_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    }
    
    // Final emergency fallback to 1/15
    if (buf1 == nullptr || buf2 == nullptr) {
        if (buf1) heap_caps_free(buf1);
        if (buf2) heap_caps_free(buf2);
        actual_buffer_size = (TFT_WIDTH * TFT_HEIGHT / 15) * sizeof(uint16_t);
        buf1 = heap_caps_malloc(actual_buffer_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        buf2 = heap_caps_malloc(actual_buffer_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    }
    
    lv_display_set_buffers(main_display, buf1, buf2, actual_buffer_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);

    apply_rotation(3); // 270 degrees right (or 90 degrees left)

    touch_enable_after_ms = (uint32_t)(esp_timer_get_time() / 1000) + 1200;

    return true;
}

void process()
{
    static uint32_t last_tick = 0;
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);

    uint32_t elapsed = now - last_tick;
    if(elapsed > 0) {
        lv_tick_inc(elapsed);
        last_tick = now;
    }

    uint32_t wait_ms = lv_timer_handler();

    // FreeRTOS tasks (like LVGL background threads) need idle time to flush.   
    // However, sleeping too long caps max framerate.
    // wait_ms tells us exactly how long LVGL wants us to wait.
    if (wait_ms == 0) wait_ms = 1;
    if (wait_ms > 5) wait_ms = 5;

    vTaskDelay(pdMS_TO_TICKS(wait_ms));
}

void rotateNext()
{
    apply_rotation(next_rotation(current_rotation));
}

void setRotationDegrees(int degrees)
{
    // Sanitize input degrees to 0, 1, 2, or 3 mappings
    degrees = degrees % 360;
    if (degrees < 0) degrees += 360;
    
    uint8_t rotation = 0;
    if (degrees == 90) rotation = 1;
    else if (degrees == 180) rotation = 2;
    else if (degrees == 270) rotation = 3;
    
    // Ignore if already set to avoid continuous redraws when triggered by UI refresh loops
    if (current_rotation == rotation && main_display != nullptr) {
        return;
    }

    apply_rotation(rotation);
}

int rotationDegrees()
{
    return (int)current_rotation * 90;
}

lv_display_t *display()
{
    return main_display;
}

void setBacklightPercent(uint8_t percent)
{
    if(percent < 1U) {
        percent = 1U;
    }
    if(percent > 100U) {
        percent = 100U;
    }

    if (current_backlight_percent == percent) return;

    current_backlight_percent = percent;

    uint32_t duty = (255U * current_backlight_percent) / 100U;
    if(duty == 0U) {
        duty = 1U;
    }

    ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)BL_PWM_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)BL_PWM_CHANNEL);
}

uint8_t backlightPercent()
{
    return current_backlight_percent;
}

}