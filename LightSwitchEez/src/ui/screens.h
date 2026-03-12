#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN = 1,
    SCREEN_ID_TEST = 2,
    _SCREEN_ID_LAST = 2
};

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *test;
    lv_obj_t *background;
    lv_obj_t *obj0;
    lv_obj_t *starcontainer;
    lv_obj_t *shooting_star;
    lv_obj_t *star;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *obj3;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
    lv_obj_t *button_down;
    lv_obj_t *test_slider;
    lv_obj_t *button_up;
    lv_obj_t *show_stats_check;
    lv_obj_t *obj6;
    lv_obj_t *obj7;
    lv_obj_t *obj8;
} objects_t;

extern objects_t objects;

void create_screen_main();
void tick_screen_main();

void create_screen_test();
void tick_screen_test();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/