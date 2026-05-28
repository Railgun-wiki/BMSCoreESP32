#include <Arduino.h>
#include <lvgl.h>

extern "C" void taskLvgl(void* param) {
    (void)param;
    for (;;) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
