#pragma once

#include <cstddef>
#include <cstdint>

namespace core {

// AT24C32-style I2C EEPROM (16-bit big-endian memory address, ack-polled write cycle).
// Bus provides: bool write(uint8_t dev, const uint8_t* data, size_t n)  (true = ACK),
//               bool read(uint8_t dev, uint8_t* out, size_t n), uint32_t millis().
// Every bus call is bounded, so an absent chip makes probe() false instead of hanging.
template <class Bus>
class I2cEeprom
{
  public:
    explicit I2cEeprom(Bus &bus, uint8_t devAddr = 0x50) : _bus(bus), _dev(devAddr) {}

    // True when a device at the address ACKs the address phase and a one-byte read at 0.
    // ponytail: any device at 0x50 passes (A0-A2 must be strapped low), no ID check.
    bool probe()
    {
        uint8_t v;
        return readByte(0, v);
    }

    // False on any bus failure; out is untouched then.
    bool tryRead(uint16_t addr, uint8_t &out) { return readByte(addr, out); }

    // Write only on change, then ack-poll (zero-length write) until the internal write
    // cycle ends. Returns false if the data write NACKs or the poll gives up after 10 ms
    // (datasheet worst case is 5 ms). A failed pre-read just forces the write.
    // ponytail: the 10 ms cap is checked between Wire calls, so on a hung bus one update
    // can take ~100 ms+ (Wire's own timeout per call). Upgrade: shorter Wire timeout.
    bool update(uint16_t addr, uint8_t value)
    {
        uint8_t cur;
        if (readByte(addr, cur) && cur == value)
            return true;
        const uint8_t frame[3] = {uint8_t(addr >> 8), uint8_t(addr & 0xFF), value};
        if (!_bus.write(_dev, frame, 3))
            return false;
        const uint32_t start = _bus.millis();
        do
        {
            if (_bus.write(_dev, nullptr, 0))
                return true;
        }
        while (uint32_t(_bus.millis() - start) < 10);
        return false;
    }

  private:
    bool readByte(uint16_t addr, uint8_t &out)
    {
        const uint8_t a[2] = {uint8_t(addr >> 8), uint8_t(addr & 0xFF)};
        return _bus.write(_dev, a, 2) && _bus.read(_dev, &out, 1);
    }

    Bus &_bus;
    uint8_t _dev;
};

}  // namespace core
