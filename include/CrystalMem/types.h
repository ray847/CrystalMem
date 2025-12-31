#ifndef CRYSTALMEM_TYPES_H_
#define CRYSTALMEM_TYPES_H_

#include <cstdint> // uint32_t

#include <new> // std::align_val_t

namespace crystal::mem {

using size_t = std::size_t;
using ssize_t = uint32_t;
using align_t = std::align_val_t;

constexpr size_t operator""_B(unsigned long long n_bytes) {
  return n_bytes;
}

constexpr size_t operator""_kB(unsigned long long n_bytes) {
  return 1024 * n_bytes;
}
} // namespace crystal::mem

#endif
