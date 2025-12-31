#include <gtest/gtest.h>
#include <cstring>
#include <CrystalMem/resource.h>
#include <CrystalMem/types.h>
#include <CrystalMem/options.h>
#include <CrystalMem/pool.h>

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

// --- Pool Tests ---

class MockResource {
public:
  size_t alloc_count = 0;
  size_t dealloc_count = 0;
  
  void* Alloc(size_t size, align_t align) {
    alloc_count++;
    return operator new(size, align);
  }

  void Dealloc(void* ptr, size_t size) {
    dealloc_count++;
    operator delete(ptr, size);
  }
};

TEST(PoolTest, BasicAllocation) {
  MockResource mock_res;
  // Use a small block size to force block allocations more easily if needed,
  // but for basic allocation 4096 is fine.
  Pool<4096, MockResource> pool(mock_res);

  int* p = pool.New<int>(42);
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(*p, 42);

  // Pool should have allocated at least one block from resource
  EXPECT_EQ(mock_res.alloc_count, 1);
  EXPECT_EQ(mock_res.dealloc_count, 0);

  pool.Del(p);
  // Deleting object doesn't free the block back to resource immediately
  EXPECT_EQ(mock_res.dealloc_count, 0);

  pool.Close();
  // Closing pool should free the block
  EXPECT_EQ(mock_res.dealloc_count, 1);
}

struct TestObject {
  int x;
  double y;
  TestObject(int val_x, double val_y) : x(val_x), y(val_y) {}
  ~TestObject() { x = -1; } // Simple side effect
};

TEST(PoolTest, ObjectConstructionDestruction) {
  MockResource mock_res;
  Pool<4096, MockResource> pool(mock_res);

  TestObject* p = pool.New<TestObject>(10, 3.14);
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(p->x, 10);
  EXPECT_EQ(p->y, 3.14);

  pool.Del(p);
  // We can't easily verify the destructor ran by checking memory since it's freed,
  // but if we had a side-effect tracking mechanism we could.
  // For now, ensuring it compiles and runs without error is the baseline.
}

TEST(PoolTest, MultipleAllocations) {
  MockResource mock_res;
  Pool<4096, MockResource> pool(mock_res);

  std::vector<int*> ptrs;
  for (int i = 0; i < 100; ++i) {
    ptrs.push_back(pool.New<int>(i));
  }

  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(*ptrs[i], i);
  }

  for (auto* p : ptrs) {
    pool.Del(p);
  }
}

TEST(PoolTest, BlockExhaustion) {
  MockResource mock_res;
  // Small block size: 128 bytes.
  // Slot size for int (4 bytes) -> bucket size 4.
  // Actually min bucket size is 4.
  // Overhead: free_head (4 bytes usually).
  // Block<4>::kCapacity = (128 - 4) / 4 = 31 slots.
  constexpr size_t kBlockSize = 128;
  Pool<kBlockSize, MockResource> pool(mock_res);

  // Allocate more than 31 items to force a second block allocation.
  std::vector<int*> ptrs;
  for (int i = 0; i < 35; ++i) {
    ptrs.push_back(pool.New<int>(i));
  }

  // Expect at least 2 blocks allocated
  EXPECT_GE(mock_res.alloc_count, 2);

  pool.Close();
  EXPECT_EQ(mock_res.dealloc_count, mock_res.alloc_count);
}