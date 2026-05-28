#include "lv_port_disp.hpp"
#include "lvgl.h"

static St7789* s_lcd;

static void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map)
{
    s_lcd->flush_cb(area->x1, area->y1, area->x2, area->y2, (uint16_t*)px_map);
    lv_display_flush_ready(disp);
}

extern "C" void lv_port_disp_init(St7789* lcd)
{
    s_lcd = lcd;

    lv_display_t* d = lv_display_create(lcd->getWidth(), lcd->getHeight());
    lv_display_set_flush_cb(d, flush_cb);

    // Double buffer for ESP32 (2x 2400 bytes = 5 lines each)
    static uint16_t buf1[240 * 5];
    static uint16_t buf2[240 * 5];
    lv_display_set_buffers(d, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
}
