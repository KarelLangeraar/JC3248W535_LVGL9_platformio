#include "SplashScreen.h"

#include <Arduino.h>

namespace {

void flush_if_canvas(Arduino_Canvas *canvas)
{
    if(canvas != nullptr) {
        canvas->flush();
    }
}

void draw_face(Arduino_GFX *gfx, bool blink)
{
    int16_t w = (int16_t)gfx->width();
    int16_t h = (int16_t)gfx->height();

    int16_t cx = (int16_t)(w / 2);
    int16_t cy = (int16_t)(h / 2 - 18);
    int16_t r = (int16_t)((w < h ? w : h) / 6);

    gfx->fillScreen(BLACK);

    gfx->fillCircle(cx, cy, r, YELLOW);
    gfx->drawCircle(cx, cy, r, WHITE);

    int16_t eye_x_off = (int16_t)(r / 3);
    int16_t eye_y = (int16_t)(cy - r / 4);
    int16_t eye_r = (int16_t)(r / 8 + 2);

    if(blink) {
        int16_t eye_w = (int16_t)(eye_r * 2 + 4);
        gfx->drawFastHLine((int16_t)(cx - eye_x_off - eye_w / 2), eye_y, eye_w, BLACK);
        gfx->drawFastHLine((int16_t)(cx + eye_x_off - eye_w / 2), eye_y, eye_w, BLACK);
    } else {
        gfx->fillCircle((int16_t)(cx - eye_x_off), eye_y, eye_r, BLACK);
        gfx->fillCircle((int16_t)(cx + eye_x_off), eye_y, eye_r, BLACK);
    }

    int16_t mouth_cy = (int16_t)(cy + r / 4);
    int16_t mouth_r = (int16_t)(r / 2);
    gfx->drawCircle(cx, mouth_cy, mouth_r, BLACK);
    gfx->drawCircle(cx, (int16_t)(mouth_cy + 1), mouth_r, BLACK);
    gfx->fillRect((int16_t)(cx - mouth_r - 2), (int16_t)(mouth_cy - mouth_r - 2),
                  (int16_t)(2 * mouth_r + 4), (int16_t)(mouth_r + 3), YELLOW);

}

}

namespace SplashScreen {

void drawBootFrame(Arduino_GFX *gfx, Arduino_Canvas *canvas, bool blink)
{
    if(gfx == nullptr) {
        return;
    }

    draw_face(gfx, blink);
    flush_if_canvas(canvas);
}

void showBoot(Arduino_GFX *gfx, Arduino_Canvas *canvas, uint32_t durationMs)
{
    if(gfx == nullptr) {
        return;
    }

    uint32_t start = millis();

    while((millis() - start) < durationMs) {
        uint32_t elapsed = millis() - start;
        bool blink = false;

        if((elapsed > 1200U && elapsed < 1320U) ||
           (elapsed > 1440U && elapsed < 1520U)) {
            blink = true;
        }

        drawBootFrame(gfx, canvas, blink);
        delay(45);
    }
}

}