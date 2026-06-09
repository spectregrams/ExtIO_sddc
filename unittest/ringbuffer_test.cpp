#include <boost/test/unit_test.hpp>

#include "dsp/ringbuffer.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>

using namespace std::chrono_literals;

BOOST_AUTO_TEST_SUITE(RingBuffer)

// Data written to the write slot must be visible at the read slot after
// WriteDone/getReadPtr.
BOOST_AUTO_TEST_CASE(BasicTest) {
  ringbuffer<int16_t> buf{128};
  buf.setBlockSize(128 * 1024);

  BOOST_REQUIRE_EQUAL(buf.getBlockSize(), 128 * 1024);

  std::memset(buf.getWritePtr(), 0x5A, buf.getBlockSize() * sizeof(int16_t));
  buf.WriteDone();

  const auto *rptr = buf.getReadPtr();
  BOOST_REQUIRE_EQUAL(*rptr, static_cast<int16_t>(0x5a5a));
  // Check a non-trivial offset to confirm the fill extends beyond the first
  // element.
  BOOST_REQUIRE_EQUAL(*(rptr + 0x100), static_cast<int16_t>(0x5a5a));
}

// Concurrent producer/consumer must not corrupt data or deadlock (small ring:
// capacity=2).
BOOST_AUTO_TEST_CASE(TwoThreadsTest) {
  ringbuffer<int16_t> buffer{2};
  buffer.setBlockSize(1024);

  auto writer = std::thread([&buffer] {
    for (int i = 0; i < 1000; ++i) {
      std::memset(buffer.getWritePtr(), 0x5A, buffer.getBlockSize());
      buffer.WriteDone();
    }
  });

  auto reader = std::thread([&buffer] {
    for (int i = 0; i < 1000; ++i) {
      BOOST_REQUIRE_EQUAL(*buffer.getReadPtr(), static_cast<int16_t>(0x5A5A));
      buffer.ReadDone();
    }
  });

  writer.join();
  reader.join();
}

// Same as TwoThreadsTest with a larger ring (capacity=128, the typical
// production size).
BOOST_AUTO_TEST_CASE(TwoThreadsLargeBufferTest) {
  ringbuffer<int16_t> buffer{128};
  buffer.setBlockSize(1024);

  auto writer = std::thread([&buffer] {
    for (int i = 0; i < 1000; ++i) {
      std::memset(buffer.getWritePtr(), 0x5A, buffer.getBlockSize());
      buffer.WriteDone();
    }
  });

  auto reader = std::thread([&buffer] {
    for (int i = 0; i < 1000; ++i) {
      BOOST_REQUIRE_EQUAL(*buffer.getReadPtr(), static_cast<int16_t>(0x5A5A));
      buffer.ReadDone();
    }
  });

  writer.join();
  reader.join();
}

// Stop() must unblock threads indefinitely blocked in getReadPtr() so that
// join() completes.
BOOST_AUTO_TEST_CASE(StopTest) {
  std::atomic<bool> running{true};
  ringbuffer<int16_t> buffer{128};
  buffer.setBlockSize(1024);

  auto makeReader = [&] {
    return std::thread([&buffer, &running] {
      while (running.load()) {
        buffer.getReadPtr();
        if (!running.load())
          break;
        buffer.ReadDone();
      }
    });
  };

  auto r1 = makeReader();
  auto r2 = makeReader();

  std::this_thread::sleep_for(1s);
  running.store(false);
  buffer.Stop();

  r1.join();
  r2.join();
}

// peekWritePtr(0)/peekReadPtr(0) must alias the active slot; peek(-1) must
// alias the last committed slot.
BOOST_AUTO_TEST_CASE(PeekTest) {
  ringbuffer<int16_t> buffer{128};

  const auto *wptr0 = buffer.getWritePtr();
  BOOST_CHECK_EQUAL(wptr0, buffer.peekWritePtr(0));
  buffer.WriteDone();
  BOOST_CHECK_EQUAL(wptr0, buffer.peekWritePtr(-1));

  const auto *rptr0 = buffer.getReadPtr();
  BOOST_CHECK_EQUAL(rptr0, buffer.peekReadPtr(0));
  buffer.ReadDone();
  BOOST_CHECK_EQUAL(rptr0, buffer.peekReadPtr(-1));
}

BOOST_AUTO_TEST_SUITE_END()
