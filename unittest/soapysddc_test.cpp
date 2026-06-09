#include <boost/test/unit_test.hpp>

#include <SoapySDR/Device.hpp>

#include <filesystem>

// Ensure FFTW wisdom is exported to disk on device creation.
BOOST_AUTO_TEST_CASE(WisdomExported, *boost::unit_test::label("integration")) {
  // Create a temporary filesystem, since we're writing to disk.
  std::filesystem::path wisdom_filename =
      std::filesystem::temp_directory_path() / "ExtIO_sddc" / "foo.wisdom";

  // Create the device and check wisdom was exported.
  SoapySDR::Kwargs kwargs = {{"driver", "SDDC"},
                             {"wisdom_filename", wisdom_filename.string()}};
  SoapySDR::Device *dev = SoapySDR::Device::make(kwargs);
  BOOST_CHECK(std::filesystem::exists(wisdom_filename));

  // Clean-up.
  SoapySDR::Device::unmake(dev);
  std::filesystem::remove_all(wisdom_filename.parent_path());
}