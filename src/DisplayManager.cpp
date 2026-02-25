#include "DisplayManager.h"

#include <Arduino.h>

#include "AXS15231B_JC3248.h"
#include "AXS15231B_touch.h"
#include "AppConfig.h"
#include "AppUI.h"
#include "SplashScreen.h"

namespace {

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    TFT_QSPI_CS, TFT_QSPI_SCK, TFT_QSPI_D0, TFT_QSPI_D1, TFT_QSPI_D2, TFT_QSPI_D3);
Arduino_AXS15231B_JC3248 *panel = new Arduino_AXS15231B_JC3248(bus, GFX_NOT_DEFINED, 0, false, TFT_WIDTH, TFT_HEIGHT);
Arduino_Canvas *canvas = nullptr;
Arduino_GFX *gfx = nullptr;

AXS15231B_Touch touch(TOUCH_SCL, TOUCH_SDA, TOUCH_INT, TOUCH_ADDR, 0);

lv_display_t *main_display = nullptr;
uint32_t touch_enable_after_ms = 0;
uint8_t current_rotation = 0;
uint8_t current_backlight_percent = 50;

constexpr uint8_t BL_PWM_CHANNEL = 0;
constexpr uint32_t BL_PWM_FREQ_HZ = 5000;
constexpr uint8_t BL_PWM_RES_BITS = 8;
constexpr uint32_t BOOT_BLACKOUT_MS = 500;
constexpr uint32_t BOOT_SPLASH_MS = 2200;

alignas(4) uint16_t lv_partial_buf[TFT_WIDTH * 40];

uint8_t next_rotation(uint8_t rotation)
{
    return (uint8_t)((rotation + 1U) & 0x03U);
}

void apply_rotation(uint8_t rotation)
{
    current_rotation = rotation;

    gfx->setRotation(current_rotation);
    touch.setRotation(current_rotation);

    lv_display_set_resolution(main_display, (int32_t)gfx->width(), (int32_t)gfx->height());
    lv_display_set_physical_resolution(main_display, (int32_t)gfx->width(), (int32_t)gfx->height());

    AppUI::setRotationLabel(DisplayManager::rotationDegrees());

    lv_obj_t *active = lv_screen_active();
    if(active != nullptr) {
        lv_obj_update_layout(active);
        lv_obj_invalidate(active);
    }
}

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    int32_t target_w = (int32_t)gfx->width();
    int32_t target_h = (int32_t)gfx->height();

    if(area->x2 < 0 || area->y2 < 0 || area->x1 > (target_w - 1) || area->y1 > (target_h - 1)) {
        lv_display_flush_ready(disp);
        return;
    }

    int32_t draw_w = lv_area_get_width(area);
    int32_t draw_h = lv_area_get_height(area);

    gfx->draw16bitRGBBitmap((int16_t)area->x1, (int16_t)area->y1,
                            reinterpret_cast<uint16_t *>(px_map),
                            (int16_t)draw_w, (int16_t)draw_h);

    lv_display_flush_ready(disp);
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;

    if(millis() < touch_enable_after_ms) {
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

    int32_t max_x = lv_display_get_horizontal_resolution(main_display) - 1;
    int32_t max_y = lv_display_get_vertical_resolution(main_display) - 1;

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
    ledcSetup(BL_PWM_CHANNEL, BL_PWM_FREQ_HZ, BL_PWM_RES_BITS);
    ledcAttachPin(TFT_BL, BL_PWM_CHANNEL);
    ledcWrite(BL_PWM_CHANNEL, 0);

    canvas = new Arduino_Canvas(TFT_WIDTH, TFT_HEIGHT, panel);
    gfx = (canvas != nullptr) ? static_cast<Arduino_GFX *>(canvas) : static_cast<Arduino_GFX *>(panel);

    if(!gfx->begin()) {
        if(gfx != panel) {
            gfx = panel;
            if(!gfx->begin()) {
                return false;
            }
        } else {
            return false;
        }
    }

    delay(BOOT_BLACKOUT_MS);

    gfx->fillScreen(BLACK);
    if(canvas != nullptr) {
        canvas->flush();
    }

    SplashScreen::drawBootFrame(gfx, canvas, false);
    setBacklightPercent(40);
    delay(120);

    SplashScreen::showBoot(gfx, canvas, BOOT_SPLASH_MS);
    setBacklightPercent(50);

    touch.begin();

    lv_init();

    main_display = lv_display_create(TFT_WIDTH, TFT_HEIGHT);
    lv_display_set_default(main_display);
    lv_display_set_color_format(main_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(main_display, my_disp_flush);
    lv_display_set_buffers(main_display, lv_partial_buf, NULL, sizeof(lv_partial_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);

    apply_rotation(0);
    touch_enable_after_ms = millis() + 1200;

    return true;
}

void process()
{
    static uint32_t last_tick = 0;
    uint32_t now = millis();

    uint32_t elapsed = now - last_tick;
    if(elapsed > 0) {
        lv_tick_inc(elapsed);
        last_tick = now;
    }

    lv_timer_handler();

    if(canvas != nullptr) {
        canvas->flush();
    }

    delay(5);
}

void rotateNext()
{
    apply_rotation(next_rotation(current_rotation));
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

    current_backlight_percent = percent;

    uint32_t duty = (255U * current_backlight_percent) / 100U;
    if(duty == 0U) {
        duty = 1U;
    }

    ledcWrite(BL_PWM_CHANNEL, duty);
}

uint8_t backlightPercent()
{
    return current_backlight_percent;
}

}