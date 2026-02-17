#ifndef CRYSTALMEM_CONTINUOUS_ALLOCATOR_H_
#define CRYSTALMEM_CONTINUOUS_ALLOCATOR_H_

#include "CrystalMem/type.h" // size_t, align_t
#include "CrystalMem/pool/concept.h" // AnyPool

namespace crystal::mem {

/**
 * A type of allocator that specializes in allocating arrays of objects.
 */
template<typename T, AnyPool Pool>
class ContinuousAllocator {
public:
  using value_type = T;
  /* Constructor */
  explicit ContinuousAllocator(Pool& pool) : pool_(pool) {}
  template <class U, typename P>
  friend class DynAllocatorImpl; // friend for following constructor
  template <class U>
  explicit ContinuousAllocator(const ContinuousAllocator<U, Pool>& other):
    pool_(other.pool_) {}
  ContinuousAllocator(const ContinuousAllocator& other) : pool_(other.pool_) {}
  ContinuousAllocator& operator=(const ContinuousAllocator& rhs) {
    pool_ = rhs.pool_;
  }

  /* Functions */
  T* allocate(size_t n) {
    return pool_.template ContinuousAlloc<T>(n);
  }
  void deallocate(T* ptr, size_t n) {
    pool_.template ContinuousDealloc<T>(ptr, n);
  }
  bool operator==(const ContinuousAllocator& other) {
    return &pool_ == &other.pool_;
  }
  bool operator!=(const ContinuousAllocator& other) {
    return &pool_ != &other.pool_;
  }
private:
  Pool& pool_;
};
} // namespace crystal::mem

#endif
