#include "actions.h"
#include "vars.h"
#include "DisplayManager.h"

// Define your custom EEZ Flow Actions in here.
// These functions correspond to the "Call C function" user actions you create 
// in the EEZ Studio Flow editor.

// Native variables
static int32_t test_value = 50; // Match default backlight percent

int32_t get_var_test_value() {
    return test_value;
}

void set_var_test_value(int32_t value) {
    if (test_value == value) return; // Prevent variable loopbacks causing infinite rendering redraws
    test_value = value;
    DisplayManager::setBacklightPercent((uint8_t)value);
}

static int32_t screen_rotation_val = 0;

int32_t get_var_screen_rotation() {
    return screen_rotation_val;
}

void set_var_screen_rotation(int32_t value) {
    if (screen_rotation_val == value) return; // Prevent loopbacks!
    screen_rotation_val = value;

    int32_t degrees = value;
    // Handle both exact degrees (0, 90, 180, 270) and logical indices (0, 1, 2, 3) 
    // that UI dropdowns/sliders might send natively.
    if (value == 1) degrees = 90;
    else if (value == 2) degrees = 180;
    else if (value == 3) degrees = 270;

    DisplayManager::setRotationDegrees(degrees);
}
