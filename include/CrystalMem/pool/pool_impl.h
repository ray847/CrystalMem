#ifndef CRYSTALMEM_POOL_IMPL_H_
#define CRYSTALMEM_POOL_IMPL_H_

#include <cstdint>
#include <cstdlib>

#include <tuple> // std::tuple
#include <list> // std::list
#include <bit> // std::bit_width
#include <utility>
#include <vector> // std::vector

#include "CrystalMem/types.h" // ssize_t
#include "CrystalMem/global.h" // kPageSize
#include "CrystalMem/resource.h" // Resource

namespace crystal::mem {
template<
size_t kBlockSize = kPageSize,
typename Resource  = Resource<Source::OS>
>
class PoolImpl {
public:

  /* Constructor */
  explicit PoolImpl(Resource& resource):
    resource_(resource),
    buckets_() {}
  PoolImpl(const PoolImpl& other) = delete;
  PoolImpl(PoolImpl&& other):
    resource_(other.resource_),
    buckets_(std::move(other.buckets_)) {}
  PoolImpl& operator=(const PoolImpl& rhs) = delete;
  /* Destructor */
  ~PoolImpl() {Clear();}

  /* Functions */
  /**
   * Allocate memory for the input type.
   *
   * @tparam T The object type.
   * @return Pointer to the allocated memory.
   *
   * @note This function only allocates memory and does not initialize the
   * object. For allocation + initialization, check `New`.
   */
  template<typename T>
  T* Alloc() {
    static_assert(BucketIndex(sizeof(T)) < std::tuple_size_v<decltype(buckets_)>, "Object too big for memory pool.");
    void* slot = std::get<BucketIndex(sizeof(T))>(
      buckets_
    ).AllocSlot(resource_);
    return reinterpret_cast<T*>(slot);
  }
  /**
   * Deallocate memory for the input type.
   *
   * @tparam T The object type.
   * @return Pointer to the allocated memory.
   *
   * @note This function only deallocates memory and does not call the
   * destructor. For destruction + deallocation, check `Del`.
   */
  template<typename T>
  void Dealloc(T* ptr) {
    std::get<BucketIndex(sizeof(T))>(buckets_).DeallocSlot(ptr);
  }
  /**
   * Allocate memory dynamically for the input type.
   *
   * @tparam T The object type.
   * @tparma outside_pool Whether to allocate the memory directly from the
   * resource.
   * @param n Number of objects. (This is the dynamic part)
   * @return Pointer to the allocated memory.
   *
   * @note This function only allocates memory and does not initialize the
   * object. For allocation + initialization, check `New`.
   */
  template<typename T, bool outside_pool = true>
  T* AllocDyn(size_t n) {
    if constexpr (outside_pool) {
      T* ptr = reinterpret_cast<T*>(
        resource_.Alloc(n * sizeof(T), static_cast<align_t>(alignof(T)))
      );
      return ptr;
    } else {
      void* slot = std::get<BucketIndexDyn(n * sizeof(T))>(
        buckets_
      ).AllocSlot(resource_);
      return reinterpret_cast<T*>(slot);
    }
  }
  /**
   * Deallocate memory for the input type.
   *
   * @tparam T The object type.
   * @tparma outside_pool Whether the memory is allocated outside of the pool.
   * @param n Number of objects. (This is the dynamic part)
   * @return Pointer to the allocated memory.
   *
   * @note This function only deallocates memory and does not call the
   * destructor. For destruction + deallocation, check `Del`.
   */
  template<typename T, bool outside_pool = true>
  void DeallocDyn(T* ptr, size_t n) {
    if constexpr (outside_pool) {
      resource_.Dealloc(ptr, n * sizeof(T));
    } else {
      std::get<BucketIndex(sizeof(T))>(buckets_).DeallocSlot(ptr);
    }
  }
  /**
   * Construct object in newly allocated memory.
   *
   * @tparam T The object type.
   * @param init_args Arguments used to initialize the object.
   * @return Pointer to the allocated memory.
   *
   * @note This does both allocation and initialization. For only allocation,
   * check `Alloc`.
   */
  template<typename T, typename... InitArgs>
  T* New(InitArgs... init_args) {
    static_assert(BucketIndex(sizeof(T)) < std::tuple_size_v<decltype(buckets_)>, "Object too big for memory pool.");
    void* slot = std::get<BucketIndex(sizeof(T))>(
      buckets_
    ).AllocSlot(resource_);
    return new(slot) T(init_args...); // call constructor
  }
  /**
   * Delete object and release memory.
   *
   * @tparam T The object type.
   * @param ptr Pointer to the object.
   *
   * @note This function will call the destructor before cleaning up memory.
   * For purely memory release, check `Dealloc`.
   */
  template<typename T>
  void Del(T* ptr) {
    ptr->~T(); // call destructor
    std::get<BucketIndex(sizeof(T))>(buckets_).DeallocSlot(ptr);
  }
  /**
   * Clear the pool by deallocating all memory.
   *
   * @note This function does not properly terminate objects in the pool.
   */
  void Clear() {
    std::apply(
      [&](auto&... buckets){
        (buckets.Clear(resource_), ...);
      },
      buckets_
    );
  }

private:

  /* Inner Types */
  template<size_t kSlotSize>
  union Slot {
    static_assert(kSlotSize >= sizeof(ssize_t), "Size of a slot must be larger than that of a free list node.");
    std::array<std::byte, kSlotSize> data;
    ssize_t free_nxt;
  };

