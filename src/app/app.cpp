#include "app.hpp"
#include "bms_config.h"

extern "C" {
#include "bms_hw.h"
}

void app_init(void) {
    // TODO: This function is never called from main.cpp — dead code, consider removing or integrating
    bms_hw_init();
}

void app_run(void) {
    // TODO: This function is never called from any task — dead code, consider removing or integrating
    bms_hw_tick_200ms();
}
