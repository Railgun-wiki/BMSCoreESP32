#include "ina226.hpp"

Ina226::Ina226(TwoWire* wire, uint8_t address7bit)
    : m_wire(wire), m_addr(address7bit), m_currentLsb_uA(0), m_powerLsb_uW(0) {
}

bool Ina226::init(uint32_t rShunt_uOhm, uint32_t maxCurrent_mA) {
    if (rShunt_uOhm == 0 || maxCurrent_mA == 0) {
        return false;
    }

    if (getManufacturerId() != 0x5449 || getDieId() != 0x2260) {
        return false;
    }

    if (!reset()) {
        return false;
    }
    delay(2);

    m_currentLsb_uA = (maxCurrent_mA * 1000) / 32768;
    if (m_currentLsb_uA == 0) m_currentLsb_uA = 1;

    uint64_t cal = 5120000000ULL / (static_cast<uint64_t>(m_currentLsb_uA) * rShunt_uOhm);
    if (!writeReg(REG_CALIB, static_cast<uint16_t>(cal))) {
        return false;
    }

    m_powerLsb_uW = 25 * m_currentLsb_uA;

    return configure(Averaging::Avg_1, ConvTime::Time_1100us, ConvTime::Time_1100us, Mode::ShuntBusContinuous);
}

bool Ina226::configure(Averaging avg, ConvTime busConvTime, ConvTime shuntConvTime, Mode mode) {
    uint16_t config = 0;
    config |= (static_cast<uint16_t>(avg) & 0x07) << 9;
    config |= (static_cast<uint16_t>(busConvTime) & 0x07) << 6;
    config |= (static_cast<uint16_t>(shuntConvTime) & 0x07) << 3;
    config |= (static_cast<uint16_t>(mode) & 0x07);
    config |= (1 << 14);

    return writeReg(REG_CONFIG, config);
}

bool Ina226::configureAlert(AlertFunction func, uint16_t limit, bool activeHigh, bool latchEnable) {
    uint16_t maskEn = static_cast<uint16_t>(func);
    if (activeHigh) maskEn |= (1 << 1);
    if (latchEnable) maskEn |= (1 << 0);

    if (!writeReg(REG_ALERT_LIM, limit)) return false;
    return writeReg(REG_MASK_EN, maskEn);
}

bool Ina226::reset() {
    return writeReg(REG_CONFIG, 0x8000);
}

int32_t Ina226::getBusVoltage_mV() {
    uint16_t raw;
    if (readReg(REG_BUS_VOLT, raw)) {
        return (static_cast<int32_t>(raw) * 125) / 100;
    }
    return 0;
}

int32_t Ina226::getShuntVoltage_uV() {
    uint16_t raw;
    if (readReg(REG_SHUNT_VOLT, raw)) {
        return (static_cast<int32_t>(static_cast<int16_t>(raw)) * 25) / 10;
    }
    return 0;
}

int32_t Ina226::getCurrent_mA() {
    uint16_t raw;
    if (readReg(REG_CURRENT, raw)) {
        return (static_cast<int32_t>(static_cast<int16_t>(raw)) * static_cast<int32_t>(m_currentLsb_uA)) / 1000;
    }
    return 0;
}

int32_t Ina226::getPower_mW() {
    uint16_t raw;
    if (readReg(REG_POWER, raw)) {
        return (static_cast<int32_t>(raw) * static_cast<int32_t>(m_powerLsb_uW)) / 1000;
    }
    return 0;
}

uint16_t Ina226::getManufacturerId() {
    uint16_t val = 0;
    readReg(REG_MANUF_ID, val);
    return val;
}

uint16_t Ina226::getDieId() {
    uint16_t val = 0;
    readReg(REG_DIE_ID, val);
    return val;
}

bool Ina226::writeReg(uint8_t reg, uint16_t value) {
    uint8_t data[2];
    data[0] = (value >> 8) & 0xFF;
    data[1] = value & 0xFF;

    m_wire->beginTransmission(m_addr);
    m_wire->write(reg);
    m_wire->write(data, 2);
    return m_wire->endTransmission() == 0;
}

bool Ina226::readReg(uint8_t reg, uint16_t& value) {
    m_wire->beginTransmission(m_addr);
    m_wire->write(reg);
    if (m_wire->endTransmission(false) != 0) {
        return false;
    }

    if (m_wire->requestFrom(m_addr, (uint8_t)2) != 2) {
        return false;
    }

    uint8_t data[2];
    data[0] = m_wire->read();
    data[1] = m_wire->read();
    value = (static_cast<uint16_t>(data[0]) << 8) | data[1];
    return true;
}
