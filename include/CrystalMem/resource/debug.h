#ifndef CRYSTALMEM_RESOURCE_DEBUG_H_
#define CRYSTALMEM_RESOURCE_DEBUG_H_

#include <iostream> // std::cout
#include <utility> // std::move

#include "../type.h" // align_t
#include "concept.h" // Resource

namespace crystal::mem {

using std::cout, std::endl, std::move;

template <AnyResource Resource>
class DebugResource {
 public:
  /* Constructor */
  DebugResource() : resource_() {
    cout << "Debug resource " << reinterpret_cast<void*>(this)
         << " constructed." << endl;
  }
  DebugResource(const DebugResource& other) = delete; // no copying
  DebugResource(DebugResource&& other) : resource_(move(other)) {
    cout << "Debug resource " << reinterpret_cast<void*>(this)
         << " move constructed." << endl;
  }
  DebugResource& operator=(const DebugResource& other) = delete; // no copying
  ~DebugResource() {
    cout << "Debug resource " << reinterpret_cast<void*>(this)
         << " destructed." << endl;
  }

  /* Functions */
  std::expected<void, std::string> Close() {
    cout << "Debug resource " << reinterpret_cast<void*>(this)
         << " closed." << endl;
    return resource_.Close();
  }
  operator bool() {
    return resource_;
  }
  void* Alloc(size_t size, align_t align) {
    void* ptr = resource_.Alloc(size, align);
    cout << "Debug resource " << reinterpret_cast<void*>(this)
         << " allocated memory at " << ptr << " with size: " << size
         << " & align: " << static_cast<size_t>(align) << "." << endl;
    return ptr;
  }
  void Dealloc(void* ptr, size_t size, align_t align) {
    cout << "Debug resource " << reinterpret_cast<void*>(this)
         << " deallocated memory at " << ptr << " with size: " << size
         << " & align: " << static_cast<size_t>(align) << "." << endl;
    resource_.Dealloc(ptr, size, align);
  }

 private:
  Resource resource_;
};

} // namespace crystal::mem

#endif
