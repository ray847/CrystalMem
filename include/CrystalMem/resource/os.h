#ifndef CRYSTALMEM_RESOURCE_OS_H_
#define CRYSTALMEM_RESOURCE_OS_H_

#include <tuple> // std::ignore

#include "../type.h" // align_t
#include "concept.h" // Resource

namespace crystal::mem {

using std::ignore;

class OSResource {
 public:
  /* Constructor */
  explicit OSResource() = default;
  OSResource(const OSResource& other) = delete; // no copying
  OSResource(OSResource&& other) {
    ignore = Close(); // nothing can go wrong
  }
  OSResource& operator=(const OSResource& other) = delete; // no copying
  ~OSResource() {
    ignore = Close();
  }

  /* Functions */
  std::expected<void, std::string> Close() {
    alive_ = false;
    return {};
  }
  operator bool() {
    return alive_;
  }
  void* Alloc(size_t size, align_t align) {
    void* ptr = operator new(size, align);
    return ptr;
  }
  void Dealloc(void* ptr, size_t size, align_t align) {
    operator delete(ptr, size);
  }

 private:
  bool alive_ = true;
};
static_assert(AnyResource<OSResource>);

} // namespace crystal::mem

#endif
