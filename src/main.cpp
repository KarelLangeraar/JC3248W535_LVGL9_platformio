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
#include "StarAnimations.h"

static const char* TAG = "MAIN";

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

    // Initialize star animation system
    StarAnimations_init(objects.starcontainer, objects.star, 
                       objects.shooting_star, objects.main);

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
