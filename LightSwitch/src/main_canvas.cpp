#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include "AXS15231B_touch.h"

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

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    TFT_QSPI_CS, TFT_QSPI_SCK, TFT_QSPI_D0, TFT_QSPI_D1, TFT_QSPI_D2, TFT_QSPI_D3);

// Use the specific init operations if available. 
// If not available, I will need to define it. 
// For now, I will assume I need to find it.

Arduino_GFX *g = new Arduino_AXS15231B(bus, GFX_NOT_DEFINED, 0, false, 320, 480);
Arduino_Canvas *gfx = new Arduino_Canvas(320, 480, g);

AXS15231B_Touch touch(TOUCH_SCL, TOUCH_SDA, TOUCH_INT, TOUCH_ADDR, 0);

static int counter = 0;
static lv_obj_t *label_count;

static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        counter++;
        lv_label_set_text_fmt(label_count, "Clicks: %d", counter);
    }
}

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    int32_t x1 = area->x1;
    int32_t y1 = area->y1;
    int32_t x2 = area->x2;
    int32_t y2 = area->y2;

    if (x2 < 0 || y2 < 0 || x1 > 319 || y1 > 479) {
        lv_display_flush_ready(disp);
        return;
    }

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > 319) x2 = 319;
    if (y2 > 479) y2 = 479;

    uint32_t w = (uint32_t)(x2 - x1 + 1);
    uint32_t h = (uint32_t)(y2 - y1 + 1);
    gfx->draw16bitRGBBitmap(x1, y1, (uint16_t *)px_map, w, h);
    lv_display_flush_ready(disp);
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    /* Temporarily disabled touch */
    data->state = LV_INDEV_STATE_RELEASED;
    /*
    uint16_t x, y;
    if (touch.touched()) {
        // ...
    }
    */
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting JC3248W535 test");

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    if (!gfx->begin()) {
        Serial.println("gfx->begin() failed!");
    }
    
    // Explicitly try to handle inversion if needed
    // g->invertDisplay(true); // or false

    gfx->fillScreen(RED);
    delay(1000);
    gfx->fillScreen(GREEN);
    delay(1000);
    gfx->fillScreen(BLUE);
    delay(1000);

    lv_init();
    // ... rest of lvgl init
}

void loop() {
    lv_task_handler();
    gfx->flush(); // Needed for Canvas
    delay(5);
}
