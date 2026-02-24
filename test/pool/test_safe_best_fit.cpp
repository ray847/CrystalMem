#include "gtest/gtest.h"
#include "CrystalMem/pool/safe_best_fit/free_map.h"
#include "CrystalMem/type.h"

#include <numeric> // For std::iota (not directly used in free_map, but useful for testing)
#include <vector> // For managing test memory
#include <limits> // For std::numeric_limits
#include <utility> // For std::pair

// Include the main SafeBestFitPool header and the mock vendor header
#include "CrystalMem/pool/safe_best_fit/safe_best_fit.h"
#include "../mock_pool.h" // Corrected relative path

namespace crystal::mem {

// Custom allocator for std::map, to simulate memory and allow testing internal map behavior
// We need this because std::map's default allocator cannot allocate void* keys directly
// A simple stateful allocator that just uses global new/delete will suffice for the map nodes.
template <typename T>
struct TestAllocator {
    using value_type = T;

    TestAllocator() = default;
    template <class U> constexpr TestAllocator(const TestAllocator<U>&) noexcept {}

    T* allocate(size_t n) {
        if (n == 0) return nullptr;
        // Check for overflow before multiplication
        if (n > std::numeric_limits<size_t>::max() / sizeof(T)) throw std::bad_alloc();
        T* p = static_cast<T*>(::operator new(n * sizeof(T)));
        if (!p) throw std::bad_alloc();
        return p;
    }
    void deallocate(T* p, size_t n) noexcept {
        ::operator delete(p);
    }
};

template <typename T, typename U>
bool operator==(const TestAllocator<T>&, const TestAllocator<U>&) { return true; }
template <typename T, typename U>
bool operator!=(const TestAllocator<T>&, const TestAllocator<U>&) { return false; }

// Alias for the allocator type used by SafeBestFitFreeMap
using FreeMapAllocator = TestAllocator<std::pair<void* const, size_t>>;

// Test fixture for SafeBestFitFreeMap
class SafeBestFitFreeMapTest : public ::testing::Test {
protected:
    // Some arbitrary memory block for testing allocations
    // Since SafeBestFitFreeMap uses void*, we need a raw memory buffer
    // and just pass pointers into it.
    std::vector<std::byte> memory_buffer;
    void* buffer_start;
    size_t buffer_size;

    void SetUp() override {
        buffer_size = 1024 * 1024; // 1MB buffer
        memory_buffer.resize(buffer_size);
        buffer_start = memory_buffer.data();
    }

    void TearDown() override {
        // Nothing specific to tear down for this simple buffer
    }

    // Helper to check if a block exists in the map with expected size
    // This requires access to the private free_nodes_ map.
    // For now, we'll test via Alloc/Dealloc behavior.
};

TEST_F(SafeBestFitFreeMapTest, ConstructorInitializesEmptyMap) {
    SafeBestFitFreeMap<FreeMapAllocator> free_map{FreeMapAllocator{}};
    // We can infer it's empty if an allocation of any size fails.
    void* allocated_addr = free_map.Alloc(1, static_cast<align_t>(1));
    ASSERT_EQ(reinterpret_cast<uint64_t>(allocated_addr), -1ul);
}

TEST_F(SafeBestFitFreeMapTest, InsertNodeAddsBlock) {
    SafeBestFitFreeMap<FreeMapAllocator> free_map{FreeMapAllocator{}};
    void* initial_block_addr = buffer_start;
    size_t initial_block_size = 1024;
    free_map.InsertNode(initial_block_addr, initial_block_size);

    // Now, try to allocate from this block
    void* allocated_addr = free_map.Alloc(128, static_cast<align_t>(1));
    ASSERT_NE(reinterpret_cast<uint64_t>(allocated_addr), -1ul);
    ASSERT_EQ(allocated_addr, initial_block_addr); // Best fit will use the start if no padding
}

TEST_F(SafeBestFitFreeMapTest, AllocatesBestFit) {
    SafeBestFitFreeMap<FreeMapAllocator> free_map{FreeMapAllocator{}};
    // Create several free blocks
    free_map.InsertNode(reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start)), 100);
    free_map.InsertNode(reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start) + 100), 50);
    free_map.InsertNode(reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start) + 150), 200);

    // Request 40 bytes. Best fit should be the 50-byte block.
    void* allocated_addr = free_map.Alloc(40, static_cast<align_t>(1));
    ASSERT_NE(reinterpret_cast<uint64_t>(allocated_addr), -1ul);
    ASSERT_EQ(allocated_addr, reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start) + 100)); // Should be the 50-byte block
}

