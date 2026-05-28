#include <Arduino.h>
#include "mqtt_bridge.h"

extern "C" void taskMqtt(void* param) {
    (void)param;

    for (;;) {
        mqttBridgeLoop();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
