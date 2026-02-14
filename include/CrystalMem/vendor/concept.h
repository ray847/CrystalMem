#ifndef CRYSTALMEM_VENDOR_CONCEPT_H_
#define CRYSTALMEM_VENDOR_CONCEPT_H_

#include <concepts> // std::convertible_to

#include "../type.h"

namespace crystal::mem {

using std::convertible_to, std::same_as, std::copy_constructible;

template <typename T>
concept AnyVendor = requires(T vendor, size_t size, align_t align, void* ptr) {
  { vendor.Alloc(size, align) } -> same_as<void*>;
  { vendor.Dealloc(ptr, size, align) } -> same_as<void>;
} && copy_constructible<T>;

} // namespace crystal::mem

#endif