TEST_F(SafeBestFitFreeMapTest, AllocatesAndSplitsBlock) {
    SafeBestFitFreeMap<FreeMapAllocator> free_map{FreeMapAllocator{}};
    free_map.InsertNode(buffer_start, 200); // A 200-byte block

    void* allocated_addr = free_map.Alloc(100, static_cast<align_t>(1));
    ASSERT_NE(reinterpret_cast<uint64_t>(allocated_addr), -1ul);
    ASSERT_EQ(allocated_addr, buffer_start); // Expect allocation from start if no alignment needed

    // The original block (200 bytes) should now be split.
    // Assuming allocation takes from the beginning, a 100-byte remnant should be at buffer_start + 100.
    // Will rely on subsequent allocations for verification.
    void* second_allocated_addr = free_map.Alloc(50, static_cast<align_t>(1));
    ASSERT_NE(reinterpret_cast<uint64_t>(second_allocated_addr), -1ul);
    ASSERT_EQ(second_allocated_addr, reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start) + 100));
}

TEST_F(SafeBestFitFreeMapTest, AllocatesWithAlignment) {
    SafeBestFitFreeMap<FreeMapAllocator> free_map{FreeMapAllocator{}};
    // Insert a block that starts unaligned relative to, say, 16-byte alignment
    void* unaligned_start = reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start) + 5);
    free_map.InsertNode(unaligned_start, 200);

    // Request 100 bytes with 16-byte alignment
    void* allocated_addr = free_map.Alloc(100, static_cast<align_t>(16));
    ASSERT_NE(reinterpret_cast<uint64_t>(allocated_addr), -1ul);
    ASSERT_EQ(reinterpret_cast<size_t>(allocated_addr) % 16, 0); // Check alignment
    ASSERT_GE(reinterpret_cast<size_t>(allocated_addr), reinterpret_cast<size_t>(unaligned_start)); // Ensure it's within the block
    ASSERT_LE(reinterpret_cast<size_t>(allocated_addr) + 100, reinterpret_cast<size_t>(unaligned_start) + 200);
}

TEST_F(SafeBestFitFreeMapTest, AllocFailsIfNoFit) {
    SafeBestFitFreeMap<FreeMapAllocator> free_map{FreeMapAllocator{}};
    free_map.InsertNode(buffer_start, 100);
    void* allocated_addr = free_map.Alloc(200, static_cast<align_t>(1)); // Request more than available
    ASSERT_EQ(reinterpret_cast<uint64_t>(allocated_addr), -1ul);
}

