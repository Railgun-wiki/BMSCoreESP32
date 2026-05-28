#include "app.hpp"
#include "bms_hw.h"
#include "bms_config.h"

void app_init(void) {
    bms_hw_init();
}

void app_run(void) {
    bms_hw_tick_200ms();
}
