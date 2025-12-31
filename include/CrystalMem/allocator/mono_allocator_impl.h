#ifndef CRYSTALMEM_MONO_ALLOCATOR_IMPL_H_
#define CRYSTALMEM_MONO_ALLOCATOR_IMPL_H_

#include "CrystalMem/types.h" // size_t, align_t

namespace crystal::mem {
template<typename T, typename Pool>
class MonoAllocatorImpl {
public:
  using value_type = T;
  /* Constructor */
  explicit MonoAllocatorImpl(Pool& pool) : pool_(pool) {}
  template <class U, typename P>
  friend class MonoAllocatorImpl; // friend for following constructor
  template <class U>
  explicit MonoAllocatorImpl(const MonoAllocatorImpl<U, Pool>& other):
    pool_(other.pool_) {}
  MonoAllocatorImpl(const MonoAllocatorImpl& other) : pool_(other.pool_) {}
  MonoAllocatorImpl& operator=(const MonoAllocatorImpl& rhs) {
    pool_ = rhs.pool_;
  }

  /* Functions */
  T* allocate(size_t n) {
    if (n != 1) [[unlikely]] return nullptr;
    return pool_.template Alloc<T>();
  }
  void deallocate(T* ptr, size_t n) {
    if (n != 1) [[unlikely]] return;
    pool_.template Dealloc<T>(ptr);
  }
  bool operator==(const MonoAllocatorImpl& other) {
    return pool_ == other.pool_;
  }
  bool operator!=(const MonoAllocatorImpl& other) {
    return pool_ != other.pool_;
  }
private:
  Pool& pool_;
};
} // namespace crystal::mem

#endif
