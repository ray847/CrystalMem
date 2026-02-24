#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "CrystalMem/pool/slub/slub.h"
#include "CrystalMem/type.h"
#include "../mock_pool.h"

namespace crystal::mem {

using ::testing::_;
using ::testing::Return;
using ::testing::AtLeast;

class SLUBPoolTest : public ::testing::Test {
 protected:
  MockVendorConceptSatisfier mock_vendor_satisfier;
  RealMockVendor& real_mock_vendor = mock_vendor_satisfier.get_real_mock();

  // Define a small SLUBPool for testing
  // We use a small block size to trigger block allocations easily if needed
  // and a few slot sizes.
  static constexpr size_t kTestBlockSize = 1024;
  
  // Note: Using a brace-enclosed list for integer_sequence as seen in slub.h's static_assert
  using TestPool = SLUBPool<kTestBlockSize, { 16_B, 32_B, 64_B }, MockVendorConceptSatisfier>;

  void SetUp() override {
    // Handle internal logic allocations (e.g., for unordered_map)
    ON_CALL(real_mock_vendor, MockAlloc(_, _))
        .WillByDefault([](size_t size, align_t align) {
          return _aligned_malloc(size, static_cast<size_t>(align));
        });
    ON_CALL(real_mock_vendor, MockDealloc(_, _, _))
        .WillByDefault([](void* ptr, size_t size, align_t align) {
          _aligned_free(ptr);
        });
  }
};

TEST_F(SLUBPoolTest, ConstructorAndDestructor) {
  // Clear() is called in destructor. 
  // If no allocations were made, nothing should be deallocated.
  {
    TestPool pool(mock_vendor_satisfier);
  }
}

TEST_F(SLUBPoolTest, DiscreteAllocSmallObject) {
  TestPool pool(mock_vendor_satisfier);

  // Allocating a 16-byte object should use the 16-byte bucket.
  // The bucket will allocate a block from the vendor.
  // sizeof(SLUBBlockNode<1024, 16>) should be exactly 1024 or less.
  
  EXPECT_CALL(real_mock_vendor, MockAlloc(kTestBlockSize, _))
      .WillOnce([](size_t size, align_t align) {
        // We need to return memory that is aligned to kTestBlockSize
        // because SLUBBlockNode::FromSlot uses bit masking based on block size.
        void* ptr = _aligned_malloc(size, static_cast<size_t>(align));
        return ptr;
      });

  int* ptr = pool.DiscreteAlloc<int>(); // sizeof(int) is 4, should fit in 16-byte bucket
  ASSERT_NE(ptr, nullptr);

  // Deallocate should not call vendor yet, as it just returns to the bucket
  pool.DiscreteDealloc(ptr);

  // Cleanup: The pool destructor will call Clear(), which should deallocate the block.
  EXPECT_CALL(real_mock_vendor, MockDealloc(_, kTestBlockSize, _))
      .WillOnce([](void* ptr, size_t size, align_t align) {
        _aligned_free(ptr);
      });
}

TEST_F(SLUBPoolTest, DiscreteAllocLargeObject) {
  TestPool pool(mock_vendor_satisfier);

  // Allocating an object larger than the largest bucket (64_B) should use the vendor directly.
  struct Large { char data[128]; };
  
  void* dummy_ptr = reinterpret_cast<void*>(0x12345678);

  EXPECT_CALL(real_mock_vendor, MockAlloc(sizeof(Large), _))
      .WillOnce(Return(dummy_ptr));

  Large* ptr = pool.DiscreteAlloc<Large>();
  ASSERT_EQ(reinterpret_cast<void*>(ptr), dummy_ptr);

  EXPECT_CALL(real_mock_vendor, MockDealloc(dummy_ptr, sizeof(Large), _))
      .Times(1);
  pool.DiscreteDealloc(ptr);
}

TEST_F(SLUBPoolTest, ContinuousAlloc) {
  TestPool pool(mock_vendor_satisfier);

  // ContinuousAlloc(10) for 16-byte objects = 160 bytes.
  // BucketforSize(160) should return the index for 256_B bucket if it existed,
  // but we have {16, 32, 64}. So 160 doesn't fit in any bucket.
  // Wait, BucketforSize(160) will return -1ul.
  
  void* dummy_ptr = reinterpret_cast<void*>(0x87654321);
  size_t expected_size = sizeof(double) * 10; // 80 bytes

  EXPECT_CALL(real_mock_vendor, MockAlloc(expected_size, _))
      .WillOnce(Return(dummy_ptr));

  double* ptr = pool.ContinuousAlloc<double>(10);
  ASSERT_EQ(reinterpret_cast<void*>(ptr), dummy_ptr);

  EXPECT_CALL(real_mock_vendor, MockDealloc(dummy_ptr, expected_size, _))
      .Times(1);
  pool.ContinuousDealloc(ptr, 10);
}

TEST_F(SLUBPoolTest, NewAndDel) {
  TestPool pool(mock_vendor_satisfier);

  struct MockObj {
    bool& destroyed;
    MockObj(bool& d) : destroyed(d) {}
    ~MockObj() { destroyed = true; }
  };

  bool destroyed = false;
  
  EXPECT_CALL(real_mock_vendor, MockAlloc(kTestBlockSize, _))
      .WillOnce([](size_t size, align_t align) {
        return _aligned_malloc(size, static_cast<size_t>(align));
      });

  MockObj* obj = pool.New<MockObj>(destroyed);
  ASSERT_NE(obj, nullptr);
  ASSERT_FALSE(destroyed);

  pool.Del(obj);
  ASSERT_TRUE(destroyed);

  EXPECT_CALL(real_mock_vendor, MockDealloc(_, kTestBlockSize, _))
      .WillOnce([](void* ptr, size_t size, align_t align) {
        _aligned_free(ptr);
      });
}

} // namespace crystal::mem
