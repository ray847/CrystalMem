#ifndef CRYSTALMEM_OS_RESOURCE_H_
#define CRYSTALMEM_OS_RESOURCE_H_

#include <new> // new
#include <memory_resource> // std::pmr

#include <CrystalUtil/log.h> // util::Logger

#include "CrystalMem/types.h" // align_t
#include "CrystalMem/options.h" // Source
#include "CrystalMem/resource.h" // Resource

namespace crystal::mem {
template<>
class Resource<Source::OS> {
public:
  /* Constructor */
  explicit Resource(util::Logger* logger = nullptr):
    logger_(logger),
    resource() {
    if (logger_) [[likely]] {
      logger_->Trace("OS resource({}) initailized.", static_cast<const void*>(this));
    }
  }

  Resource(const Resource& other) = delete;
  Resource(Resource&& other):
    logger_(other.logger_) {
      logger_->Trace("OS resource({}) moved to resource({}).", static_cast<const void*>(&other), static_cast<const void*>(this));
    other.Close();
  }
  Resource& operator=(const Resource& other) = delete;
  ~Resource() {
    resource.release();
    Close();
  }

  std::expected<void, std::string> Close() {
    alive_ = false;
    if (logger_) [[likely]] {
      logger_->Trace("OS resource({}) deleted.", static_cast<const void*>(this));
    }
    return {};
  }

  explicit operator bool() {return alive_;}

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
  std::pmr::synchronized_pool_resource resource;
};
} // namespace crystal::mem

#endif
