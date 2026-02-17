#ifndef CRYSTALMEM_POOL_SAFE_BEST_FIT_SAFE_BEST_FIT_H_
#define CRYSTALMEM_POOL_SAFE_BEST_FIT_SAFE_BEST_FIT_H_

#include <CrystalBase/integer_sequence.h> // integer_sequence

#include <array> // std::array
#include <limits> // std::numeric_limits
#include <memory>
#include <type_traits> // std::is_same_v
#include <vector> // std::vector
#include <map> // std::map
#include <utility> // std::pair
#include <unordered_map> // std::unordered_map

#include "CrystalMem/pool/concept.h" // AnyPool
#include "CrystalMem/resource/os.h" // OSResource
#include "CrystalMem/type.h" // _kB
#include "CrystalMem/vendor.h" // Vendor
#include "CrystalMem/vendor/allocator.h" // VendorAllocator
#include "CrystalMem/vendor/concept.h" // AnyVendor
#include "block.h" // SafeBestFitBlock
#include "free_map.h" // SafeBestFitFreeMap

namespace crystal::mem {

using std::array, std::vector, std::map, std::move, std::is_same_v, std::less,
    std::pair, std::numeric_limits, std::swap, std::unordered_map,
    std::construct_at;

/**
 * A memory pool that implements the naive best fit strategy.
 *
 * @tparam kBlockSize The block size the strategy operates on.
 * @tparam ResourceVendor The vendor to request data memory from. Since this
 * pool doesn't have in-memory optimization, memory obtained from this vendor
 * will not be operated on.
 * @tparam LogicVendor The vendor to request memory for allocating logic state.
 */
template <size_t kBlockSize,
          AnyVendor ResourceVendor,
          AnyVendor LogicVendor = ResourceVendor>
class SafeBestFitPool {
 public:
  using Block = SafeBestFitBlock<kBlockSize>;
  using FreeMap =
      SafeBestFitFreeMap<VendorAllocator<pair<void* const, size_t>, LogicVendor>>;

  /* Attributes */
  static constexpr bool kInMemoryOptimization = false;

  /* Constructor */
  SafeBestFitPool(const ResourceVendor& vendor)
      requires is_same_v<ResourceVendor, LogicVendor>
      : SafeBestFitPool(vendor, vendor) {
  }
  SafeBestFitPool(const ResourceVendor& resource_vendor,
                  const LogicVendor& logic_vendor) :
      resource_vendor_(resource_vendor),
      blocks_(VendorAllocator<Block*, LogicVendor>(logic_vendor)),
      free_map_(VendorAllocator<pair<void* const, size_t>, LogicVendor>(
          logic_vendor)) {
  }
  /* No Copying */
  SafeBestFitPool(const SafeBestFitPool&) = delete;
  /* Move Constructor */
  SafeBestFitPool(SafeBestFitPool&& other) : blocks_(move(other.blocks_)) {
  }
  /* No Copying */
  SafeBestFitPool& operator=(const SafeBestFitPool&) = delete;
  SafeBestFitPool& operator=(SafeBestFitPool&& rhs) {
    blocks_ = move(rhs.blocks_);
  }
  /* Destructor */
  ~SafeBestFitPool() {
    Clear();
  }

  /* Functions */
  template <typename T>
  T* DiscreteAlloc() {
    if constexpr (sizeof(T) > kBlockSize) {
      void* addr =
          resource_vendor_.Alloc(sizeof(T), static_cast<align_t>(alignof(T)));
      extern_allocs_[addr] = { sizeof(T), static_cast<align_t>(alignof(T)) };
      return reinterpret_cast<T*>(addr);
    } else {
      void* addr = free_map_.Alloc(sizeof(T), static_cast<align_t>(alignof(T)));
      if (reinterpret_cast<size_t>(addr) == -1ul) {
        void* new_block = reinterpret_cast<void*>(AppendBlock());
        free_map_.InsertNode(
            reinterpret_cast<void*>(reinterpret_cast<size_t>(new_block)
                                    + sizeof(T)),
            kBlockSize - sizeof(T));
        return reinterpret_cast<T*>(new_block);
      } else return reinterpret_cast<T*>(addr);
    }
  }
  template <typename T>
  void DiscreteDealloc(T* ptr) {
    if constexpr (sizeof(T) > kBlockSize) {
      resource_vendor_.Dealloc(
          static_cast<void*>(ptr), sizeof(T), static_cast<align_t>(alignof(T)));
      extern_allocs_.erase(reinterpret_cast<void*>(ptr));
    } else
      free_map_.Dealloc(
          static_cast<void*>(ptr), sizeof(T), static_cast<align_t>(alignof(T)));
  }
  template <typename T>
  T* ContinuousAlloc(size_t n) {
    if (sizeof(T) * n > kBlockSize) {
      void* addr = resource_vendor_.Alloc(sizeof(T) * n,
                                          static_cast<align_t>(alignof(T)));
      extern_allocs_[addr] = { sizeof(T) * n,
                               static_cast<align_t>(alignof(T)) };
      return reinterpret_cast<T*>(addr);
    } else {
      void* addr =
          free_map_.Alloc(sizeof(T) * n, static_cast<align_t>(alignof(T)));
      if (reinterpret_cast<size_t>(addr) == -1ul) {
        void* new_block = reinterpret_cast<void*>(AppendBlock());
        free_map_.InsertNode(
            reinterpret_cast<void*>(reinterpret_cast<size_t>(new_block)
                                    + sizeof(T) * n),
            kBlockSize - sizeof(T) * n);
        return reinterpret_cast<T*>(new_block);
      } else return reinterpret_cast<T*>(addr);
    }
  }
  template <typename T>
  void ContinuousDealloc(T* ptr, size_t n) {
    if (sizeof(T) * n > kBlockSize) {
      resource_vendor_.Dealloc(static_cast<void*>(ptr),
                               sizeof(T) * n,
                               static_cast<align_t>(alignof(T)));
      extern_allocs_.erase(reinterpret_cast<void*>(ptr));
    } else
      free_map_.Dealloc(static_cast<void*>(ptr),
                        sizeof(T) * n,
                        static_cast<align_t>(alignof(T)));
  }
  template <typename T, typename ...Args>
  T* New(Args&&... args) {
    T* addr = DiscreteAlloc<T>();
    construct_at(addr, args...);
    return addr;
  }
  template <typename T>
  void Del(T* ptr) {
    ptr->~T();
    DiscreteDealloc(ptr);
  }
  void Clear() {
    for (auto block : blocks_)
      resource_vendor_.Dealloc(
          block, kBlockSize, static_cast<align_t>(kBlockSize));
    for (auto [addr, size_align] : extern_allocs_) {
      auto [size, align] = size_align;
      resource_vendor_.Dealloc(addr, size, align);
    }
    extern_allocs_.clear();
  }

 private:
  /* Variables */
  ResourceVendor resource_vendor_;
  vector<Block*, VendorAllocator<Block*, LogicVendor>> blocks_;
  FreeMap free_map_;
  unordered_map<void*, pair<size_t, align_t>> extern_allocs_;

  /* Functions */
  Block* AppendBlock() {
    blocks_.push_back(reinterpret_cast<Block*>(resource_vendor_.Alloc(
        sizeof(Block), static_cast<align_t>(alignof(Block)))));
    return blocks_.back();
  }
};
static_assert(AnyPool<SafeBestFitPool<4_kB, Vendor<OSResource>>>);

} // namespace crystal::mem

#endif
