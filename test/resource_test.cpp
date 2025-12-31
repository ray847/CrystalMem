#include <gtest/gtest.h>
#include <cstring>
#include <CrystalMem/resource.h>
#include <CrystalMem/types.h>
#include <CrystalMem/options.h>

using namespace crystal::mem;

// Test the default Resource template implementation (fallback)
// using Source::File which currently has no specialization in the build.
TEST(ResourceTest, DefaultResourceLifecycle) {
  Resource<Source::File> res;
  
  // Test validity
  EXPECT_TRUE(static_cast<bool>(res));
  
  // Test Allocation
  size_t size = 1024;
  align_t align = static_cast<align_t>(16);
  void* ptr = res.Alloc(size, align);
  
  EXPECT_NE(ptr, nullptr);
  
  // Verify write access
  std::memset(ptr, 0xCC, size);
  EXPECT_EQ(*static_cast<unsigned char*>(ptr), 0xCC);

  // Test Deallocation (should not crash)
  res.Dealloc(ptr, size);
  
  // Test Close
  auto close_result = res.Close();
  EXPECT_TRUE(close_result.has_value());
  EXPECT_FALSE(static_cast<bool>(res));
}

TEST(ResourceTest, DestructorCleanup) {
  // Verify that the destructor runs without crashing
  auto* res = new Resource<Source::File>();
  EXPECT_TRUE(static_cast<bool>(*res));
  delete res;
}
