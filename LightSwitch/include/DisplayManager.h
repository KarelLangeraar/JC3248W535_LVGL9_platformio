#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino_GFX_Library.h>
#include <lvgl.h>

namespace DisplayManager {

bool begin();
void process();
void rotateNext();
int rotationDegrees();
lv_display_t *display();

}

#endif