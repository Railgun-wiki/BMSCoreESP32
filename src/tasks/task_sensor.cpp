#include <Arduino.h>
#include "bms_hw.h"

extern "C" void taskSensorInit(void) {
    // No-op: mutexes now managed internally by bms_hw
}

extern "C" void taskSensor(void* param) {
    (void)param;

    for (;;) {
        bms_hw_tick_200ms();
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
