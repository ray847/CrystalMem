#ifndef CRYSTALMEM_POOL_SLUB_SLUB_H_
#define CRYSTALMEM_POOL_SLUB_SLUB_H_

#include <CrystalBase/integer_sequence.h> // integer_sequence

#include <cstddef> // std::byte
#include <limits> // std::nuneric_limits
#include <memory> // std::allocator
#include <tuple> // std::tuple
#include <utility> // std::index_sequence

#include "CrystalMem/type.h" // _kB
#include "CrystalMem/vendor.h" // AnyVendor
#include "../concept.h" // AnyPool
#include "bucket.h" // SLUBBucket


namespace crystal::mem {

using std::allocator, std::byte, std::tuple, std::index_sequence,
    std::make_index_sequence, std::numeric_limits, std::allocator_traits,
    std::get, std::apply, std::move;

template <size_t kBlockSize, integer_sequence kSlotSizes, AnyVendor Vendor>
class SLUBPool {
 public:
  /* Attributes */
  static constexpr bool kInMemoryOptimization = true;

  template <size_t kSlotSize>
  using Bucket = SLUBBucket<kBlockSize, kSlotSize, Vendor>;

  /* Constructor */
  SLUBPool(const Vendor& vendor = {}) : vendor_(vendor) {
  }
  /* Destructor */
  ~SLUBPool() {
    Clear();
  }
  SLUBPool(const SLUBPool& other) = delete;
  SLUBPool(SLUBPool&& other) :
      vendor_(other.vendor_), buckets_(move(other.buckets_)) {
  }
  SLUBPool& operator=(const SLUBPool& rhs) = delete;
  SLUBPool& operator=(SLUBPool&& rhs) {
    vendor_ = rhs.vendor_;
    buckets_ = move(rhs.buckets_);
  }

  /* Functions */
  template <typename T>
  T* DiscreteAlloc() {
    constexpr size_t bucket_idx = BucketforSize(sizeof(T));
    if constexpr (bucket_idx == -1ul) { // no bucket
      return vendor_.Alloc(sizeof(T), alignof(T));
    }
    return reinterpret_cast<T*>(get<bucket_idx>(buckets_).AllocSlot());
  }
  template <typename T>
  void DiscreteDealloc(T* ptr) {
    constexpr size_t bucket_idx = BucketforSize(sizeof(T));
    if constexpr (bucket_idx == -1ul) { // no bucket
      vendor_.Dealloc(ptr, sizeof(T), alignof(T));
    }
    get<bucket_idx>(buckets_).DeallocSlot(ptr);
  }
  template <typename T>
  T* ContinuousAlloc(size_t n) {
    constexpr size_t bucket_idx = BucketforSize(sizeof(T) * n);
    if constexpr (bucket_idx == -1ul) { // no bucket
      return vendor_.Alloc(sizeof(T) * n, alignof(T));
    }
    return get<bucket_idx>(buckets_).AllocSlot();
  }
  template <typename T>
  void ContinuousDealloc(T* ptr, size_t n) {
    constexpr size_t bucket_idx = BucketforSize(sizeof(T) * n);
    if constexpr (bucket_idx == -1ul) { // no bucket
      vendor_.Dealloc(ptr, sizeof(T) * n, alignof(T));
    }
    get<bucket_idx>(buckets_).DeallocSlot(ptr);
  }
  template <typename T, typename... Args>
  T* New(Args... args) {
    T* addr = DiscreteAlloc<T>();
    if constexpr (sizeof...(Args))
      new (addr) T(std::forward<Args...>(args...));
    else
      new (addr) T();
    return addr;
  }
  template <typename T>
  void Del(T* addr) {
    addr->~T();
    DiscreteDealloc(addr);
  }
  void Clear() {
    apply([](auto& bucket) { bucket.Clear(); }, buckets_);
  }

 private:
  /* Variables */
  Vendor vendor_;
  template <typename Seq>
  struct ArraytoTuple;
  template <size_t... Is>
  struct ArraytoTuple<index_sequence<Is...>> {
    using type = tuple<Bucket<kSlotSizes[Is]>...>;
  };
  ArraytoTuple<make_index_sequence<kSlotSizes.size()>>::type buckets_;

  /* Functions */
  consteval size_t BucketforSize(size_t size) {
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