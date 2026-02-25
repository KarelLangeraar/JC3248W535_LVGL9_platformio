#include "AppUI.h"

#include <Arduino.h>
#include <lvgl.h>

#include "DisplayManager.h"

namespace {

lv_obj_t *label_rotation = nullptr;
lv_obj_t *container = nullptr;
lv_obj_t *label_memory = nullptr;
lv_obj_t *label_backlight = nullptr;

void update_memory_label();

void memory_refresh_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    update_memory_label();
}

void update_memory_label()
{
    if(label_memory == nullptr) {
        return;
    }

    uint32_t heap_free_kb = ESP.getFreeHeap() / 1024U;
    if(psramFound()) {
        uint32_t psram_free_kb = ESP.getFreePsram() / 1024U;
        lv_label_set_text_fmt(label_memory, "Heap: %lu KB\nPSRAM: %lu KB", heap_free_kb, psram_free_kb);
    } else {
        lv_label_set_text_fmt(label_memory, "Heap: %lu KB\nPSRAM: N/A", heap_free_kb);
    }
}

void backlight_slider_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }

    static int last_applied_value = -1;

    lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
    int value = lv_slider_get_value(slider);

    int quantized = ((value + 1) / 2) * 2;
    if(quantized < 1) {
        quantized = 1;
    }
    if(quantized > 100) {
        quantized = 100;
    }

    if(last_applied_value == quantized) {
        return;
    }

    last_applied_value = quantized;

    if(value != quantized) {
        lv_slider_set_value(slider, quantized, LV_ANIM_OFF);
    }

    DisplayManager::setBacklightPercent((uint8_t)quantized);
    if(label_backlight != nullptr) {
        lv_label_set_text_fmt(label_backlight, "BL: %d%%", quantized);
    }
}

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
    constexpr int32_t header_height = 45;
    constexpr int32_t footer_height = 112;

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(scr, lv_color_hex(0xE6E6E6), LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(scr, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_screen_load(scr);

    lv_obj_t *header = lv_obj_create(scr);
    lv_obj_set_size(header, LV_PCT(100), header_height);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x0a0a0a), LV_PART_MAIN);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(header, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_set_style_pad_all(header, 4, LV_PART_MAIN);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    label_rotation = lv_label_create(header);
    lv_obj_align(label_rotation, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_set_style_text_color(label_rotation, lv_color_hex(0xF5F5F5), LV_PART_MAIN);

    lv_obj_t *rotate_btn = lv_button_create(header);
    lv_obj_set_size(rotate_btn, 100, 35);
    lv_obj_align(rotate_btn, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_add_event_cb(rotate_btn, rotate_btn_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *rotate_label = lv_label_create(rotate_btn);
    lv_label_set_text(rotate_label, "Rotate");
    lv_obj_center(rotate_label);

    container = lv_obj_create(scr);
    lv_obj_set_width(container, LV_PCT(100));
    lv_obj_set_flex_grow(container, 1);
    lv_obj_set_style_bg_color(container, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
    lv_obj_set_style_pad_all(container, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
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

    lv_obj_t *footer = lv_obj_create(scr);
    lv_obj_set_size(footer, LV_PCT(100), footer_height);
    lv_obj_set_style_bg_color(footer, lv_color_hex(0x0a0a0a), LV_PART_MAIN);
    lv_obj_set_style_border_side(footer, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    lv_obj_set_style_border_width(footer, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(footer, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_set_style_pad_all(footer, 10, LV_PART_MAIN);
    lv_obj_remove_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    label_memory = lv_label_create(footer);
    lv_obj_align(label_memory, LV_ALIGN_TOP_LEFT, 6, 0);
    lv_obj_set_style_text_color(label_memory, lv_color_hex(0xEDEDED), LV_PART_MAIN);
    update_memory_label();

    label_backlight = lv_label_create(footer);
    lv_obj_align(label_backlight, LV_ALIGN_TOP_RIGHT, -6, 2);
    lv_obj_set_style_text_color(label_backlight, lv_color_hex(0xEDEDED), LV_PART_MAIN);
    lv_label_set_text_fmt(label_backlight, "BL: %u%%", DisplayManager::backlightPercent());

    lv_obj_t *slider = lv_slider_create(footer);
    lv_obj_set_width(slider, LV_PCT(92));
    lv_obj_set_height(slider, 18);
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_pad_all(slider, 6, LV_PART_KNOB);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x5A5A5A), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xC8C8C8), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    lv_slider_set_range(slider, 1, 100);
    lv_slider_set_value(slider, DisplayManager::backlightPercent(), LV_ANIM_OFF);
    lv_obj_add_flag(slider, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_event_cb(slider, backlight_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_timer_create(memory_refresh_timer_cb, 1000, NULL);

    setRotationLabel(DisplayManager::rotationDegrees());
}

void setRotationLabel(int degrees)
{
    if(label_rotation != nullptr) {
        lv_label_set_text_fmt(label_rotation, "Rotation: %d deg", degrees);
    }
}

}