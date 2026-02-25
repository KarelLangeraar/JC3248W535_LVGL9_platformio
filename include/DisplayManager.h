#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino_GFX_Library.h>
#include <lvgl.h>

namespace DisplayManager {

bool begin();      // Initialize display, LVGL, and touch input
void process();    // Call in main loop for LVGL and display updates
void rotateNext(); // Cycle to next rotation state (0° → 90° → 180° → 270°)
int rotationDegrees(); // Get current rotation in degrees (0, 90, 180, 270)
lv_display_t *display(); // Get LVGL display handle

}

#endif