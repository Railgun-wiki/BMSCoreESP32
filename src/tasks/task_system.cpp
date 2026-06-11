#include "tasks.h"
#include <Arduino.h>
#include <OneButton.h>
#include <WiFi.h>
#include "pin_config.h"

static void handleLongPressStart() {
    Serial.println("[SYSTEM] Flash button long press detected! Starting SmartConfig...");
    WiFi.beginSmartConfig();
}

void taskSystem(void* param) {
    OneButton btnFlash(PIN_FLASH_BTN, true, true);
    btnFlash.setPressTicks(3000); // 3 seconds long press
    btnFlash.attachLongPressStart(handleLongPressStart);

    Serial.println("[SYSTEM] taskSystem running");

    while (1) {
        btnFlash.tick();
        // TODO: Handle WS2812 LED blinking here in the future
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
