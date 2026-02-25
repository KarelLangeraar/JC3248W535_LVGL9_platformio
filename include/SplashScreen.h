#ifndef SPLASH_SCREEN_H
#define SPLASH_SCREEN_H

#include <Arduino_GFX_Library.h>

namespace SplashScreen {

void drawBootFrame(Arduino_GFX *gfx, Arduino_Canvas *canvas, bool blink);
void showBoot(Arduino_GFX *gfx, Arduino_Canvas *canvas, uint32_t durationMs);

}

#endif