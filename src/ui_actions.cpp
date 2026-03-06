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
    test_value = value;
    DisplayManager::setBacklightPercent((uint8_t)value);
}
