#include <gtest/gtest.h>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "core/storage/I2cEeprom.hpp"

namespace {

struct Frame
{
    uint8_t dev;
    std::vector<uint8_t> bytes;
};

// Fake bus: records writes, serves a fixed read byte, ACKs the first ackFirst writes, then NACKs
// nackCount writes (zero-length ack polls included), and advances the clock by 1 ms per millis() call.
struct FakeBus
{
    std::vector<Frame> writes;
    std::vector<size_t> reads;
    uint8_t readValue = 0;
    bool readOk = true;
    int ackFirst = 0;
    int nackCount = 0;
    bool alwaysNack = false;
    uint32_t now = 0;

    bool write(uint8_t dev, const uint8_t *data, size_t n)
    {
        writes.push_back({dev, std::vector<uint8_t>(data, data + n)});
        if (ackFirst > 0)
        {
            --ackFirst;
            return true;
        }
        if (alwaysNack)
            return false;
        if (nackCount > 0)
        {
            --nackCount;
            return false;
        }
        return true;
    }
    bool read(uint8_t, uint8_t *out, size_t n)
    {
        reads.push_back(n);
        if (!readOk)
            return false;
        for (size_t i = 0; i < n; i++)
            out[i] = readValue;
        return true;
    }
    uint32_t millis() { return now++; }
};

}  // namespace

TEST(I2cEepromTest, ReadSendsBigEndianAddress)
{
    FakeBus bus;
    bus.readValue = 0x5A;
    core::I2cEeprom<FakeBus> eeprom(bus);
    uint8_t v = 0;
    ASSERT_TRUE(eeprom.tryRead(0x0123, v));
    EXPECT_EQ(0x5A, v);
    ASSERT_EQ(1u, bus.writes.size());
    EXPECT_EQ(0x50, bus.writes[0].dev);
    EXPECT_EQ((std::vector<uint8_t>{0x01, 0x23}), bus.writes[0].bytes);
    ASSERT_EQ(1u, bus.reads.size());
    EXPECT_EQ(1u, bus.reads[0]);
}

TEST(I2cEepromTest, UpdateSkipsWriteWhenUnchanged)
{
    FakeBus bus;
    bus.readValue = 0x42;
    core::I2cEeprom<FakeBus> eeprom(bus);
    eeprom.update(0x0010, 0x42);
    ASSERT_EQ(1u, bus.writes.size());  // only the read's address phase
    EXPECT_EQ(2u, bus.writes[0].bytes.size());
}

TEST(I2cEepromTest, UpdateWritesAddressAndValue)
{
    FakeBus bus;
    bus.readValue = 0x00;
    core::I2cEeprom<FakeBus> eeprom(bus, 0x57);
    eeprom.update(0x0123, 0x99);
    ASSERT_GE(bus.writes.size(), 2u);
    EXPECT_EQ(0x57, bus.writes[1].dev);
    EXPECT_EQ((std::vector<uint8_t>{0x01, 0x23, 0x99}), bus.writes[1].bytes);
}

TEST(I2cEepromTest, UpdatePollsUntilAck)
{
    FakeBus bus;
    bus.readValue = 0x00;
    bus.ackFirst = 2;  // read address phase + data write ACK, then the device is busy
    bus.nackCount = 2;
    core::I2cEeprom<FakeBus> eeprom(bus);
    eeprom.update(0x0001, 0x77);
    ASSERT_EQ(5u, bus.writes.size());  // 2 + 3 polls (NACK, NACK, ACK)
    for (size_t i = 2; i < 5; i++)
        EXPECT_EQ(0u, bus.writes[i].bytes.size());
}

TEST(I2cEepromTest, ProbeFalseOnNack)
{
    FakeBus bus;
    bus.alwaysNack = true;
    core::I2cEeprom<FakeBus> eeprom(bus);
    EXPECT_FALSE(eeprom.probe());
}

TEST(I2cEepromTest, ProbeTrueOnAck)
{
    FakeBus bus;
    core::I2cEeprom<FakeBus> eeprom(bus);
    EXPECT_TRUE(eeprom.probe());
}

TEST(I2cEepromTest, UpdateGivesUpAfterTenMilliseconds)
{
    FakeBus bus;
    bus.readValue = 0x00;
    bus.ackFirst = 2;
    bus.alwaysNack = true;  // device never finishes the write cycle
    core::I2cEeprom<FakeBus> eeprom(bus);
    EXPECT_FALSE(eeprom.update(0x0001, 0x77));
    // fake clock ticks 1 ms per millis() call: it must poll, and must stop at ~10 ms
    EXPECT_GE(bus.writes.size(), 2u + 5u);
    EXPECT_LE(bus.writes.size(), 2u + 12u);
}

TEST(I2cEepromTest, TryReadFalseOnAddressNack)
{
    FakeBus bus;
    bus.alwaysNack = true;
    core::I2cEeprom<FakeBus> eeprom(bus);
    uint8_t v = 0x11;
    EXPECT_FALSE(eeprom.tryRead(0x0001, v));
    EXPECT_EQ(0x11, v);
}

TEST(I2cEepromTest, TryReadFalseOnReadFailure)
{
    FakeBus bus;
    bus.readOk = false;
    core::I2cEeprom<FakeBus> eeprom(bus);
    uint8_t v = 0x11;
    EXPECT_FALSE(eeprom.tryRead(0x0001, v));
}

TEST(I2cEepromTest, ProbeFalseOnReadFailure)
{
    FakeBus bus;
    bus.readOk = false;
    core::I2cEeprom<FakeBus> eeprom(bus);
    EXPECT_FALSE(eeprom.probe());
}

TEST(I2cEepromTest, UpdateWritesDespiteFailedPreRead)
{
    FakeBus bus;
    bus.readOk = false;
    core::I2cEeprom<FakeBus> eeprom(bus);
    EXPECT_TRUE(eeprom.update(0x0123, 0x99));
    ASSERT_GE(bus.writes.size(), 2u);
    EXPECT_EQ((std::vector<uint8_t>{0x01, 0x23, 0x99}), bus.writes[1].bytes);
}

TEST(I2cEepromTest, UpdateFalseWhenDataWriteNacks)
{
    FakeBus bus;
    bus.ackFirst = 1;  // pre-read address phase ACKs, data write NACKs
    bus.alwaysNack = true;
    core::I2cEeprom<FakeBus> eeprom(bus);
    EXPECT_FALSE(eeprom.update(0x0001, 0x77));
    EXPECT_EQ(2u, bus.writes.size());  // no ack-poll after a failed data write
}

TEST(I2cEepromTest, UpdateTrueWhenUnchanged)
{
    FakeBus bus;
    bus.readValue = 0x42;
    core::I2cEeprom<FakeBus> eeprom(bus);
    EXPECT_TRUE(eeprom.update(0x0010, 0x42));
}
