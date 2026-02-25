#include <Arduino.h>

#include "AppUI.h"
#include "DisplayManager.h"

void setup()
{
    Serial.begin(115200);

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
