#pragma once

#include "config.h"
#include "FX3Class.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>

// Simulates the FX3 USB bridge by feeding zeroed blocks at ~1 kHz.
// Reuse this in any test suite that needs a RadioHandlerClass or SoapySDDC
// instance without a physical device attached.
class MockFX3Handler final : public fx3class
{
public:
    long transferCount() const { return nxfers_.load(); }
    void resetTransferCount()  { nxfers_.store(0); }

private:
    bool Open()                            override { return true; }
    bool Control(FX3Command, uint8_t)      override { return true; }
    bool Control(FX3Command, uint32_t)     override { return true; }
    bool Control(FX3Command, uint64_t)     override { return true; }
    bool SetArgument(uint16_t, uint16_t)   override { return true; }
    bool Enumerate(unsigned char&, char*)  override { return true; }
    bool ReadDebugTrace(uint8_t*, uint8_t) override { return true; }

    bool GetHardwareInfo(uint32_t* data) override
    {
        const uint8_t d[4] = { 0, FIRMWARE_VER_MAJOR, FIRMWARE_VER_MINOR, 0 };
        std::memcpy(data, d, sizeof(*data));
        return true;
    }

    void StartStream(ringbuffer<int16_t>& input, int) override
    {
        input.setBlockSize(transferSamples);
        run_.store(true);
        streamThread_ = std::thread([&input, this] {
            while (run_.load()) {
                std::memset(input.getWritePtr(), 0x5A, input.getWriteCount());
                input.WriteDone();
                ++nxfers_;
                std::this_thread::sleep_for(std::chrono::milliseconds{1});
            }
        });
    }

    void StopStream() override
    {
        run_.store(false);
        streamThread_.join();
    }

    std::thread       streamThread_;
    std::atomic<bool> run_{false};
    std::atomic<long> nxfers_{0};
};
