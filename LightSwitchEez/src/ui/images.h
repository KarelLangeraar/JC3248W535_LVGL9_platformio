#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_back_main;
extern const lv_img_dsc_t img_star1;
extern const lv_img_dsc_t img_star2;
extern const lv_img_dsc_t img_star3;
extern const lv_img_dsc_t img_star4;
extern const lv_img_dsc_t img_star5;
extern const lv_img_dsc_t img_star6;
extern const lv_img_dsc_t img_star7;
extern const lv_img_dsc_t img_star8;
extern const lv_img_dsc_t img_star9;
extern const lv_img_dsc_t img_star10;
extern const lv_img_dsc_t img_shooting_star_1;
extern const lv_img_dsc_t img_shooting_star_2;
extern const lv_img_dsc_t img_shooting_star_3;
extern const lv_img_dsc_t img_shooting_star_4;
extern const lv_img_dsc_t img_shooting_star_5;
extern const lv_img_dsc_t img_shooting_star_6;
extern const lv_img_dsc_t img_shooting_star_7;
extern const lv_img_dsc_t img_shooting_star_8;
extern const lv_img_dsc_t img_shooting_star_9;
extern const lv_img_dsc_t img_shooting_star_10;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[21];

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/