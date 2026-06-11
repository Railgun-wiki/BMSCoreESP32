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
    btnFlash.setPressMs(3000); // 3 seconds long press
    btnFlash.attachLongPressStart(handleLongPressStart);

    Serial.println("[SYSTEM] taskSystem running");

    uint32_t tick_count = 0;

    while (1) {
        btnFlash.tick();
        
        // Handle WS2812 LED status (1 tick = 10ms)
        tick_count++;
        
        if (WiFi.status() != WL_CONNECTED) {
            // Not connected / SmartConfig mode: Blink Blue (500ms ON, 500ms OFF)
            if ((tick_count % 100) < 50) {
                rgbLedWrite(PIN_RGB_LED, 0, 0, 32); // Dim blue
            } else {
                rgbLedWrite(PIN_RGB_LED, 0, 0, 0);
            }
        } else {
            // Connected: Breathing Green (2s cycle)
            uint32_t cycle = tick_count % 200;
            uint8_t brightness = 0;
            if (cycle < 100) {
                // Fade in: 0 to 32
                brightness = (cycle * 32) / 100;
            } else {
                // Fade out: 32 to 0
                brightness = ((200 - cycle) * 32) / 100;
            }
            rgbLedWrite(PIN_RGB_LED, 0, brightness, 0); // Green breathing
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
