#pragma once

#include <cstdint>
#include <SPI.h>

class Dac8562 {
public:
    enum class Channel : uint8_t {
        DAC_A = 0,
        DAC_B = 1,
        BOTH  = 7
    };

    enum class Command : uint8_t {
        WriteInputReg         = 0x00,
        UpdateDac             = 0x01,
        WriteInputRegUpdateAll = 0x02,
        WriteInputRegUpdateN   = 0x03,
        PowerDown             = 0x04,
        SoftwareReset         = 0x05,
        LdacConfig            = 0x06,
        InternalRefConfig     = 0x07
    };

    enum class PowerMode : uint8_t {
        Normal      = 0,
        Down_1k     = 1,
        Down_100k   = 2,
        Down_HiZ    = 3
    };

    Dac8562(SPIClass* spi, int syncPin);

    bool init();
    void writeValue(Channel channel, uint16_t value);
    void setVoltage(Channel channel, float voltage, float vRef = 5.0f);
    bool setInternalRef(bool enable);
    bool setGain(uint8_t gainA, uint8_t gainB);
    void reset();

private:
    SPIClass* m_spi;
    int m_syncPin;

    void transmit(Command cmd, Channel addr, uint16_t data);
};
