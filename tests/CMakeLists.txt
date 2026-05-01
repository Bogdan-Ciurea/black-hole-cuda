# Auto-collect all test_*.cpp files
file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp")

add_executable(bht_tests ${TEST_SOURCES})

target_link_libraries(bht_tests
  PRIVATE
    BlackHoleEngine
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(bht_tests)
