enable_testing()

add_subdirectory(GoogleTest)

add_executable(
  test
  resource_test.cpp
  pool/pool_impl_test.cpp
)
target_link_libraries(
  test
  CrystalMem
  GTest::gtest_main
)
include(GoogleTest)
gtest_discover_tests(test)
