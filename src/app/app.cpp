#include "app.hpp"
#include "bms_config.h"

extern "C" {
#include "bms_hw.h"
}

void app_init(void) {
    bms_hw_init();
}

void app_run(void) {
    bms_hw_tick_200ms();
}
