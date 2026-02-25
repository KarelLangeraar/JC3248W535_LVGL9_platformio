#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <esp_heap_caps.h>
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
#define TOUCH_ADDR 0x3B // Of 0x24 checken
#define ENABLE_TOUCH 1
#define TFT_WIDTH 320
#define TFT_HEIGHT 480

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    TFT_QSPI_CS, TFT_QSPI_SCK, TFT_QSPI_D0, TFT_QSPI_D1, TFT_QSPI_D2, TFT_QSPI_D3);
Arduino_AXS15231B_JC3248 *panel = new Arduino_AXS15231B_JC3248(bus, GFX_NOT_DEFINED, 0, false, 320, 480);
Arduino_GFX *gfx = panel;
AXS15231B_Touch touch(TOUCH_SCL, TOUCH_SDA, TOUCH_INT, TOUCH_ADDR, 0);

static int counter = 0;
static lv_obj_t *label_count;
static lv_display_t *main_display = nullptr;
static uint32_t touch_enable_after_ms = 0;
alignas(4) static uint16_t lv_partial_buf[TFT_WIDTH * 40];
static uint16_t *lv_full_buf = nullptr;

static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        counter++;
        lv_label_set_text_fmt(label_count, "Clicks: %d", counter);
    }
}

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    static bool first_flush_logged = false;
    static uint32_t flush_count = 0;
    int32_t x1 = area->x1;
    int32_t y1 = area->y1;
    int32_t x2 = area->x2;
    int32_t y2 = area->y2;

    if (x2 < 0 || y2 < 0 || x1 > (TFT_WIDTH - 1) || y1 > (TFT_HEIGHT - 1)) {
        lv_display_flush_ready(disp);
        return;
    }

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > (TFT_WIDTH - 1)) x2 = (TFT_WIDTH - 1);
    if (y2 > (TFT_HEIGHT - 1)) y2 = (TFT_HEIGHT - 1);

    uint32_t w = (uint32_t)(x2 - x1 + 1);
    uint32_t h = (uint32_t)(y2 - y1 + 1);

    uint16_t *src = (uint16_t *)px_map;
    gfx->draw16bitRGBBitmap((int16_t)x1, (int16_t)y1, src, (int16_t)w, (int16_t)h);

    flush_count++;
    if (flush_count <= 4) {
        Serial.printf("flush[%lu]: (%ld,%ld)-(%ld,%ld)\n",
                      (unsigned long)flush_count, (long)x1, (long)y1, (long)x2, (long)y2);
    }

    if (!first_flush_logged) {
        first_flush_logged = true;
        Serial.printf("LVGL first flush: area=(%ld,%ld)-(%ld,%ld)\n", (long)area->x1, (long)area->y1, (long)area->x2, (long)area->y2);
    }

    lv_display_flush_ready(disp);
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    (void)indev;
    if (millis() < touch_enable_after_ms) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    uint16_t x, y;
    if (touch.touched()) {
        touch.readData(&x, &y);
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
        // Serial.printf("Touch: %u, %u\n", x, y);
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Boot: setup start");
    // Backlight aanzetten
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    
    gfx->begin();
    gfx->setRotation(0);
    touch.setRotation(gfx->getRotation());
    Serial.printf("Display geometry: %d x %d, rotation=%u\n", gfx->width(), gfx->height(), gfx->getRotation());
    
    // Explicit Color Cycle Test - 1 Second Each
    gfx->fillScreen(RED);
    delay(1000);
    gfx->fillScreen(GREEN);
    delay(1000);
    gfx->fillScreen(BLUE);
    delay(1000);
    Serial.println("Boot: RGB test complete");
    gfx->fillScreen(BLACK);

    // Continue to LVGL
#if ENABLE_TOUCH
    if(!touch.begin()) {
        Serial.println("Touch init failed!");
    } else {
        Serial.println("Touch init OK");
    }
    Serial.println("Boot: touch init done");
#else
    Serial.println("Boot: touch disabled for display isolation test");
#endif
    
    lv_init();
    Serial.println("Boot: lv_init done");
    
    // LVGL Display buffer & driver
    Serial.println("LVGL: creating display");
    main_display = lv_display_create(TFT_WIDTH, TFT_HEIGHT);
    Serial.println("LVGL: set default");
    lv_display_set_default(main_display);
    Serial.println("LVGL: set color format");
    lv_display_set_color_format(main_display, LV_COLOR_FORMAT_RGB565);
    Serial.println("LVGL: set flush cb");
    lv_display_set_flush_cb(main_display, my_disp_flush);
    Serial.println("LVGL: set buffers");
    size_t full_buf_bytes = (size_t)TFT_WIDTH * (size_t)TFT_HEIGHT * sizeof(uint16_t);
    lv_full_buf = (uint16_t *)heap_caps_malloc(full_buf_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (lv_full_buf != nullptr) {
        lv_display_set_buffers(main_display, lv_full_buf, NULL, full_buf_bytes, LV_DISPLAY_RENDER_MODE_FULL);
        Serial.println("LVGL: full buffer mode (PSRAM)");
    } else {
        lv_display_set_buffers(main_display, lv_partial_buf, NULL, sizeof(lv_partial_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
        Serial.println("LVGL: partial buffer mode (fallback)");
    }
    
    // Input device driver
#if ENABLE_TOUCH
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);
#endif
    
    // UI Creation - Explicit Colors
    Serial.println("LVGL: create screen");
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x202020), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);       // Disable scrolling
    lv_screen_load(scr);
    Serial.println("LVGL: screen loaded");

    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, -50);
    lv_obj_set_size(btn, 140, 60);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xFFFF00), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "LVGL BUTTON");
    lv_obj_set_style_text_color(label, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_center(label);
    
    label_count = lv_label_create(scr);
    lv_label_set_text(label_count, "Clicks: 0");
    lv_obj_set_style_text_color(label_count, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(label_count, LV_ALIGN_CENTER, 0, 50);

    lv_obj_invalidate(scr);
    lv_refr_now(main_display);
    for (int i = 0; i < 10; i++) {
        lv_tick_inc(5);
        lv_timer_handler();
        delay(5);
    }
    touch_enable_after_ms = millis() + 2000;
    Serial.println("Boot: UI setup done");
}

void loop() {
    static uint32_t last_heartbeat = 0;
    static uint32_t last_tick = 0;
    uint32_t now = millis();

    uint32_t elapsed = now - last_tick;
    if (elapsed > 0) {
        lv_tick_inc(elapsed);
        last_tick = now;
    }

    lv_timer_handler();

    if (now - last_heartbeat >= 5000) {
        last_heartbeat = now;
        Serial.println("Loop heartbeat");
    }

    delay(5);
}
