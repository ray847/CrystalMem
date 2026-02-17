#ifndef CRYSTALMEM_POOL_SLUB_SLUB_H_
#define CRYSTALMEM_POOL_SLUB_SLUB_H_

#include <CrystalBase/integer_sequence.h> // integer_sequence

#include <array> // std::array
#include <cstddef> // std::byte
#include <limits>  // std::nuneric_limits
#include <memory>  // std::allocator
#include <tuple>   // std::tuple
#include <unordered_map> // std::unordered_map
#include <utility> // std::index_sequence

#include "CrystalMem/pool/concept.h" // AnyPool
#include "CrystalMem/type.h"         // _kB
#include "CrystalMem/vendor.h"       // AnyVendor
#include "CrystalMem/vendor/allocator.h" // VendorAllocator
#include "bucket.h"                  // SLUBBucket

namespace crystal::mem {

using std::allocator, std::byte, std::tuple, std::index_sequence,
    std::make_index_sequence, std::numeric_limits, std::allocator_traits,
    std::get, std::apply, std::unordered_map, std::pair, std::hash,
    std::equal_to, std::array, std::is_same_v;

template <size_t kBlockSize,
          integer_sequence kSlotSizes,
          AnyVendor ResourceVendor,
          AnyVendor LogicVendor = ResourceVendor>
class SLUBPool {
 public:
  /* Attributes */
  static constexpr bool kInMemoryOptimization = true;

  template <size_t kSlotSize>
  using Bucket = SLUBBucket<kBlockSize, kSlotSize, ResourceVendor>;
  using BucketInterface = SLUBBucketInterface;

  /* Constructor */
  SLUBPool(const ResourceVendor& vendor)
      requires is_same_v<ResourceVendor, LogicVendor>
      : SLUBPool(vendor, vendor) {
  }
  SLUBPool(const ResourceVendor& resource_vendor,
           const LogicVendor& logic_vendor) :
      resource_vendor_(resource_vendor),
      buckets_(Buckets::Init(resource_vendor)),
      extern_allocs_(
          VendorAllocator<pair<void*, pair<size_t, align_t>>, LogicVendor>{
              logic_vendor }) {
    size_t i = 0;
    apply(
        [&](auto&... buckets) { ((bucket_interfaces_[i++] = &buckets), ...); },
        buckets_);
  }
  /* Destructor */
  ~SLUBPool() {
    Clear();
  }
  SLUBPool(const SLUBPool& other) = delete;
  SLUBPool(SLUBPool&& other) :
      resource_vendor_(other.resource_vendor_),
      buckets_(move(other.buckets_)),
      extern_allocs_(move(other.extern_allocs_)) {
    size_t i = 0;
    apply(
        [&](auto&... buckets) { ((bucket_interfaces_[i++] = &buckets), ...); },
        buckets_);
  }
  SLUBPool& operator=(const SLUBPool& rhs) = delete;
  SLUBPool& operator=(SLUBPool&& rhs) {
    resource_vendor_ = rhs.resource_vendor_;
    buckets_ = move(rhs.buckets_);
    extern_allocs_ = move(rhs.extern_allocs_);
    size_t i = 0;
    apply(
        [&](auto&... buckets) { ((bucket_interfaces_[i++] = &buckets), ...); },
        buckets_);
  }

