#include "st7789.hpp"
#include <Arduino.h>
#include <cstring>

static int s_dcPin = -1;

static void IRAM_ATTR lcd_spi_pre_transfer_callback(spi_transaction_t *t) {
    int dc = (int)t->user;
    if (s_dcPin >= 0) {
        digitalWrite(s_dcPin, dc);
    }
}

St7789::St7789(int mosiPin, int sclkPin, int csPin, int dcPin, int rstPin, int blkPin)
    : m_mosiPin(mosiPin), m_sclkPin(sclkPin), m_csPin(csPin), m_dcPin(dcPin), m_rstPin(rstPin), m_blkPin(blkPin),
      m_width(135), m_height(240), m_xOffset(52), m_yOffset(40), m_dma_in_flight(false) {
    s_dcPin = m_dcPin;
}

void St7789::init() {
    pinMode(m_dcPin, OUTPUT);
    pinMode(m_rstPin, OUTPUT);
    pinMode(m_blkPin, OUTPUT);

    // 1. Initialize SPI bus (SPI2_HOST = FSPI)
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = m_mosiPin;
    buscfg.miso_io_num = -1; // Transmit Only
    buscfg.sclk_io_num = m_sclkPin;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = (240 * 135 * 2) + 8;

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // 2. Add device to SPI bus
    spi_device_interface_config_t devcfg = {};
    devcfg.command_bits = 0;
    devcfg.address_bits = 0;
    devcfg.dummy_bits = 0;
    devcfg.mode = 0;
    devcfg.clock_speed_hz = SPI_MASTER_FREQ_80M; // 80 MHz
    devcfg.spics_io_num = m_csPin;
    devcfg.flags = SPI_DEVICE_HALFDUPLEX; // Transmit Only Master
    devcfg.queue_size = 7;
    devcfg.pre_cb = lcd_spi_pre_transfer_callback;

    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &m_spi_handle));

    // Hardware Reset
    digitalWrite(m_rstPin, HIGH);
    delay(10);
    digitalWrite(m_rstPin, LOW);
    delay(20);
    digitalWrite(m_rstPin, HIGH);
    delay(120);

    writeCmd(0x11); // Sleep Out
    delay(120);

    writeCmd(0x36); // MADCTL
    writeData(0x00); // Default Portrait

    writeCmd(0x3A); // COLMOD
    writeData(0x05); // RGB565

    writeCmd(0xB2); // PORCTRL
    uint8_t porctrl[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
    writeData(porctrl, 5);

    writeCmd(0xB7); // GCTRL
    writeData(0x35);

    writeCmd(0xBB); // VCOMS
    writeData(0x19);

    writeCmd(0xC0); // LCMCTRL
    writeData(0x2C);

    writeCmd(0xC2); // VDVVRHEN
    writeData(0x01);

    writeCmd(0xC3); // VRHS
    writeData(0x12);

    writeCmd(0xC4); // VDVS
    writeData(0x20);

    writeCmd(0xC6); // FRCTRL2
    writeData(0x0F);

    writeCmd(0xD0); // PWCTRL1
    uint8_t pwctrl[] = {0xA4, 0xA1};
    writeData(pwctrl, 2);

    writeCmd(0xE0); // PVGAMCTRL
    uint8_t pvgam[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
    writeData(pvgam, 14);

    writeCmd(0xE1); // NVGAMCTRL
    uint8_t nvgam[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
    writeData(nvgam, 14);

    writeCmd(0x21); // Display Inversion On
    writeCmd(0x29); // Display On

    fillRect(0, 0, m_width, m_height, 0x0000);

    // Backlight On
    digitalWrite(m_blkPin, HIGH);
}

void St7789::setRotation(Rotation rotation) {
    writeCmd(0x36);
    switch (rotation) {
        case Rotation::Portrait:
            writeData(0x00);
            m_width = 135;
            m_height = 240;
            m_xOffset = 52;
            m_yOffset = 40;
            break;
        case Rotation::Landscape:
            writeData(0x70);
            m_width = 240;
            m_height = 135;
            m_xOffset = 40;
            m_yOffset = 53;
            break;
        case Rotation::PortraitMirror:
            writeData(0xC0);
            m_width = 135;
            m_height = 240;
            m_xOffset = 53;
            m_yOffset = 40;
            break;
        case Rotation::LandscapeMirror:
            writeData(0xA0);
            m_width = 240;
            m_height = 135;
            m_xOffset = 40;
            m_yOffset = 52;
            break;
    }
}

void St7789::setAddressWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    writeCmd(0x2A); // Column Address Set
    uint8_t data_x[] = {(uint8_t)((x1 + m_xOffset) >> 8), (uint8_t)((x1 + m_xOffset) & 0xFF), 
                        (uint8_t)((x2 + m_xOffset) >> 8), (uint8_t)((x2 + m_xOffset) & 0xFF)};
    writeData(data_x, 4);

    writeCmd(0x2B); // Row Address Set
    uint8_t data_y[] = {(uint8_t)((y1 + m_yOffset) >> 8), (uint8_t)((y1 + m_yOffset) & 0xFF), 
                        (uint8_t)((y2 + m_yOffset) >> 8), (uint8_t)((y2 + m_yOffset) & 0xFF)};
    writeData(data_y, 4);

    writeCmd(0x2C); // Memory Write
}

void St7789::fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if ((x >= m_width) || (y >= m_height)) return;
    if ((x + w) > m_width) w = m_width - x;
    if ((y + h) > m_height) h = m_height - y;

    // wait previous DMA
    if (m_dma_in_flight) {
        spi_transaction_t* rtrans;
        spi_device_get_trans_result(m_spi_handle, &rtrans, portMAX_DELAY);
        m_dma_in_flight = false;
    }

    setAddressWindow(x, y, x + w - 1, y + h - 1);

    uint8_t colorBytes[2] = {static_cast<uint8_t>(color >> 8), static_cast<uint8_t>(color & 0xFF)};
    uint8_t buf[128];
    for (int i = 0; i < 128; i += 2) {
        buf[i] = colorBytes[0];
        buf[i + 1] = colorBytes[1];
    }

    uint32_t totalPixels = (uint32_t)w * h;
    while (totalPixels > 0) {
        uint32_t chunk = (totalPixels > 64) ? 64 : totalPixels;
        writeData(buf, chunk * 2);
        totalPixels -= chunk;
    }
}

