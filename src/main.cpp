#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_random.h"
#include <math.h>
#include "driver/gpio.h"

#include "AppConfig.h"
#include "DisplayManager.h"
#include "ui.h"
#include "screens.h"

static const char* TAG = "MAIN";

static lv_timer_t* star_timer = nullptr;
static lv_timer_t* shooting_star_timer = nullptr;

static void set_obj_x_cb(void * var, int32_t v) { lv_obj_set_x((lv_obj_t*)var, v); }
static void set_obj_y_cb(void * var, int32_t v) { lv_obj_set_y((lv_obj_t*)var, v); }

// Timer callback for shooting star animation
static void shooting_star_anim_timer_cb(lv_timer_t * timer) {
    if (!objects.shooting_star || !objects.starcontainer) return;

    if (lv_obj_has_flag(objects.shooting_star, LV_OBJ_FLAG_HIDDEN)) {
        // Unhide and init movement parameters
        lv_coord_t parent_w = lv_obj_get_width(objects.starcontainer);
        lv_coord_t parent_h = lv_obj_get_height(objects.starcontainer);
        lv_coord_t star_w = lv_obj_get_width(objects.shooting_star);
        lv_coord_t star_h = lv_obj_get_height(objects.shooting_star);
        
        lv_coord_t max_x = parent_w - star_w;
        lv_coord_t max_y = parent_h - star_h;
        
        if (max_x <= 0) max_x = 1;
        if (max_y <= 0) max_y = 1;

        // Pick random start and end points within boundaries
        int32_t start_x = esp_random() % max_x;
        int32_t start_y = esp_random() % max_y;
        int32_t end_x = esp_random() % max_x;
        int32_t end_y = esp_random() % max_y;
        
        // Calculate the trajectory angle to always face the direction of flight
        double dx = end_x - start_x;
        double dy = end_y - start_y;
        double angle_rad = atan2(dy, dx);
        double angle_deg = angle_rad * 180.0 / 3.14159265;
        if (angle_deg < 0) angle_deg += 360.0;
        
        // Rotate (requires uncompressed RGB565A8 images to avoid CPU stutter)
        lv_image_set_rotation(objects.shooting_star, (int32_t)(angle_deg * 10));

        // Uniform scaling between -30% to +50% (base 256 -> 179 to 384)
        uint32_t scale = 179 + (esp_random() % 206);
        lv_image_set_scale(objects.shooting_star, scale);
        
        // Speed (animation duration) +/- 50% of the normal 600ms. (300ms to 900ms)
        // Because the physical distance changes but duration is random, the perceived speed varies wildly! 
        uint32_t anim_duration = 300 + (esp_random() % 601);
        lv_animimg_set_duration(objects.shooting_star, anim_duration);
        
        // Setup linear motion animation for X
        lv_anim_t a_x;
        lv_anim_init(&a_x);
        lv_anim_set_var(&a_x, objects.shooting_star);
        lv_anim_set_duration(&a_x, anim_duration);
        lv_anim_set_values(&a_x, start_x, end_x);
        lv_anim_set_exec_cb(&a_x, set_obj_x_cb);
        lv_anim_start(&a_x);
        
        // Setup linear motion animation for Y
        lv_anim_t a_y;
        lv_anim_init(&a_y);
        lv_anim_set_var(&a_y, objects.shooting_star);
        lv_anim_set_duration(&a_y, anim_duration);
        lv_anim_set_values(&a_y, start_y, end_y);
        lv_anim_set_exec_cb(&a_y, set_obj_y_cb);
        lv_anim_start(&a_y);
        
        // Restart the sprite animation so it plays from frame 1
        lv_animimg_start(objects.shooting_star);
        
        lv_obj_clear_flag(objects.shooting_star, LV_OBJ_FLAG_HIDDEN);

        // Keep visible for exactly one animation cycle
        lv_timer_set_period(shooting_star_timer, anim_duration);
    } else {
        // Hide shooting star
        lv_obj_add_flag(objects.shooting_star, LV_OBJ_FLAG_HIDDEN);
        
        // Next appearance in 10 to 20 seconds
        uint32_t next_show = 10000 + (esp_random() % 10001);
        lv_timer_set_period(shooting_star_timer, next_show);
    }
}

