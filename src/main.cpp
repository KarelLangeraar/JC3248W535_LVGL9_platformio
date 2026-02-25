#include <Arduino.h>

#include "AppConfig.h"
#include "AppUI.h"
#include "DisplayManager.h"

void setup()
{
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, LOW);

    Serial.begin(115200);
    delay(200);

    Serial.println("[Mem] Startup memory report");
    if(psramFound()) {
        Serial.printf("[Mem] PSRAM total: %u bytes\n", ESP.getPsramSize());
        Serial.printf("[Mem] PSRAM free : %u bytes\n", ESP.getFreePsram());
    } else {
        Serial.println("[Mem] PSRAM not detected");
    }
    Serial.printf("[Mem] Heap total : %u bytes\n", ESP.getHeapSize());
    Serial.printf("[Mem] Heap free  : %u bytes\n", ESP.getFreeHeap());

    if(!DisplayManager::begin()) {
        Serial.println("Display init failed");
        while(true) {
            delay(1000);
        }
    }

    AppUI::build();
}

void loop()
{
    DisplayManager::process();
}
