find_package(Boost REQUIRED COMPONENTS unit_test_framework)

file(GLOB UNITTEST_SOURCES "*.cpp")

add_executable(unittest ${UNITTEST_SOURCES})

target_include_directories(unittest
    PRIVATE
        "${LIBFFTW_INCLUDE_DIRS}"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Core"
)

target_link_directories(unittest PRIVATE "${LIBFFTW_LIBRARY_DIRS}")

target_compile_definitions(unittest PRIVATE BOOST_TEST_DYN_LINK)

target_link_libraries(unittest
    PRIVATE
    SDDC_CORE
    Boost::unit_test_framework
    ${ASANLIB}
)

if (NOT MSVC)
    target_link_libraries(unittest PRIVATE pthread)
endif()

# Fast tests – safe to run in CI with no device attached.
add_test(
    NAME    unit
    COMMAND unittest --run_test=!@integration --log_level=message
)

add_test(
    NAME    integration
    COMMAND unittest --run_test=@integration --log_level=message
)
