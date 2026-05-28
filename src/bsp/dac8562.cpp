#include "dac8562.hpp"
#include <algorithm>

Dac8562::Dac8562(SPIClass* spi, int syncPin)
    : m_spi(spi), m_syncPin(syncPin) {
}

bool Dac8562::init() {
    pinMode(m_syncPin, OUTPUT);
    digitalWrite(m_syncPin, HIGH);

    reset();

    if (!setInternalRef(true)) {
        return false;
    }

    return setGain(2, 2);
}

void Dac8562::writeValue(Channel channel, uint16_t value) {
    transmit(Command::WriteInputRegUpdateN, channel, value);
}

void Dac8562::setVoltage(Channel channel, float voltage, float vRef) {
    float scaled = (voltage / vRef) * 65535.0f;
    uint16_t value = static_cast<uint16_t>(std::clamp(scaled, 0.0f, 65535.0f));
    writeValue(channel, value);
}

bool Dac8562::setInternalRef(bool enable) {
    transmit(Command::InternalRefConfig, Channel::DAC_A, enable ? 0x0001 : 0x0000);
    return true;
}

bool Dac8562::setGain(uint8_t gainA, uint8_t gainB) {
    uint16_t data = 0;
    if (gainA == 1) data |= 0x0001;
    if (gainB == 1) data |= 0x0002;

    transmit(Command::WriteInputReg, static_cast<Channel>(2), data);
    return true;
}

void Dac8562::reset() {
    transmit(Command::SoftwareReset, Channel::DAC_A, 0x0001);
}

void Dac8562::transmit(Command cmd, Channel addr, uint16_t data) {
    uint8_t buffer[3];
    buffer[0] = (static_cast<uint8_t>(cmd) << 3) | (static_cast<uint8_t>(addr) & 0x07);
    buffer[1] = (data >> 8) & 0xFF;
    buffer[2] = data & 0xFF;

    digitalWrite(m_syncPin, LOW);
    m_spi->transfer(buffer, 3);
    digitalWrite(m_syncPin, HIGH);
}
