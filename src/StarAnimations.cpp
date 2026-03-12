#include "StarAnimations.h"
#include "esp_random.h"
#include <math.h>

#define MAX_STARS 3
#define MAX_SHOOTING_STARS 2
#define MAX_ANIMATION_TIMERS (MAX_STARS + MAX_SHOOTING_STARS)

// Global animation state
static struct {
    lv_obj_t *starcontainer;
    lv_obj_t *main_screen;
    lv_timer_t *timers[MAX_ANIMATION_TIMERS];
    uint32_t timer_count;
} g_anim_state = {0};

// Callback setters for LVGL animations
static void set_obj_x_cb(void *var, int32_t v) { 
    lv_obj_set_x((lv_obj_t*)var, v); 
}

static void set_obj_y_cb(void *var, int32_t v) { 
    lv_obj_set_y((lv_obj_t*)var, v); 
}

/**
 * @brief Clone an animimg template object
 * 
 * Creates a duplicate animimg with the same properties, used to spawn
 * additional star instances from a single template.
 */
static lv_obj_t *clone_animimg(lv_obj_t *template_obj) {
    lv_obj_t *new_obj = lv_animimg_create(lv_obj_get_parent(template_obj));
    lv_animimg_set_src(new_obj, lv_animimg_get_src(template_obj), 
                       lv_animimg_get_src_count(template_obj));
    lv_obj_set_size(new_obj, lv_obj_get_width(template_obj), 
                    lv_obj_get_height(template_obj));
    lv_animimg_set_duration(new_obj, lv_animimg_get_duration(template_obj));
    lv_animimg_set_repeat_count(new_obj, lv_animimg_get_repeat_count(template_obj));
    
    // Remove interaction flags
    lv_obj_remove_flag(new_obj, (lv_obj_flag_t)(
        LV_OBJ_FLAG_ADV_HITTEST | 
        LV_OBJ_FLAG_CLICK_FOCUSABLE | 
        LV_OBJ_FLAG_SCROLLABLE | 
        LV_OBJ_FLAG_SCROLL_CHAIN_HOR | 
        LV_OBJ_FLAG_SNAPPABLE
    ));
    
    lv_obj_add_flag(new_obj, LV_OBJ_FLAG_HIDDEN);
    return new_obj;
}

/**
 * @brief Timer callback for shooting star animation
 * 
 * Manages the lifecycle of a single shooting star:
 * - Hidden state: wait for next appearance interval
 * - Visible state: animate motion and sprite, then hide after duration
 * - Off-screen: pause and poll until returning to main screen
 */
