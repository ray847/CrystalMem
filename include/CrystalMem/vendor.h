#ifndef CRYSTALMEM_VENDOR_H_
#define CRYSTALMEM_VENDOR_H_

#include "resource/concept.h" // AnyResource
#include "resource/os.h" // OSResource
#include "vendor/concept.h" // AnyVendor

namespace crystal::mem {
  
/**
 * A memory vendor reveals a generic interface for allocating memory without the
 * knowledge of a specific type.
 * 
 * Functions:
 *  * void* Alloc(size_t size, align_t align);
 *  * void Dealloc(void* ptr, size_t size, align_t align);
 */
template <AnyResource Resource>
class Vendor {
 public:
  /* Constructor */
  Vendor(Resource& resource) : resource_(&resource) {}
  Vendor(const Vendor& other) : resource_(other.resource_) {}
  Vendor(Vendor&& other) : resource_(other.resource_) {}
  Vendor& operator=(const Vendor& rhs) = delete;
  Vendor& operator=(Vendor&& rhs) = delete;

  /* Functions */
  void* Alloc(size_t size, align_t align) {
    return resource_->Alloc(size, align);
  }
  void Dealloc(void* ptr, size_t size, align_t align) {
    resource_->Dealloc(ptr, size, align);
  }
  bool operator==(const Vendor& other) const {
    return resource_ == other.resource_;
  }
  bool operator!=(const Vendor& other) const {
    return resource_ != other.resource_;
  }

 private:
  Resource* resource_;
};
static_assert(AnyVendor<Vendor<OSResource>>);

} // 

#endif