// Timer callback for twinkling star animation
static void star_anim_timer_cb(lv_timer_t * timer) {
    if (!objects.star || !objects.starcontainer) return;

    if (lv_obj_has_flag(objects.star, LV_OBJ_FLAG_HIDDEN)) {
        // Star is currently hidden, show it at a random position
        lv_coord_t parent_w = lv_obj_get_width(objects.starcontainer);
        lv_coord_t parent_h = lv_obj_get_height(objects.starcontainer);
        lv_coord_t star_w = lv_obj_get_width(objects.star);
        lv_coord_t star_h = lv_obj_get_height(objects.star);
        
        lv_coord_t max_x = parent_w - star_w;
        lv_coord_t max_y = parent_h - star_h;
        
        if (max_x > 0 && max_y > 0) {
            lv_coord_t rand_x = esp_random() % max_x;
            lv_coord_t rand_y = esp_random() % max_y;
            lv_obj_set_pos(objects.star, rand_x, rand_y);
        }
        
        // Random scale +/- 30% (base is 256 for 100%, 30% is 77 -> range 179 to 333)
        uint32_t scale = 179 + (esp_random() % 154);
        lv_image_set_scale(objects.star, scale);
        
        // Random duration between 500 and 1000 ms
        uint32_t anim_duration = 500 + (esp_random() % 501);
        lv_animimg_set_duration(objects.star, anim_duration);
        
        // Restart animation so it always starts at frame 1 when shown
        lv_animimg_start(objects.star);
        
        lv_obj_clear_flag(objects.star, LV_OBJ_FLAG_HIDDEN);

        // 80% chance of blinking 1 time, 20% chance of blinking 2 times
        uint32_t repeat_count = (esp_random() % 100 < 80) ? 1 : 2;
        
        // Keep visible for enough time to finish the animation repeat_count times
        lv_timer_set_period(star_timer, anim_duration * repeat_count);
    } else {
        // Hide star
        lv_obj_add_flag(objects.star, LV_OBJ_FLAG_HIDDEN);
        
        // Next appearance in 1 to 3.5 seconds
        uint32_t next_show = 1000 + (esp_random() % 2500);
        lv_timer_set_period(star_timer, next_show);
    }
}

// Main application entry point
extern "C" void app_main()
{
    gpio_set_direction((gpio_num_t)TFT_BL, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)TFT_BL, 0);

    vTaskDelay(pdMS_TO_TICKS(200));

    ESP_LOGI(TAG, "[Mem] Startup memory report");
    
    size_t psram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    
    if(psram_total > 0) {
        ESP_LOGI(TAG, "[Mem] PSRAM total: %zu bytes", psram_total);     
        ESP_LOGI(TAG, "[Mem] PSRAM free : %zu bytes", psram_free);     
    } else {
        ESP_LOGI(TAG, "[Mem] PSRAM not detected");
    }
    
    ESP_LOGI(TAG, "[Mem] Heap total : %zu bytes", heap_caps_get_total_size(MALLOC_CAP_INTERNAL));
    ESP_LOGI(TAG, "[Mem] Heap free  : %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    if(!DisplayManager::begin()) {
        ESP_LOGE(TAG, "Display init failed");
        while(true) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    ui_init();

    if (objects.star) {
        lv_obj_add_flag(objects.star, LV_OBJ_FLAG_HIDDEN);
        star_timer = lv_timer_create(star_anim_timer_cb, 1000, NULL);
    }

    if (objects.shooting_star) {
        lv_obj_add_flag(objects.shooting_star, LV_OBJ_FLAG_HIDDEN);
        shooting_star_timer = lv_timer_create(shooting_star_anim_timer_cb, 5000 + (esp_random() % 10001), NULL);
    }

    while(true) {
        DisplayManager::process();
        ui_tick();
        static uint32_t last_print = 0;
        if(lv_tick_get() - last_print > 5000) {
            ESP_LOGI(TAG, "Main task stack HWM: %d words", uxTaskGetStackHighWaterMark(NULL));
            ESP_LOGI(TAG, "Heap free: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
            last_print = lv_tick_get();
        }
    }
}
