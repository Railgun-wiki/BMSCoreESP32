#include <Arduino.h>
#include "bms_hw.h"
#include "bms_ui.h"
#include "ocv_soc.h"

static SemaphoreHandle_t s_stateMutex = nullptr;

extern "C" void taskSensorInit(void) {
    s_stateMutex = xSemaphoreCreateMutex();
}

extern "C" void taskSensor(void* param) {
    (void)param;

    for (;;) {
        // Read sensors via bms_hw (updates internal bms_state_t)
        bms_hw_tick_200ms();

        // Update SOC from OCV
        if (s_stateMutex && xSemaphoreTake(s_stateMutex, pdMS_TO_TICKS(10))) {
            const bms_state_t* state = bms_hw_get_state();
            if (state && state->voltage_mV > 0) {
                int32_t soc = soc_from_ocv_mV(state->voltage_mV);
                // Write SOC back (bms_state_t is mutable via non-const access in bms_hw)
                // For now, bms_hw_tick_200ms should handle this internally
                (void)soc;
            }
            xSemaphoreGive(s_stateMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