  /* Functions */
  template <typename T>
  T* DiscreteAlloc() {
    constexpr size_t bucket_idx = BucketforSize(sizeof(T));
    if constexpr (bucket_idx == -1ul) { // no bucket
      void* addr =
          resource_vendor_.Alloc(sizeof(T), static_cast<align_t>(alignof(T)));
      extern_allocs_[addr] = { sizeof(T), static_cast<align_t>(alignof(T)) };
      return reinterpret_cast<T*>(addr);
    } else return reinterpret_cast<T*>(get<bucket_idx>(buckets_).AllocSlot());
  }
  template <typename T>
  void DiscreteDealloc(T* ptr) {
    constexpr size_t bucket_idx = BucketforSize(sizeof(T));
    if constexpr (bucket_idx == -1ul) { // no bucket
      resource_vendor_.Dealloc(
          ptr, sizeof(T), static_cast<align_t>(alignof(T)));
      extern_allocs_.erase(reinterpret_cast<void*>(ptr));
    } else get<bucket_idx>(buckets_).DeallocSlot(ptr);
  }
  template <typename T>
  T* ContinuousAlloc(size_t n) {
    size_t bucket_idx = BucketforSize(sizeof(T) * n);
    if (bucket_idx == -1ul) { // no bucket
      void* addr = resource_vendor_.Alloc(sizeof(T) * n,
                                          static_cast<align_t>(alignof(T)));
      extern_allocs_[addr] = { sizeof(T), static_cast<align_t>(alignof(T)) };
      return reinterpret_cast<T*>(addr);
    } else {
      return reinterpret_cast<T*>(bucket_interfaces_[bucket_idx]->AllocSlot());
    }
  }
  template <typename T>
  void ContinuousDealloc(T* ptr, size_t n) {
    size_t bucket_idx = BucketforSize(sizeof(T) * n);
    if (bucket_idx == -1ul) { // no bucket
      resource_vendor_.Dealloc(
          ptr, sizeof(T) * n, static_cast<align_t>(alignof(T)));
      extern_allocs_.erase(reinterpret_cast<void*>(ptr));
    } else {
      bucket_interfaces_[bucket_idx]->DeallocSlot(reinterpret_cast<void*>(ptr));
    }
  }
  template <typename T, typename... Args>
  T* New(Args&&... args) {
    T* addr = DiscreteAlloc<T>();
    std::construct_at(addr, args...);
    return addr;
  }
  template <typename T>
  void Del(T* addr) {
    addr->~T();
    DiscreteDealloc(addr);
  }
  void Clear() {
    /* Clear the buckets. */
    apply([](auto&... buckets) { (buckets.Clear(), ...); }, buckets_);
    /* Clear external allocations. */
    for (auto [addr, size_align] : extern_allocs_) {
      auto [size, align] = size_align;
      resource_vendor_.Dealloc(addr, size, align);
    }
    extern_allocs_.clear();
  }

 private:
  /* Variables */
  ResourceVendor resource_vendor_;
  template <typename Seq>
  struct ArraytoTuple;
  template <size_t... Is>
  struct ArraytoTuple<index_sequence<Is...>> {
    using type = tuple<Bucket<kSlotSizes[Is]>...>;
    static auto Init(const ResourceVendor& vendor) {
      return type{ Bucket<kSlotSizes[Is]>(vendor)... };
    }
  };
  using Buckets = ArraytoTuple<make_index_sequence<kSlotSizes.size()>>;
  Buckets::type buckets_;
  array<BucketInterface*, kSlotSizes.size()> bucket_interfaces_;

  unordered_map<void*,
                pair<size_t, align_t>,
                hash<void*>,
                equal_to<void*>,
                VendorAllocator<pair<void* const, pair<size_t, align_t>>,
                                LogicVendor>>
      extern_allocs_;

  /* Functions */
  constexpr size_t BucketforSize(size_t size) {
    size_t best_fit_idx = -1ul;
    size_t best_fit_slot_size = numeric_limits<size_t>::max();
    size_t i = 0;
    for (auto slot_size : kSlotSizes) {
      if (size <= slot_size && slot_size < best_fit_slot_size) {
        best_fit_slot_size = slot_size;
        best_fit_idx = i;
      }
      i++;
    }
    return best_fit_idx;
  }
};
static_assert(AnyPool<SLUBPool<4_kB, { 8_B, 2_kB }, Vendor<OSResource>>>);

} // namespace crystal::mem

#endif
