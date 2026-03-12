#include "actions.h"
#include "vars.h"
#include "DisplayManager.h"
#include <lvgl.h>

// Define your custom EEZ Flow Actions in here.
// These functions correspond to the "Call C function" user actions you create 
// in the EEZ Studio Flow editor.

// Native variables
static int32_t test_value = 50; // Match default backlight percent

int32_t get_var_test_value() {
    return test_value;
}

void set_var_test_value(int32_t value) {
    if (test_value == value) return;
    test_value = value;
    DisplayManager::setBacklightPercent((uint8_t)value);
}

// Stores the dropdown index (0=0°, 1=90°, 2=180°, 3=270°)
static int32_t screen_rotation_idx = 0;

int32_t get_var_screen_rotation() {
    return screen_rotation_idx;
}

void set_var_screen_rotation(int32_t value) {
    int32_t idx;

    // Accept both dropdown indices (0..3) and degree values (0/90/180/270).
    if (value == 90) {
        idx = 1;
    } else if (value == 180) {
        idx = 2;
    } else if (value == 270) {
        idx = 3;
    } else {
        idx = (value >= 0 && value <= 3) ? value : 0;
    }

    if (screen_rotation_idx == idx) return;
    screen_rotation_idx = idx;

    static const int32_t degrees[] = {0, 90, 180, 270};
    DisplayManager::setRotationDegrees(degrees[idx]);
}

static bool show_stats_val = true;

bool get_var_show_stats() {
    return show_stats_val;
}

void set_var_show_stats(bool value) {
    if (show_stats_val == value) return;
    show_stats_val = value;
    
    // We toggle visibility via the sys layer children.
    // Calling lv_sysmon_show_performance() repeatedly creates memory leaks and duplicate timers!
    lv_display_t * disp = lv_display_get_default();
    if(disp) {
        lv_obj_t * sys_layer = lv_display_get_layer_sys(disp);
        if(sys_layer) {
            uint32_t count = lv_obj_get_child_count(sys_layer);
            for(uint32_t i = 0; i < count; i++) {
                lv_obj_t * child = lv_obj_get_child(sys_layer, i);
                if(child) {
                    if (value) {
                        lv_obj_remove_flag(child, LV_OBJ_FLAG_HIDDEN);
                    } else {
                        lv_obj_add_flag(child, LV_OBJ_FLAG_HIDDEN);
                    }
                }
            }
        }
    }
}
