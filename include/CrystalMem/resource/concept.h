#ifndef CRYSTALMEM_RESOURCE_CONCEPT_H_
#define CRYSTALMEM_RESOURCE_CONCEPT_H_

#include <concepts> // std::convertible_to
#include <expected> // std::expected
#include <string> // std::string

#include "../type.h" // size_t, align_t

namespace crystal::mem {

using std::convertible_to, std::same_as, std::expected, std::string;

/* Resource Template */
template <typename T>
concept AnyResource =
    requires(T resource, size_t size, align_t align, void* ptr) {
      { resource.Close() } -> same_as<expected<void, string>>;
      { resource.Alloc(size, align) } -> same_as<void*>;
      { resource.Dealloc(ptr, size, align) } -> same_as<void>;
    } && convertible_to<T, bool>;

} // namespace crystal::mem

#endif
