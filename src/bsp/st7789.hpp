#pragma once

#include <cstdint>
#include <SPI.h>

class St7789 {
public:
    enum class Rotation {
        Portrait = 0,
        Landscape = 1,
        PortraitMirror = 2,
        LandscapeMirror = 3
    };

    St7789(SPIClass* spi, int csPin, int dcPin, int rstPin, int blkPin);

    void init();
    void setRotation(Rotation rotation);
    void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
    void drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data);

    void flush_cb(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t* color_p);

    uint16_t getWidth() const { return m_width; }
    uint16_t getHeight() const { return m_height; }

private:
    SPIClass* m_spi;
    int m_csPin;
    int m_dcPin;
    int m_rstPin;
    int m_blkPin;
    uint16_t m_width;
    uint16_t m_height;
    uint16_t m_xOffset;
    uint16_t m_yOffset;

    void writeCmd(uint8_t cmd);
    void writeData(uint8_t data);
    void writeData(const uint8_t* data, uint32_t len);
    void setAddressWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
};