static void shooting_star_anim_timer_cb(lv_timer_t *timer) {
    lv_obj_t *star_obj = (lv_obj_t *)lv_timer_get_user_data(timer);
    if (!star_obj || !g_anim_state.starcontainer) return;

    // Pause when off the main screen
    if (lv_scr_act() != g_anim_state.main_screen) {
        if (!lv_obj_has_flag(star_obj, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_add_flag(star_obj, LV_OBJ_FLAG_HIDDEN);
            lv_anim_delete(star_obj, NULL);
        }
        lv_timer_set_period(timer, 1000);
        return;
    }

    if (lv_obj_has_flag(star_obj, LV_OBJ_FLAG_HIDDEN)) {
        // Init movement parameters
        lv_coord_t parent_w = lv_obj_get_width(g_anim_state.starcontainer);
        lv_coord_t parent_h = lv_obj_get_height(g_anim_state.starcontainer);
        lv_coord_t star_w = lv_obj_get_width(star_obj);
        lv_coord_t star_h = lv_obj_get_height(star_obj);
        
        lv_coord_t max_x = parent_w - star_w;
        lv_coord_t max_y = parent_h - star_h;
        
        if (max_x <= 0) max_x = 1;
        if (max_y <= 0) max_y = 1;

        // Pick random start and end points
        int32_t start_x = esp_random() % max_x;
        int32_t start_y = esp_random() % max_y;
        int32_t end_x = esp_random() % max_x;
        int32_t end_y = esp_random() % max_y;
        
        // Calculate trajectory angle
        double dx = end_x - start_x;
        double dy = end_y - start_y;
        double angle_rad = atan2(dy, dx);
        double angle_deg = angle_rad * 180.0 / 3.14159265;
        if (angle_deg < 0) angle_deg += 360.0;
        
        lv_image_set_rotation(star_obj, (int32_t)(angle_deg * 10));

        // Random scale: -30% to +50% (base 256)
        uint32_t scale = 179 + (esp_random() % 206);
        lv_image_set_scale(star_obj, scale);
        
        // Random speed: 300ms to 900ms
        uint32_t anim_duration = 300 + (esp_random() % 601);
        lv_animimg_set_duration(star_obj, anim_duration);
        
        // Setup X motion
        lv_anim_t a_x;
        lv_anim_init(&a_x);
        lv_anim_set_var(&a_x, star_obj);
        lv_anim_set_duration(&a_x, anim_duration);
        lv_anim_set_values(&a_x, start_x, end_x);
        lv_anim_set_exec_cb(&a_x, set_obj_x_cb);
        lv_anim_start(&a_x);
        
        // Setup Y motion
        lv_anim_t a_y;
        lv_anim_init(&a_y);
        lv_anim_set_var(&a_y, star_obj);
        lv_anim_set_duration(&a_y, anim_duration);
        lv_anim_set_values(&a_y, start_y, end_y);
        lv_anim_set_exec_cb(&a_y, set_obj_y_cb);
        lv_anim_start(&a_y);
        
        // Restart sprite animation
        lv_animimg_start(star_obj);
        lv_obj_clear_flag(star_obj, LV_OBJ_FLAG_HIDDEN);

        // Keep visible for one full animation cycle
        lv_timer_set_period(timer, anim_duration);
    } else {
        // Hide and schedule next appearance
        lv_obj_add_flag(star_obj, LV_OBJ_FLAG_HIDDEN);
        uint32_t next_show = 10000 + (esp_random() % 10001);
        lv_timer_set_period(timer, next_show);
    }
}

/**
 * @brief Timer callback for twinkling star animation
 * 
 * Manages the lifecycle of a single twinkling star:
 * - Hidden state: wait for next appearance interval
 * - Visible state: fade in/out animation (twinkle), then hide after duration
 * - Off-screen: pause and poll until returning to main screen
 */
static void star_anim_timer_cb(lv_timer_t *timer) {
    lv_obj_t *star_obj = (lv_obj_t *)lv_timer_get_user_data(timer);
    if (!star_obj || !g_anim_state.starcontainer) return;

    // Pause when off the main screen
    if (lv_scr_act() != g_anim_state.main_screen) {
        if (!lv_obj_has_flag(star_obj, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_add_flag(star_obj, LV_OBJ_FLAG_HIDDEN);
            lv_anim_delete(star_obj, NULL);
        }
        lv_timer_set_period(timer, 1000);
        return;
    }

    if (lv_obj_has_flag(star_obj, LV_OBJ_FLAG_HIDDEN)) {
        // Show at random position
        lv_coord_t parent_w = lv_obj_get_width(g_anim_state.starcontainer);
        lv_coord_t parent_h = lv_obj_get_height(g_anim_state.starcontainer);
        lv_coord_t star_w = lv_obj_get_width(star_obj);
        lv_coord_t star_h = lv_obj_get_height(star_obj);
        
        lv_coord_t max_x = parent_w - star_w;
        lv_coord_t max_y = parent_h - star_h;
        
        if (max_x > 0 && max_y > 0) {
            lv_coord_t rand_x = esp_random() % max_x;
            lv_coord_t rand_y = esp_random() % max_y;
            lv_obj_set_pos(star_obj, rand_x, rand_y);
        }
        
        // Random scale: ±30% (base 256)
        uint32_t scale = 179 + (esp_random() % 154);
        lv_image_set_scale(star_obj, scale);
        
        // Random animation duration: 500-1500ms
        uint32_t anim_duration = 500 + (esp_random() % 1001);
        lv_animimg_set_duration(star_obj, anim_duration);
        
        lv_animimg_start(star_obj);
        lv_obj_clear_flag(star_obj, LV_OBJ_FLAG_HIDDEN);

        // 80% chance of 1 twinkle, 20% chance of 2 twinkles
        uint32_t repeat_count = (esp_random() % 100 < 80) ? 1 : 2;
        lv_timer_set_period(timer, anim_duration * repeat_count);

    } else {
        // Hide and schedule next appearance
        lv_obj_add_flag(star_obj, LV_OBJ_FLAG_HIDDEN);
        uint32_t next_show = 1000 + (esp_random() % 2500);
        lv_timer_set_period(timer, next_show);
    }
}

void StarAnimations_init(lv_obj_t *starcontainer, lv_obj_t *star, 
                         lv_obj_t *shooting_star, lv_obj_t *main_screen) {
    if (!starcontainer || !main_screen) return;
    
    g_anim_state.starcontainer = starcontainer;
    g_anim_state.main_screen = main_screen;
    g_anim_state.timer_count = 0;

    // Initialize twinkling stars
    if (star && g_anim_state.timer_count < MAX_ANIMATION_TIMERS) {
        lv_obj_add_flag(star, LV_OBJ_FLAG_HIDDEN);
        lv_timer_t *timer = lv_timer_create(star_anim_timer_cb, 1000, star);
        g_anim_state.timers[g_anim_state.timer_count++] = timer;
        
        for (int i = 1; i < MAX_STARS && g_anim_state.timer_count < MAX_ANIMATION_TIMERS; i++) {
            lv_obj_t *new_star = clone_animimg(star);
            uint32_t delay = 1000 + (esp_random() % 2000);
            timer = lv_timer_create(star_anim_timer_cb, delay, new_star);
            g_anim_state.timers[g_anim_state.timer_count++] = timer;
        }
    }

    // Initialize shooting stars
    if (shooting_star && g_anim_state.timer_count < MAX_ANIMATION_TIMERS) {
        lv_obj_add_flag(shooting_star, LV_OBJ_FLAG_HIDDEN);
        uint32_t delay = 5000 + (esp_random() % 5000);
        lv_timer_t *timer = lv_timer_create(shooting_star_anim_timer_cb, delay, shooting_star);
        g_anim_state.timers[g_anim_state.timer_count++] = timer;
        
        for (int i = 1; i < MAX_SHOOTING_STARS && g_anim_state.timer_count < MAX_ANIMATION_TIMERS; i++) {
            lv_obj_t *new_shooting_star = clone_animimg(shooting_star);
            uint32_t delay = 8000 + (esp_random() % 8000);
            timer = lv_timer_create(shooting_star_anim_timer_cb, delay, new_shooting_star);
            g_anim_state.timers[g_anim_state.timer_count++] = timer;
        }
    }
}

void StarAnimations_shutdown(void) {
    for (uint32_t i = 0; i < g_anim_state.timer_count; i++) {
        if (g_anim_state.timers[i]) {
            lv_timer_delete(g_anim_state.timers[i]);
            g_anim_state.timers[i] = NULL;
        }
    }
    g_anim_state.timer_count = 0;
}
