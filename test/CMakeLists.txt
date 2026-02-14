enable_testing()

add_subdirectory(GoogleTest)

add_executable(
  test
  test.cpp
)
target_link_libraries(
  test
  CrystalMem
  GTest::gtest_main
)
include(GoogleTest)
gtest_discover_tests(test)
