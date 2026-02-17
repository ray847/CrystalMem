#ifndef CRYSTALMEM_POOL_SAFE_BEST_FIT_BLOCK_H_
#define CRYSTALMEM_POOL_SAFE_BEST_FIT_BLOCK_H_

#include <cstddef> // std::byte

#include <array> // std::array

namespace crystal::mem {

using std::array, std::byte;

template <size_t kSize>
struct alignas(kSize) SafeBestFitBlock {
  /* Varaibles */
  array<byte, kSize> data;

  /* Constructor */
  SafeBestFitBlock() {
    static_assert(sizeof(SafeBestFitBlock) == kSize);
  }
};

} //

#endif