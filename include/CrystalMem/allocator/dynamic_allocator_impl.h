#ifndef CRYSTALMEM_DYNAMIC_ALLOCATOR_IMPL_H_
#define CRYSTALMEM_DYNAMIC_ALLOCATOR_IMPL_H_

#include "CrystalMem/types.h" // size_t, align_t

namespace crystal::mem {
template<typename T, typename Pool>
class DynAllocatorImpl {
public:
  using value_type = T;
  /* Constructor */
  explicit DynAllocatorImpl(Pool& pool) : pool_(pool) {}
  template <class U, typename P>
  friend class DynAllocatorImpl; // friend for following constructor
  template <class U>
  explicit DynAllocatorImpl(const DynAllocatorImpl<U, Pool>& other):
    pool_(other.pool_) {}
  DynAllocatorImpl(const DynAllocatorImpl& other) : pool_(other.pool_) {}
  DynAllocatorImpl& operator=(const DynAllocatorImpl& rhs) {
    pool_ = rhs.pool_;
  }

  /* Functions */
  T* allocate(size_t n) {
    return pool_.template AllocDyn<T>(n);
  }
  void deallocate(T* ptr, size_t n) {
    pool_.template DeallocDyn<T>(ptr, n);
  }
  bool operator==(const DynAllocatorImpl& other) {
    return pool_ == other.pool_;
  }
  bool operator!=(const DynAllocatorImpl& other) {
    return pool_ != other.pool_;
  }
private:
  Pool& pool_;
};
} // namespace crystal::mem

#endif
