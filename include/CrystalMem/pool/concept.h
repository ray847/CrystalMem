#ifndef CRYSTALMEM_POOL_CONCEPT_H_
#define CRYSTALMEM_POOL_CONCEPT_H_

#include <concepts>    // std::move_constructible
#include <type_traits> // std::move_assignable_v

namespace crystal::mem {

using std::same_as, std::convertible_to, std::move_constructible,
    std::is_move_assignable_v, std::move;

struct SomeObject {
  int a;
};
template <typename T>
concept AnyPool = requires(T pool, SomeObject* ptr, size_t n, int a) {
  /* Attributes */
  { T::kInMemoryOptimization } -> convertible_to<bool>;
  /* Functions */
  { pool.template DiscreteAlloc<SomeObject>() } -> same_as<SomeObject*>;
  { pool.template DiscreteDealloc<SomeObject>(ptr) } -> same_as<void>;
  { pool.template ContinuousAlloc<SomeObject>(n) } -> same_as<SomeObject*>;
  { pool.template ContinuousDealloc<SomeObject>(ptr, n) } -> same_as<void>;
  { pool.template New<SomeObject, int>(std::move(a)) } -> same_as<SomeObject*>;
  { pool.template Del<SomeObject>(ptr) } -> same_as<void>;
  { pool.Clear() } -> same_as<void>;
} && move_constructible<T> && is_move_assignable_v<T>;

} // namespace crystal::mem

#endif
