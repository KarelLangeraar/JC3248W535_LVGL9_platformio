#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <src/draw/sw/lv_draw_sw.h>
#include "AXS15231B_touch.h"
#include "AXS15231B_JC3248.h"

/* Pincode configuratie voor JC3248W535 */
#define TFT_BL 1
#define TFT_QSPI_CS 45
#define TFT_QSPI_SCK 47
#define TFT_QSPI_D0 21
#define TFT_QSPI_D1 48
#define TFT_QSPI_D2 40
#define TFT_QSPI_D3 39

/* Touch pinnen */
#define TOUCH_SCL 8
#define TOUCH_SDA 4
#define TOUCH_INT 3
#define TOUCH_ADDR 0x3B
#define ENABLE_TOUCH 1

#define TFT_WIDTH 320
#define TFT_HEIGHT 480

enum class RotationBackend : uint8_t {
    CanvasPanel,
    LvglSoftware
};

static RotationBackend active_backend = RotationBackend::CanvasPanel;

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    TFT_QSPI_CS, TFT_QSPI_SCK, TFT_QSPI_D0, TFT_QSPI_D1, TFT_QSPI_D2, TFT_QSPI_D3);
Arduino_AXS15231B_JC3248 *panel = new Arduino_AXS15231B_JC3248(bus, GFX_NOT_DEFINED, 0, false, TFT_WIDTH, TFT_HEIGHT);
Arduino_Canvas *canvas = nullptr;
Arduino_GFX *gfx = nullptr;

AXS15231B_Touch touch(TOUCH_SCL, TOUCH_SDA, TOUCH_INT, TOUCH_ADDR, 0);

static int counter = 0;
static lv_obj_t *label_count = nullptr;
static lv_obj_t *label_rotation = nullptr;
static lv_obj_t *label_backend = nullptr;
static lv_display_t *main_display = nullptr;
static uint32_t touch_enable_after_ms = 0;
static lv_display_rotation_t current_rotation = LV_DISPLAY_ROTATION_0;

alignas(4) static uint16_t lv_partial_buf[TFT_WIDTH * 40];
alignas(4) static uint16_t lv_rotate_buf[TFT_WIDTH * 40];

static const char *backend_name()
{
    return active_backend == RotationBackend::CanvasPanel ? "Canvas+Panel" : "LVGL-SW";
}

static uint8_t panel_rotation_from_lv(lv_display_rotation_t rotation)
{
    switch(rotation) {
        case LV_DISPLAY_ROTATION_0:
            return 0;
        case LV_DISPLAY_ROTATION_90:
            return 1;
        case LV_DISPLAY_ROTATION_180:
            return 2;
        case LV_DISPLAY_ROTATION_270:
            return 3;
    }

    return 0;
}

static lv_display_rotation_t next_rotation(lv_display_rotation_t rotation)
{
    switch(rotation) {
        case LV_DISPLAY_ROTATION_0:
            return LV_DISPLAY_ROTATION_90;
        case LV_DISPLAY_ROTATION_90:
            return LV_DISPLAY_ROTATION_180;
        case LV_DISPLAY_ROTATION_180:
            return LV_DISPLAY_ROTATION_270;
        case LV_DISPLAY_ROTATION_270:
            return LV_DISPLAY_ROTATION_0;
    }

    return LV_DISPLAY_ROTATION_0;
}

static lv_display_rotation_t inverse_rotation(lv_display_rotation_t rotation)
{
    switch(rotation) {
        case LV_DISPLAY_ROTATION_0:
            return LV_DISPLAY_ROTATION_0;
        case LV_DISPLAY_ROTATION_90:
            return LV_DISPLAY_ROTATION_270;
        case LV_DISPLAY_ROTATION_180:
            return LV_DISPLAY_ROTATION_180;
        case LV_DISPLAY_ROTATION_270:
            return LV_DISPLAY_ROTATION_90;
    }

    return LV_DISPLAY_ROTATION_0;
}

static void update_rotation_label()
{
    if(label_rotation == nullptr) {
        return;
    }

    int degree = 0;
    switch(current_rotation) {
        case LV_DISPLAY_ROTATION_0:
            degree = 0;
            break;
        case LV_DISPLAY_ROTATION_90:
            degree = 90;
            break;
        case LV_DISPLAY_ROTATION_180:
            degree = 180;
            break;
        case LV_DISPLAY_ROTATION_270:
            degree = 270;
            break;
    }

    lv_label_set_text_fmt(label_rotation, "Rotation: %d deg", degree);
}

static void apply_rotation(lv_display_rotation_t rotation)
{
    current_rotation = rotation;

    if(active_backend == RotationBackend::CanvasPanel) {
        lv_display_set_rotation(main_display, inverse_rotation(current_rotation));
        panel->setRotation(0);
        gfx->setRotation(panel_rotation_from_lv(current_rotation));
        touch.setRotation(0);
    } else {
        gfx->setRotation(0);
        panel->setRotation(0);
        lv_display_set_rotation(main_display, current_rotation);
    }

    update_rotation_label();
    lv_obj_update_layout(lv_screen_active());
    lv_obj_invalidate(lv_screen_active());
}

