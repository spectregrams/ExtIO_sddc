#include <boost/test/unit_test.hpp>

#include "RadioHandler.h"
#include "mock_fx3.h"

#include <atomic>
#include <chrono>
#include <cstdint>

using namespace std::chrono_literals;

namespace {

struct CoreFixture {
  MockFX3Handler usb;
  RadioHandlerClass radio;

  std::atomic<uint32_t> callbackCount{0};
  std::atomic<uint64_t> totalSamples{0};

  CoreFixture() {
    // Pass this as context so the callback can update per-fixture counters
    // rather than sharing global state across tests.
    radio.Init(
        &usb,
        [](void *ctx, const float *, uint32_t len) {
          auto &self = *static_cast<CoreFixture *>(ctx);
          ++self.callbackCount;
          self.totalSamples += len;
        },
        nullptr, this);
  }
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(Core, CoreFixture)

// Each control property must be off/default after Init and immediately reflect
// any setter call.
BOOST_AUTO_TEST_CASE(BasicTest) {
  BOOST_REQUIRE_EQUAL(radio.getModel(), NORADIO);
  BOOST_REQUIRE_EQUAL(radio.getName(), "Dummy");

  BOOST_REQUIRE_EQUAL(radio.getSampleRate(), 64000000u);
  radio.UpdateSampleRate(128000000);
  BOOST_REQUIRE_EQUAL(radio.getSampleRate(), 128000000u);

  BOOST_REQUIRE_EQUAL(radio.GetDither(), false);
  radio.UptDither(true);
  BOOST_REQUIRE_EQUAL(radio.GetDither(), true);

  BOOST_REQUIRE_EQUAL(radio.GetRand(), false);
  radio.UptRand(true);
  BOOST_REQUIRE_EQUAL(radio.GetRand(), true);

  BOOST_REQUIRE_EQUAL(radio.GetPga(), false);
  radio.UptPga(true);
  BOOST_REQUIRE_EQUAL(radio.GetPga(), true);

  BOOST_REQUIRE_EQUAL(radio.GetBiasT_HF(), false);
  radio.UpdBiasT_HF(true);
  BOOST_REQUIRE_EQUAL(radio.GetBiasT_HF(), true);

  BOOST_REQUIRE_EQUAL(radio.GetBiasT_VHF(), false);
  radio.UpdBiasT_VHF(true);
  BOOST_REQUIRE_EQUAL(radio.GetBiasT_VHF(), true);
}

// At every decimation level the r2iq pipeline must deliver callbacks with the
// correct output block size.
BOOST_AUTO_TEST_CASE(R2IQTest, *boost::unit_test::label("integration")) {
  for (int decimate = 0; decimate < 5; ++decimate) {
    callbackCount.store(0);
    totalSamples.store(0);

    radio.Start(decimate);
    std::this_thread::sleep_for(1s);
    radio.Stop();

    const auto count = callbackCount.load();
    const auto total = totalSamples.load();

    BOOST_REQUIRE_GT(count, 0u);
    BOOST_REQUIRE_GT(total, 0u);
    BOOST_REQUIRE_EQUAL(total / count,
                        static_cast<uint64_t>(transferSamples / 2));

    BOOST_TEST_MESSAGE("decimate=" << decimate
                                   << " nxfers=" << usb.transferCount()
                                   << " count=" << count << " total=" << total);
    usb.resetTransferCount();
  }
}

// Sweeping TuneLO across the full HF band must not stall or crash the streaming
// pipeline.
BOOST_AUTO_TEST_CASE(TuneTest, *boost::unit_test::label("integration")) {
  callbackCount.store(0);

  radio.Start(1);

  for (uint64_t freq = 1'000; freq < 15'000'000; freq += 377'000) {
    radio.TuneLO(freq);
    std::this_thread::sleep_for(11ms);

    radio.Stop();

    BOOST_REQUIRE_GT(callbackCount.load(), 0u);
  }
}

BOOST_AUTO_TEST_SUITE_END()
