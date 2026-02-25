#include "AppUI.h"

#include <lvgl.h>

#include "DisplayManager.h"

namespace {

lv_obj_t *label_rotation = nullptr;
lv_obj_t *container = nullptr;

void rotate_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        DisplayManager::rotateNext();
    }
}

void box_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_t *box = (lv_obj_t *)lv_event_get_target(e);
        lv_color_t current_color = lv_obj_get_style_bg_color(box, LV_PART_MAIN);
        lv_color_t new_color = lv_color_hex(lv_color_to_u32(current_color) ^ 0x00FF00);
        lv_obj_set_style_bg_color(box, new_color, LV_PART_MAIN);
    }
}

}

namespace AppUI {

void build()
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_screen_load(scr);

    lv_obj_t *header = lv_obj_create(scr);
    lv_obj_set_size(header, LV_PCT(100), 45);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x0a0a0a), LV_PART_MAIN);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(header, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    label_rotation = lv_label_create(header);
    lv_obj_align(label_rotation, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *rotate_btn = lv_button_create(header);
    lv_obj_set_size(rotate_btn, 100, 35);
    lv_obj_align(rotate_btn, LV_ALIGN_CENTER, 110, 0);
    lv_obj_add_event_cb(rotate_btn, rotate_btn_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *rotate_label = lv_label_create(rotate_btn);
    lv_label_set_text(rotate_label, "Rotate");
    lv_obj_center(rotate_label);

    container = lv_obj_create(scr);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100) - 45);
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 45);
    lv_obj_set_style_bg_color(container, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
    lv_obj_set_style_pad_all(container, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    const uint32_t colors[] = {0xff5555, 0x55ff55, 0x5555ff, 0xffff55, 0xff55ff, 0x55ffff};
    for(int i = 0; i < 6; i++) {
        lv_obj_t *box = lv_obj_create(container);
        lv_obj_set_size(box, 75, 75);
        lv_obj_set_style_bg_color(box, lv_color_hex(colors[i]), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(box, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(box, 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(box, lv_color_hex(0x222222), LV_PART_MAIN);
        lv_obj_set_style_radius(box, 8, LV_PART_MAIN);
        lv_obj_add_event_cb(box, box_event_cb, LV_EVENT_ALL, NULL);

        lv_obj_t *txt = lv_label_create(box);
        lv_label_set_text_fmt(txt, "%d", i + 1);
        lv_obj_center(txt);
    }

    setRotationLabel(DisplayManager::rotationDegrees());
}

void setRotationLabel(int degrees)
{
    if(label_rotation != nullptr) {
        lv_label_set_text_fmt(label_rotation, "Rotation: %d deg", degrees);
    }
}

}