TEST_F(SafeBestFitFreeMapTest, DeallocMergesWithRightBlock) {
    SafeBestFitFreeMap<FreeMapAllocator> free_map_alloc{FreeMapAllocator{}}; // Use a separate map for this part
    free_map_alloc.InsertNode(buffer_start, 100); // Allocate something here
    void* allocated_block = free_map_alloc.Alloc(50, static_cast<align_t>(1));
    free_map_alloc.InsertNode(reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start) + 100), 50); // Left block for merge
    free_map_alloc.InsertNode(reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start) + 200), 50); // Right block for merge

    // Scenario 1: No merge, just insert
    SafeBestFitFreeMap<FreeMapAllocator> map_no_merge{FreeMapAllocator{}};
    void* first_addr = reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start));
    void* second_addr = reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start) + 200);
    map_no_merge.InsertNode(first_addr, 100);
    map_no_merge.InsertNode(second_addr, 100);
    // Deallocate a block that is NOT adjacent to any free block
    void* dealloc_addr_no_merge = reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start) + 150);
    map_no_merge.Dealloc(dealloc_addr_no_merge, 20, static_cast<align_t>(1));
    // Verify by trying to allocate from the newly deallocated block.
    void* allocated_from_newly_dealloc = map_no_merge.Alloc(10, static_cast<align_t>(1));
    ASSERT_NE(reinterpret_cast<uint64_t>(allocated_from_newly_dealloc), -1ul);
    ASSERT_EQ(allocated_from_newly_dealloc, reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start) + 150));


    // Scenario 2: Merge with right
    SafeBestFitFreeMap<FreeMapAllocator> map_merge_right{FreeMapAllocator{}};
    map_merge_right.InsertNode(reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start) + 150), 50); // Right free block
    // Deallocate a block at buffer_start + 100, size 50. It should merge with the right.
    map_merge_right.Dealloc(reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start) + 100), 50, static_cast<align_t>(1));
    // The map should now contain a single block at buffer_start + 100 with size 100.
    // Verify by trying to allocate 70 bytes from the merged block.
    void* allocated_merged_addr = map_merge_right.Alloc(70, static_cast<align_t>(1));
    ASSERT_NE(reinterpret_cast<uint64_t>(allocated_merged_addr), -1ul);
    ASSERT_EQ(allocated_merged_addr, reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start) + 100));
    
    // Scenario 3: Merge with left
    SafeBestFitFreeMap<FreeMapAllocator> map_merge_left{FreeMapAllocator{}};
    map_merge_left.InsertNode(reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start) + 100), 50); // Left free block
    // Deallocate a block at buffer_start + 150, size 50. It should merge with the left.
    map_merge_left.Dealloc(reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start) + 150), 50, static_cast<align_t>(1));
    // The map should now contain a single block at buffer_start + 100 with size 100.
    allocated_merged_addr = map_merge_left.Alloc(70, static_cast<align_t>(1));
    ASSERT_NE(reinterpret_cast<uint64_t>(allocated_merged_addr), -1ul);
    ASSERT_EQ(allocated_merged_addr, reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start) + 100));

    // Scenario 4: Merge with both left and right
    SafeBestFitFreeMap<FreeMapAllocator> map_merge_both{FreeMapAllocator{}};
    map_merge_both.InsertNode(reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start) + 50), 50);  // Left free block
    map_merge_both.InsertNode(reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start) + 150), 50); // Right free block
    // Deallocate a block at buffer_start + 100, size 50. It should merge with both.
    map_merge_both.Dealloc(reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start) + 100), 50, static_cast<align_t>(1));
    // The map should now contain a single block at buffer_start + 50 with size 150.
    allocated_merged_addr = map_merge_both.Alloc(120, static_cast<align_t>(1));
    ASSERT_NE(reinterpret_cast<uint64_t>(allocated_merged_addr), -1ul);
    ASSERT_EQ(allocated_merged_addr, reinterpret_cast<void*>(reinterpret_cast<size_t>(buffer_start) + 50));
}

} // namespace crystal::mem

// Append the SafeBestFitPool tests
// Needed includes (put here to avoid changing indentation of existing content)
#include "CrystalMem/pool/safe_best_fit/safe_best_fit.h" // Include the main SafeBestFitPool header
#include "../mock_pool.h" // Include mock vendor

// Define a block size for testing purposes
constexpr size_t kTestBlockSize = 256; // Smaller block size for easier testing

// Test fixture for SafeBestFitPool
class SafeBestFitPoolTest : public ::testing::Test {
protected:
    crystal::mem::MockVendorConceptSatisfier mock_resource_vendor_satisfier;
    crystal::mem::RealMockVendor& real_mock_resource_vendor = mock_resource_vendor_satisfier.get_real_mock();

    // In SafeBestFitPool, LogicVendor can be the same as ResourceVendor, or different.
    // For simplicity, we'll use the same mock for both.
    // Note: If LogicVendor is different, it would need its own MockVendorConceptSatisfier and RealMockVendor.
    crystal::mem::MockVendorConceptSatisfier mock_logic_vendor_satisfier;
    crystal::mem::RealMockVendor& real_mock_logic_vendor = mock_logic_vendor_satisfier.get_real_mock();