void St7789::drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data) {
    if ((x >= m_width) || (y >= m_height)) return;
    if ((x + w) > m_width) w = m_width - x;
    if ((y + h) > m_height) h = m_height - y;

    // wait previous DMA
    if (m_dma_in_flight) {
        spi_transaction_t* rtrans;
        spi_device_get_trans_result(m_spi_handle, &rtrans, portMAX_DELAY);
        m_dma_in_flight = false;
    }

    setAddressWindow(x, y, x + w - 1, y + h - 1);
    writeData((const uint8_t*)data, w * h * 2);
}

void St7789::flush_cb(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t* color_p) {
    // Wait for any previous DMA transaction to finish
    if (m_dma_in_flight) {
        spi_transaction_t* rtrans;
        spi_device_get_trans_result(m_spi_handle, &rtrans, portMAX_DELAY);
        m_dma_in_flight = false;
    }

    setAddressWindow(x1, y1, x2, y2);

    size_t len = (x2 - x1 + 1) * (y2 - y1 + 1) * 2;
    memset(&m_trans, 0, sizeof(spi_transaction_t));
    m_trans.length = len * 8; // length in bits
    m_trans.tx_buffer = color_p;
    m_trans.user = (void*)1; // DC = 1

    ESP_ERROR_CHECK(spi_device_queue_trans(m_spi_handle, &m_trans, portMAX_DELAY));
    m_dma_in_flight = true;
}

void St7789::writeCmd(uint8_t cmd) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &cmd;
    t.user = (void*)0; // DC = 0
    spi_device_polling_transmit(m_spi_handle, &t);
}

void St7789::writeData(uint8_t data) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &data;
    t.user = (void*)1; // DC = 1
    spi_device_polling_transmit(m_spi_handle, &t);
}

void St7789::writeData(const uint8_t* data, uint32_t len) {
    if (len == 0) return;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = len * 8;
    t.tx_buffer = data;
    t.user = (void*)1; // DC = 1
    spi_device_polling_transmit(m_spi_handle, &t);
}
