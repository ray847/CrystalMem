#ifndef CRYSTALMEM_DISCRETE_ALLOCATOR_H_
#define CRYSTALMEM_DISCRETE_ALLOCATOR_H_

#include "CrystalMem/type.h" // size_t, align_t
#include "CrystalMem/pool/concept.h" // AnyPool

namespace crystal::mem {

/**
 * A type of allocator that only allocates singular objects rather than array of
 * objects.
 */
template<typename T, AnyPool Pool>
class DiscreteAllocator {
public:
  using value_type = T;
  /* Constructor */
  explicit DiscreteAllocator(Pool& pool) : pool_(pool) {}
  template <class U, typename P>
  friend class MonoAllocatorImpl; // friend for following constructor
  template <class U>
  explicit DiscreteAllocator(const DiscreteAllocator<U, Pool>& other):
    pool_(other.pool_) {}
  DiscreteAllocator(const DiscreteAllocator& other) : pool_(other.pool_) {}
  DiscreteAllocator& operator=(const DiscreteAllocator& rhs) {
    pool_ = rhs.pool_;
  }

  /* Functions */
  T* allocate(size_t n) {
// clang-format off
    assert(n == 1 && "Discrete allocator can only allocate a single object.");
// clang-format on
    return pool_.template DiscreteAlloc<T>();
  }
  void deallocate(T* ptr, size_t n) {
// clang-format off
    assert(n == 1 && "Discrete allocator can only deallocate a single object.");
// clang-format on
    pool_.template DiscreteDealloc<T>(ptr);
  }
  bool operator==(const DiscreteAllocator& other) {
    return &pool_ == &other.pool_;
  }
  bool operator!=(const DiscreteAllocator& other) {
    return &pool_ != &other.pool_;
  }
private:
  Pool& pool_;
};
} // namespace crystal::mem

#endif
