#include <boost/test/unit_test.hpp>

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.h>

#include <filesystem>
#include <string>

// Ensure FFTW wisdom is exported to disk on device creation.
BOOST_AUTO_TEST_CASE(WisdomExported, *boost::unit_test::label("integration")) {
  // Create a temporary filesystem, since we're writing to disk.
  std::filesystem::path wisdom_filename =
      std::filesystem::temp_directory_path() / "ExtIO_sddc" / "foo.wisdom";

  // Create the device and check wisdom was exported.
  SoapySDR::Kwargs kwargs = {{"driver", "SDDC"},
                             {"wisdom_filename", wisdom_filename.string()}};
  SoapySDR::Device *dev = SoapySDR::Device::make(kwargs);
  BOOST_REQUIRE(dev != nullptr);
  BOOST_CHECK(std::filesystem::exists(wisdom_filename));

  // Clean-up.
  SoapySDR::Device::unmake(dev);
  std::filesystem::remove_all(wisdom_filename.parent_path());
}

// `buffers` must be declared as a supported stream arg.
BOOST_AUTO_TEST_CASE(BuffersStreamArgDeclared, *boost::unit_test::label("integration")) {
  SoapySDR::Kwargs kwargs = {{"driver", "SDDC"}};
  SoapySDR::Device *dev = SoapySDR::Device::make(kwargs);
  BOOST_REQUIRE(dev != nullptr);

  const auto args = dev->getStreamArgsInfo(SOAPY_SDR_RX, 0);

  const auto it = std::find_if(args.begin(), args.end(),
      [](const SoapySDR::ArgInfo &a) { return a.key == "buffers"; });

  BOOST_REQUIRE_MESSAGE(it != args.end(), "'buffers' not found in stream args");
  BOOST_CHECK_EQUAL(it->type, SoapySDR::ArgInfo::INT);

  SoapySDR::Device::unmake(dev);
}