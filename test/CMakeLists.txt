enable_testing()

add_subdirectory(GoogleTest)

add_executable(
  test
  test.cpp # Keep basic test.cpp for now
  pool/test_safe_best_fit.cpp # Add safe_best_fit pool tests
  pool/test_slub.cpp
)
target_include_directories(test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}) # Add include directory for mocks
target_link_libraries(
  test
  CrystalMem
  GTest::gtest_main
  gmock
)
include(GoogleTest)
gtest_discover_tests(test)
