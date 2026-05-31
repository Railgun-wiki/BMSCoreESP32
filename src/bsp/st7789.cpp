#include "st7789.hpp"
#include <Arduino.h>

St7789::St7789(SPIClass* spi, int csPin, int dcPin, int rstPin, int blkPin)
    : m_spi(spi), m_csPin(csPin), m_dcPin(dcPin), m_rstPin(rstPin), m_blkPin(blkPin),
      m_width(135), m_height(240), m_xOffset(52), m_yOffset(40) {
}

void St7789::init() {
    pinMode(m_csPin, OUTPUT);
    pinMode(m_dcPin, OUTPUT);
    pinMode(m_rstPin, OUTPUT);
    pinMode(m_blkPin, OUTPUT);

    // Hardware Reset
    digitalWrite(m_rstPin, HIGH);
    delay(10);
    digitalWrite(m_rstPin, LOW);
    delay(20);
    digitalWrite(m_rstPin, HIGH);
    delay(120);

    digitalWrite(m_csPin, LOW);

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
    writeData((x1 + m_xOffset) >> 8);
    writeData((x1 + m_xOffset) & 0xFF);
    writeData((x2 + m_xOffset) >> 8);
    writeData((x2 + m_xOffset) & 0xFF);

    writeCmd(0x2B); // Row Address Set
    writeData((y1 + m_yOffset) >> 8);
    writeData((y1 + m_yOffset) & 0xFF);
    writeData((y2 + m_yOffset) >> 8);
    writeData((y2 + m_yOffset) & 0xFF);

    writeCmd(0x2C); // Memory Write
}

void St7789::fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if ((x >= m_width) || (y >= m_height)) return;
    if ((x + w) > m_width) w = m_width - x;
    if ((y + h) > m_height) h = m_height - y;

    setAddressWindow(x, y, x + w - 1, y + h - 1);

    uint8_t colorBytes[2] = {static_cast<uint8_t>(color >> 8), static_cast<uint8_t>(color & 0xFF)};

    m_spi->beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
    digitalWrite(m_dcPin, HIGH);
    uint32_t totalPixels = (uint32_t)w * h;
    uint8_t buf[128];
    for (int i = 0; i < 128; i += 2) {
        buf[i] = colorBytes[0];
        buf[i + 1] = colorBytes[1];
    }
    while (totalPixels > 0) {
        uint32_t chunk = (totalPixels > 64) ? 64 : totalPixels;
        m_spi->transfer(buf, chunk * 2);
        totalPixels -= chunk;
    }
    m_spi->endTransaction();
}

void St7789::drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data) {
    if ((x >= m_width) || (y >= m_height)) return;
    if ((x + w) > m_width) w = m_width - x;
    if ((y + h) > m_height) h = m_height - y;

    setAddressWindow(x, y, x + w - 1, y + h - 1);

    m_spi->beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
    digitalWrite(m_dcPin, HIGH);
    m_spi->transfer((uint8_t*)data, w * h * 2);
    m_spi->endTransaction();
}

void St7789::flush_cb(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t* color_p) {
    uint16_t w = x2 - x1 + 1;
    uint16_t h = y2 - y1 + 1;

    setAddressWindow(x1, y1, x2, y2);

    m_spi->beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
    digitalWrite(m_dcPin, HIGH);
    m_spi->transfer((uint8_t*)color_p, w * h * 2);
    m_spi->endTransaction();
}

void St7789::writeCmd(uint8_t cmd) {
    m_spi->beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
    digitalWrite(m_dcPin, LOW);
    m_spi->transfer(cmd);
    m_spi->endTransaction();
}

void St7789::writeData(uint8_t data) {
    m_spi->beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
    digitalWrite(m_dcPin, HIGH);
    m_spi->transfer(data);
    m_spi->endTransaction();
}

void St7789::writeData(const uint8_t* data, uint32_t len) {
    m_spi->beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
    digitalWrite(m_dcPin, HIGH);
    m_spi->transfer((uint8_t*)data, len);
    m_spi->endTransaction();
}
