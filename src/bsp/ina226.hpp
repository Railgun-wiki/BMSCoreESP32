#pragma once

#include <cstdint>
#include <Wire.h>

class Ina226 {
public:
    enum class Mode : uint16_t {
        PowerDown = 0,
        ShuntTriggered = 1,
        BusTriggered = 2,
        ShuntBusTriggered = 3,
        PowerDown2 = 4,
        ShuntContinuous = 5,
        BusContinuous = 6,
        ShuntBusContinuous = 7
    };

    enum class ConvTime : uint16_t {
        Time_140us = 0,
        Time_204us = 1,
        Time_332us = 2,
        Time_588us = 3,
        Time_1100us = 4,
        Time_2116us = 5,
        Time_4156us = 6,
        Time_8244us = 7
    };

    enum class Averaging : uint16_t {
        Avg_1 = 0,
        Avg_4 = 1,
        Avg_16 = 2,
        Avg_64 = 3,
        Avg_128 = 4,
        Avg_256 = 5,
        Avg_512 = 6,
        Avg_1024 = 7
    };

    enum class AlertFunction : uint16_t {
        None = 0,
        ShuntOverVoltage = 0x8000,
        ShuntUnderVoltage = 0x4000,
        BusOverVoltage = 0x2000,
        BusUnderVoltage = 0x1000,
        PowerOverLimit = 0x0800,
        ConversionReady = 0x0400
    };

    Ina226(TwoWire* wire, uint8_t address7bit);

    bool init(uint32_t rShunt_uOhm, uint32_t maxCurrent_mA);
    bool configure(Averaging avg, ConvTime busConvTime, ConvTime shuntConvTime, Mode mode);
    bool configureAlert(AlertFunction func, uint16_t limit, bool activeHigh = false, bool latchEnable = false);
    bool reset();

    int32_t getBusVoltage_mV();
    int32_t getShuntVoltage_uV();
    int32_t getCurrent_mA();
    int32_t getPower_mW();

    uint16_t getManufacturerId();
    uint16_t getDieId();

private:
    TwoWire* m_wire;
    uint8_t  m_addr;

    uint32_t m_currentLsb_uA;
    uint32_t m_powerLsb_uW;

    bool writeReg(uint8_t reg, uint16_t value);
    bool readReg(uint8_t reg, uint16_t& value);

    static constexpr uint8_t REG_CONFIG     = 0x00;
    static constexpr uint8_t REG_SHUNT_VOLT = 0x01;
    static constexpr uint8_t REG_BUS_VOLT   = 0x02;
    static constexpr uint8_t REG_POWER      = 0x03;
    static constexpr uint8_t REG_CURRENT    = 0x04;
    static constexpr uint8_t REG_CALIB      = 0x05;
    static constexpr uint8_t REG_MASK_EN    = 0x06;
    static constexpr uint8_t REG_ALERT_LIM  = 0x07;
    static constexpr uint8_t REG_MANUF_ID   = 0xFE;
    static constexpr uint8_t REG_DIE_ID     = 0xFF;
};