    void SetUp() override {
        // Default behavior for mocks if not explicitly set in a test case
        // ResourceVendor Alloc for blocks:
        EXPECT_CALL(real_mock_resource_vendor, MockAlloc(kTestBlockSize, static_cast<crystal::mem::align_t>(alignof(crystal::mem::SafeBestFitBlock<kTestBlockSize>))))
            .WillRepeatedly([](size_t size, crystal::mem::align_t align) {
                // Simulate allocating a block of memory
                static std::vector<std::byte> block_memory_for_resource(kTestBlockSize); // Unique buffer
                return reinterpret_cast<void*>(block_memory_for_resource.data());
            });

        // ResourceVendor Dealloc for blocks:
        EXPECT_CALL(real_mock_resource_vendor, MockDealloc(::testing::_, kTestBlockSize, static_cast<crystal::mem::align_t>(alignof(crystal::mem::SafeBestFitBlock<kTestBlockSize>))))
            .WillRepeatedly(::testing::Return());

        // For small allocs: free_map operations are internal, no direct vendor call unless new block needed.
        // For large allocs (sizeof(T) > kTestBlockSize):
        EXPECT_CALL(real_mock_resource_vendor, MockAlloc(::testing::Gt(kTestBlockSize), ::testing::_))
            .WillRepeatedly([](size_t size, crystal::mem::align_t align) {
                // Simulate allocating a large external block
                static std::vector<std::byte> large_block_memory_for_resource(1024); // Unique buffer
                return reinterpret_cast<void*>(large_block_memory_for_resource.data());
            });
        EXPECT_CALL(real_mock_resource_vendor, MockDealloc(::testing::_, ::testing::Gt(kTestBlockSize), ::testing::_))
            .WillRepeatedly(::testing::Return());

        // LogicVendor Alloc/Dealloc for internal data structures (std::vector, std::map)
        // These allocations are generally small and varied.
        // We'll use a generic handler for any size/align, always returning a dummy pointer.
        EXPECT_CALL(real_mock_logic_vendor, MockAlloc(::testing::_, ::testing::_))
            .WillRepeatedly([](size_t size, crystal::mem::align_t align) {
                // Simulate internal allocation. Use a larger static buffer or dynamically allocate as needed.
                // For simplicity, a small, fixed-size static buffer that's large enough for typical internal allocations.
                static std::vector<std::byte> internal_memory_for_logic(1024); // Provide a buffer for logic allocations
                return reinterpret_cast<void*>(internal_memory_for_logic.data());
            });
        EXPECT_CALL(real_mock_logic_vendor, MockDealloc(::testing::_, ::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return());
    }
};

// Test construction and destruction
TEST_F(SafeBestFitPoolTest, ConstructorDestructor) {
    // Only verify that a pool can be constructed and destructed without crashing.
    // Mocks will verify resource_vendor_ interaction during Clear().
    crystal::mem::SafeBestFitPool<kTestBlockSize,
                                   crystal::mem::MockVendorConceptSatisfier,
                                   crystal::mem::MockVendorConceptSatisfier> pool(mock_resource_vendor_satisfier, mock_logic_vendor_satisfier);
    // Destructor calls Clear(), which should call resource_vendor_.Dealloc for blocks.
}

// Test DiscreteAlloc for small object (fits in block)
TEST_F(SafeBestFitPoolTest, DiscreteAllocSmallObject) {
    crystal::mem::SafeBestFitPool<kTestBlockSize,
                                   crystal::mem::MockVendorConceptSatisfier,
                                   crystal::mem::MockVendorConceptSatisfier> pool(mock_resource_vendor_satisfier, mock_logic_vendor_satisfier);
    
    // Initial call to DiscreteAlloc, will trigger AppendBlock
    EXPECT_CALL(real_mock_resource_vendor, MockAlloc(kTestBlockSize, static_cast<crystal::mem::align_t>(alignof(crystal::mem::SafeBestFitBlock<kTestBlockSize>))))
        .WillOnce([](size_t size, crystal::mem::align_t align) {
            static std::vector<std::byte> block_memory(kTestBlockSize);
            return reinterpret_cast<void*>(block_memory.data());
        });

    struct SmallObj { int x; };
    SmallObj* obj = pool.DiscreteAlloc<SmallObj>();
    ASSERT_NE(obj, nullptr);
}

// Test DiscreteAlloc for large object (exceeds block size)
TEST_F(SafeBestFitPoolTest, DiscreteAllocLargeObject) {
    crystal::mem::SafeBestFitPool<kTestBlockSize,
                                   crystal::mem::MockVendorConceptSatisfier,
                                   crystal::mem::MockVendorConceptSatisfier> pool(mock_resource_vendor_satisfier, mock_logic_vendor_satisfier);

    struct LargeObj { std::array<char, kTestBlockSize + 100> data; }; // Larger than kTestBlockSize
    
    EXPECT_CALL(real_mock_resource_vendor, MockAlloc(sizeof(LargeObj), static_cast<crystal::mem::align_t>(alignof(LargeObj))))
        .WillOnce([](size_t size, crystal::mem::align_t align) {
            static std::vector<std::byte> large_block_memory(size);
            return reinterpret_cast<void*>(large_block_memory.data());
        });

    LargeObj* obj = pool.DiscreteAlloc<LargeObj>();
    ASSERT_NE(obj, nullptr);
}

// Test DiscreteDealloc for small object
TEST_F(SafeBestFitPoolTest, DiscreteDeallocSmallObject) {
    crystal::mem::SafeBestFitPool<kTestBlockSize,
                                   crystal::mem::MockVendorConceptSatisfier,
                                   crystal::mem::MockVendorConceptSatisfier> pool(mock_resource_vendor_satisfier, mock_logic_vendor_satisfier);
    struct SmallObj { int x; };
    
    // Allocate a small object first (will trigger AppendBlock)
    EXPECT_CALL(real_mock_resource_vendor, MockAlloc(kTestBlockSize, static_cast<crystal::mem::align_t>(alignof(crystal::mem::SafeBestFitBlock<kTestBlockSize>))))
        .WillOnce([](size_t size, crystal::mem::align_t align) {
            static std::vector<std::byte> block_memory(kTestBlockSize);
            return reinterpret_cast<void*>(block_memory.data());
        });
    SmallObj* obj = pool.DiscreteAlloc<SmallObj>();
    ASSERT_NE(obj, nullptr);

    // Then deallocate. This should go to free_map_.Dealloc, not resource_vendor_.Dealloc
    // No direct EXPECT_CALL on resource_vendor_ for deallocating small objects.
    pool.DiscreteDealloc(obj);
    SUCCEED();
}

// Test DiscreteDealloc for large object
TEST_F(SafeBestFitPoolTest, DiscreteDeallocLargeObject) {
    crystal::mem::SafeBestFitPool<kTestBlockSize,
                                   crystal::mem::MockVendorConceptSatisfier,
                                   crystal::mem::MockVendorConceptSatisfier> pool(mock_resource_vendor_satisfier, mock_logic_vendor_satisfier);
    struct LargeObj { std::array<char, kTestBlockSize + 100> data; };

    // Allocate a large object
    EXPECT_CALL(real_mock_resource_vendor, MockAlloc(sizeof(LargeObj), static_cast<crystal::mem::align_t>(alignof(LargeObj))))
        .WillOnce([](size_t size, crystal::mem::align_t align) {
            static std::vector<std::byte> large_block_memory(size);
            return reinterpret_cast<void*>(large_block_memory.data());
        });
    LargeObj* obj = pool.DiscreteAlloc<LargeObj>();
    ASSERT_NE(obj, nullptr);

    // Deallocate the large object
    EXPECT_CALL(real_mock_resource_vendor, MockDealloc(reinterpret_cast<void*>(obj), sizeof(LargeObj), static_cast<crystal::mem::align_t>(alignof(LargeObj))))
        .Times(1);
    pool.DiscreteDealloc(obj);
}

// Test Clear function
TEST_F(SafeBestFitPoolTest, ClearDeallocatesAllMemory) {
    crystal::mem::SafeBestFitPool<kTestBlockSize,
                                   crystal::mem::MockVendorConceptSatisfier,
                                   crystal::mem::MockVendorConceptSatisfier> pool(mock_resource_vendor_satisfier, mock_logic_vendor_satisfier);

    // Setup: Allocate one small object and one large object
    struct SmallObj { int x; };
    struct LargeObj { std::array<char, kTestBlockSize + 100> data; };

    // Mocks for allocation during setup
    EXPECT_CALL(real_mock_resource_vendor, MockAlloc(kTestBlockSize, static_cast<crystal::mem::align_t>(alignof(crystal::mem::SafeBestFitBlock<kTestBlockSize>))))
        .WillOnce([](size_t size, crystal::mem::align_t align) {
            static std::vector<std::byte> block_memory(kTestBlockSize);
            return reinterpret_cast<void*>(block_memory.data());
        });
    SmallObj* small_obj = pool.DiscreteAlloc<SmallObj>(); // Will cause a block to be appended

    void* large_obj_ptr;
    EXPECT_CALL(real_mock_resource_vendor, MockAlloc(sizeof(LargeObj), static_cast<crystal::mem::align_t>(alignof(LargeObj))))
        .WillOnce([&](size_t size, crystal::mem::align_t align) {
            static std::vector<std::byte> large_block_memory(size);
            large_obj_ptr = reinterpret_cast<void*>(large_block_memory.data());
            return large_obj_ptr;
        });
    LargeObj* large_obj = pool.DiscreteAlloc<LargeObj>(); // Will cause external allocation

    ASSERT_NE(small_obj, nullptr);
    ASSERT_NE(large_obj, nullptr);

    // Expect Dealloc calls during Clear()
    EXPECT_CALL(real_mock_resource_vendor, MockDealloc(::testing::_, kTestBlockSize, static_cast<crystal::mem::align_t>(kTestBlockSize))) // For the internal block
        .Times(1);
    EXPECT_CALL(real_mock_resource_vendor, MockDealloc(large_obj_ptr, sizeof(LargeObj), static_cast<crystal::mem::align_t>(alignof(LargeObj)))) // For the external large object
        .Times(1);

    // No explicit call to pool.Clear() here; it will be called by the destructor.
}