  template<size_t kSlotSize>
  struct alignas(kBlockSize) Block {
    /* Constants */
    static constexpr size_t kCapacity =
      (kBlockSize - sizeof(ssize_t)) / kSlotSize; // make room for free_head
    static_assert(kCapacity > 0, "Slot size too big for a single block.");
    /* Variables */
    std::array<Slot<kSlotSize>, kCapacity> data;
    ssize_t free_head;
    /* Constructor */
    Block() : free_head(0) {
      for (ssize_t i = 0; i < kCapacity - 1; ++i) {
        data[i].free_nxt = i + 1;
      }
      data.back().free_nxt = -1;
    }
    /* Functions */
    /**
     * Allocate a slot from the block.
     */
    void* AllocSlot() {
      if (free_head == -1) [[unlikely]] {
        return nullptr;
      }
      void* ptr = &data[free_head];
      free_head = data[free_head].free_nxt;
      return ptr;
    }
    /**
     * Deallocate a slot from the block.
     *
     * This function does not check if the input address is actually in the
     * block.
     */
    void DeallocSlot(void* ptr) {
      ssize_t idx = reinterpret_cast<Slot<kSlotSize>*>(ptr) - data.data();
      data[idx].free_nxt = free_head;
      free_head = idx;
    }
    bool Full() const {return free_head == -1;}
  };

  template<size_t kSlotSize>
  struct Bucket : public std::list<Block<kSlotSize>*> {
  public:
    /* Constructor */
    explicit Bucket() : std::list<Block<kSlotSize>*>() {}
    /* Functions */
    /**
     * Allocate a slot with size of `kSlotSize`.
     */
    [[nodiscard]] void* AllocSlot(Resource& resource) {
      Block<kSlotSize>& block = GetAvailableBlock(resource);
      void* slot = block.AllocSlot();
      if (block.Full()) available_blocks.pop_back();
      return slot;
    }
    /**
     * Deallocate a slot with size of `kSlotSize`.
     */
    void DeallocSlot(void* ptr) {
      Block<kSlotSize>* block = Addr2Block(ptr);
      if (block->Full()) [[unlikely]] available_blocks.push_back(block);
      block->DeallocSlot(ptr);
    }
    /**
     * Clear the bucket by deallocating all blocks.
     */
    void Clear(Resource& resource) {
      for (auto block : *this) {
        DeallocBlock(block, resource);
      }
      this->clear();
      available_blocks.clear();
    }
  private:
    /* Constructor */
    std::vector<Block<kSlotSize>*> available_blocks; //< used as stack
    /* Functions */
    /**
     * Pushes new block to the front of the list.
     */
    void AllocBlock(Resource& resource) {
      Block<kSlotSize>* new_block = reinterpret_cast<Block<kSlotSize>*>(
        resource.Alloc(sizeof(Block<kSlotSize>),
                       static_cast<align_t>(alignof(Block<kSlotSize>)))
      );
      new (new_block) Block<kSlotSize>(); // call constructor
      this->push_front(new_block);
      available_blocks.push_back(new_block);
    }
    /**
     * Deallocate the input block.
     */
    void DeallocBlock(Block<kSlotSize>* block, Resource& resource) {
      block->~Block(); // call destructor
      resource.Dealloc(block, sizeof(Block<kSlotSize>));
    }
    /**
     * Get a block available for allocation.
     *
     * If there are no available blocks, allocate a new one.
     */
    Block<kSlotSize>& GetAvailableBlock(Resource& resource) {
      if (available_blocks.empty()) [[unlikely]] {
        AllocBlock(resource);
      }
      return *(available_blocks.back());
    }
    /**
     * Get the block containing the input address.
     *
     * Relies block alignment & bit masking.
     */
    static Block<kSlotSize>* Addr2Block(void* addr) {
      return reinterpret_cast<Block<kSlotSize>*>(
        reinterpret_cast<uintptr_t>(addr) & ~(alignof(Block<kSlotSize>) - 1)
      );
    }
  };

  /* Constants */
  static_assert(kBlockSize > 4, "Block size must be greater than or equal to 4 bytes.");
  static constexpr size_t kNBuckets = std::bit_width(
    kBlockSize
  ) - 3; // bucket size starts from 4 and ends at (kBlockSize / 2)

  /* Variables */
  Resource& resource_;
  /* Automatically determine the number of buckets base on the block size. */
  template<typename Sequence>
  struct BucketTupleGenerator;
  template<size_t... Is>
  struct BucketTupleGenerator<std::index_sequence<Is...>> {
    using type = std::tuple<Bucket<size_t{1} << (Is + 2)>...>; // start from 4
  };
  using BucketsType =
    typename BucketTupleGenerator<std::make_index_sequence<kNBuckets>>::type;
  BucketsType buckets_;

  /* Functions */
  static consteval size_t BucketIndex(size_t obj_size) {
    return std::max(
      0,
      std::bit_width(obj_size - 1) - 2 // bucket size starts from 4
    );
  }
  static size_t BucketIndexDyn(size_t obj_size) {
    return std::max(
      0,
      std::bit_width(obj_size - 1) - 2 // bucket size starts from 4
    );
  }
};
} // namespace crystal::mem

#endif