static void btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        counter++;
        lv_label_set_text_fmt(label_count, "Clicks: %d", counter);
    }
}

static void rotate_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        apply_rotation(next_rotation(current_rotation));
    }
}

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    lv_area_t draw_area = *area;
    uint8_t *draw_px = px_map;

    int32_t draw_w = lv_area_get_width(area);
    int32_t draw_h = lv_area_get_height(area);

    if(active_backend == RotationBackend::LvglSoftware) {
        lv_display_rotation_t rotation = lv_display_get_rotation(disp);
        if(rotation != LV_DISPLAY_ROTATION_0) {
            uint32_t src_stride = lv_draw_buf_width_to_stride(draw_w, LV_COLOR_FORMAT_RGB565);
            uint32_t dst_stride = (rotation == LV_DISPLAY_ROTATION_180)
                                      ? src_stride
                                      : lv_draw_buf_width_to_stride(draw_h, LV_COLOR_FORMAT_RGB565);

            lv_draw_sw_rotate(px_map, lv_rotate_buf, draw_w, draw_h, src_stride, dst_stride, rotation, LV_COLOR_FORMAT_RGB565);
            draw_px = reinterpret_cast<uint8_t *>(lv_rotate_buf);

            lv_display_rotate_area(disp, &draw_area);
            if(rotation != LV_DISPLAY_ROTATION_180) {
                int32_t tmp = draw_w;
                draw_w = draw_h;
                draw_h = tmp;
            }
        }
    }

    int32_t target_w = (int32_t)gfx->width();
    int32_t target_h = (int32_t)gfx->height();

    if(draw_area.x2 < 0 || draw_area.y2 < 0 || draw_area.x1 > (target_w - 1) || draw_area.y1 > (target_h - 1)) {
        lv_display_flush_ready(disp);
        return;
    }

    gfx->draw16bitRGBBitmap((int16_t)draw_area.x1, (int16_t)draw_area.y1,
                            reinterpret_cast<uint16_t *>(draw_px),
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

    uint16_t x = 0;
    uint16_t y = 0;

    if(touch.touched()) {
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
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void setup()
{
    Serial.begin(115200);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    canvas = new Arduino_Canvas(TFT_WIDTH, TFT_HEIGHT, panel);
    if(canvas == nullptr) {
        active_backend = RotationBackend::LvglSoftware;
        gfx = panel;
    } else {
        gfx = canvas;
    }

    if(!gfx->begin()) {
        if(gfx != panel) {
            active_backend = RotationBackend::LvglSoftware;
            gfx = panel;
            if(!gfx->begin()) {
                Serial.println("Display init failed");
                while(true) {
                    delay(1000);
                }
            }
        } else {
            Serial.println("Display init failed");
            while(true) {
                delay(1000);
            }
        }
    }

    gfx->fillScreen(BLACK);

#if ENABLE_TOUCH
    if(!touch.begin()) {
        Serial.println("Touch init failed");
    }
#endif

    lv_init();

    main_display = lv_display_create(TFT_WIDTH, TFT_HEIGHT);
    lv_display_set_default(main_display);
    lv_display_set_color_format(main_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(main_display, my_disp_flush);
    lv_display_set_buffers(main_display, lv_partial_buf, NULL, sizeof(lv_partial_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

#if ENABLE_TOUCH
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);
#endif

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x202020), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_screen_load(scr);

    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_set_size(btn, 140, 60);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, -60);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "LVGL BUTTON");
    lv_obj_center(label);

    lv_obj_t *rotate_btn = lv_button_create(scr);
    lv_obj_set_size(rotate_btn, 140, 52);
    lv_obj_align(rotate_btn, LV_ALIGN_CENTER, 0, 20);
    lv_obj_add_event_cb(rotate_btn, rotate_btn_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *rotate_label = lv_label_create(rotate_btn);
    lv_label_set_text(rotate_label, "Rotate");
    lv_obj_center(rotate_label);

    label_count = lv_label_create(scr);
    lv_label_set_text(label_count, "Clicks: 0");
    lv_obj_align(label_count, LV_ALIGN_CENTER, 0, 90);

    label_rotation = lv_label_create(scr);
    lv_obj_align(label_rotation, LV_ALIGN_TOP_MID, 0, 10);

    label_backend = lv_label_create(scr);
    lv_label_set_text_fmt(label_backend, "Backend: %s", backend_name());
    lv_obj_align(label_backend, LV_ALIGN_TOP_MID, 0, 32);

    apply_rotation(LV_DISPLAY_ROTATION_0);

    touch_enable_after_ms = millis() + 1200;
    Serial.printf("Display backend: %s\n", backend_name());
}

void loop()
{
    static uint32_t last_tick = 0;
    uint32_t now = millis();

    uint32_t elapsed = now - last_tick;
    if(elapsed > 0) {
        lv_tick_inc(elapsed);
        last_tick = now;
    }

    lv_timer_handler();

    if(active_backend == RotationBackend::CanvasPanel && canvas != nullptr) {
        canvas->flush();
    }

    delay(5);
}
