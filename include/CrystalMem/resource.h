#ifndef CRYSTALMEM_RESOURCE_H_
#define CRYSTALMEM_RESOURCE_H_

#include <expected> // std::expected
#include <string> // std::string

#include <CrystalUtil/log.h> // util::Logger

#include "CrystalMem/types.h" // size_t, align_t
#include "CrystalMem/options.h" // Source

namespace crystal::mem {

/* Resource Class */
template<Source>
class Resource {
public:
  /* Constructor */
  explicit Resource(util::Logger* logger = nullptr) : logger_(logger) {
    if (logger_) [[likely]] {
      logger_->Trace("Default resource({}) initailized.", static_cast<const void*>(this));
      logger_->Warn("Implementation for requested resource not found. Using default(os) resource.");
    }
  }
  /* Destructor */
  ~Resource() {
    Close();
  }
  /* Close */
  std::expected<void, std::string> Close() {
    alive_ = false;
    if (logger_) [[likely]] {
      logger_->Trace("Default resource({}) deleted.", static_cast<const void*>(this));
    }
    return {};
  }
  /* Validity */
  explicit operator bool() {return alive_;}
  /* Allocation Function */
  void* Alloc(size_t size, align_t align) {
    void* ptr = operator new(size, align);
    if (logger_) [[likely]] {
      logger_->Trace("Allocation at {} from default resource({}) of size {} & align {}.", ptr, static_cast<const void*>(this), size, static_cast<size_t>(align));
    }
    return ptr;
  }
  void Dealloc(void* ptr, size_t size) {
    if (logger_) [[likely]] {
      logger_->Trace("Deallocation at {} from default resource({}) of size {}.", ptr, static_cast<const void*>(this), size);
    }
    operator delete(ptr, size);
  }
private:
  bool alive_ = true;
  util::Logger* logger_;
};

} // namespace crystal::mem

#endif
