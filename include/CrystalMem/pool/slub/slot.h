#ifndef CRYSTALMEM_POOL_SLUB_SLOT_H_
#define CRYSTALMEM_POOL_SLUB_SLOT_H_

#include <array> // std::array

namespace crystal::mem {

using std::array;

/**
 * A slot that potentially holds an object.
 * 
 * This slot is a part of SLUB slab allocator so it contains a free list node.
 * Memory Layout:
 * | free_list_node / data |
 */
template <size_t kSize>
union alignas(kSize) SLUBSlotNode {
// clang-format off
  static_assert(kSize >= sizeof(size_t), "Size of a slot must be larger than that of a free list node.");
// clang-format on
  std::array<std::byte, kSize> data;
  size_t free_nxt;
};

} // crystal::mem

#endif