FILE(GLOB_RECURSE app_sources 
    "*.cpp" 
    "*.c"
    "../LightSwitchEez/src/ui/*.c"
    "../LightSwitchEez/src/ui/*.cpp"
)

idf_component_register(SRCS ${app_sources}
                    INCLUDE_DIRS "." "../include" "../LightSwitchEez/src/ui"
                    REQUIRES esp_timer esp_lcd driver freertos nvs_flash)
