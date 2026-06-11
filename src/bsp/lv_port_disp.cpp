#include "lv_port_disp.hpp"
#include "lvgl.h"
#include <esp_heap_caps.h>

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

    // Double buffer for ESP32 DMA (Half screen = 240 * 135 / 2 = 16200 pixels)
    uint32_t buf_size = (lcd->getWidth() * lcd->getHeight() / 2) * sizeof(uint16_t);
    
    uint16_t* buf1 = (uint16_t*)heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    uint16_t* buf2 = (uint16_t*)heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    lv_display_set_buffers(d, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
}
