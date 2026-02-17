#ifndef CRYSTALMEM_VENDOR_ALLOCATOR_H_
#define CRYSTALMEM_VENDOR_ALLOCATOR_H_

#include "concept.h"

namespace crystal::mem {

template <typename T, AnyVendor V>
class VendorAllocator {
 public:
  using value_type = T;

  /* Constructor */
  VendorAllocator(const V& vendor) noexcept : vendor_(vendor) {
  }
  /* Cross Constructor */
  template <typename U, AnyVendor W>
  friend class VendorAllocator;
  template <typename U>
  VendorAllocator(const VendorAllocator<U, V>& other) noexcept :
      vendor_(other.vendor_) {
  }
  /* Assignment Operator */
  VendorAllocator& operator=(const VendorAllocator& rhs) {
    vendor_ = rhs.vendor_;
    return *this;
  }

  /* Functions */
  [[nodiscard]] T* allocate(size_t n) {
    return reinterpret_cast<T*>(
        vendor_.Alloc(sizeof(T) * n, static_cast<align_t>(alignof(T))));
  }
  void deallocate(T* ptr, size_t n) noexcept {
    vendor_.Dealloc(ptr, sizeof(T) * n, static_cast<align_t>(alignof(T)));
  }

  /* Accessor */
  const V& Vendor() const {
    return vendor_;
  }

 private:
  V vendor_;
};

template <typename T, typename U, AnyVendor Vendor>
bool operator==(const VendorAllocator<T, Vendor>& l,
                const VendorAllocator<U, Vendor>& r) {
  return l.vendor_ == r.vendor_;
}

} // namespace crystal::mem

#endif