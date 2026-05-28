#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * OCV-SOC Lookup Table for LG 18650HG2
 * 13 calibration points, integer linear interpolation
 * Input: mV, Output: SOC * 10 (0-1000, i.e. 0.0%-100.0%)
 */

static const int32_t ocv_table_mV[13] = {
    2500, 3150, 3330, 3450, 3530, 3600, 3680,
    3760, 3860, 3960, 4060, 4130, 4200
};

static const int32_t soc_table_x10[13] = {
    0, 50, 100, 200, 300, 400, 500,
    600, 700, 800, 900, 950, 1000
};

/**
 * @brief Get SOC from open-circuit voltage using linear interpolation
 * @param ocv_mV Battery open-circuit voltage in millivolts
 * @return SOC * 10 (0 = 0%, 1000 = 100.0%)
 */
static inline int32_t soc_from_ocv_mV(int32_t ocv_mV)
{
    if (ocv_mV <= ocv_table_mV[0]) return soc_table_x10[0];
    if (ocv_mV >= ocv_table_mV[12]) return soc_table_x10[12];

    for (int i = 1; i < 13; i++) {
        if (ocv_mV <= ocv_table_mV[i]) {
            int32_t v0 = ocv_table_mV[i - 1];
            int32_t v1 = ocv_table_mV[i];
            int32_t s0 = soc_table_x10[i - 1];
            int32_t s1 = soc_table_x10[i];
            return s0 + (int32_t)(((int64_t)(ocv_mV - v0) * (s1 - s0)) / (v1 - v0));
        }
    }
    return soc_table_x10[12];
}

#ifdef __cplusplus
}
#endif
