#ifndef CRYSTALMEM_POOL_SAFE_BEST_FIT_FREE_MAP_H_
#define CRYSTALMEM_POOL_SAFE_BEST_FIT_FREE_MAP_H_

#include <CrystalBase/bitwise.h> // lowbit

#include <limits> // std::numeric_limits
#include <map> // std::map
#include <optional> // std::optional
#include <tuple> // std::tuple
#include <utility> // std::pair

#include "CrystalMem/type.h" // size_t

namespace crystal::mem {

using std::pair, std::optional, std::make_pair, std::map, std::tuple,
    std::make_tuple, std::numeric_limits, std::get, std::prev, std::less,
    std::move;

template <typename Allocator>
class SafeBestFitFreeMap {
 public:
  using allocator_type = Allocator;

  /* Constructor */
  SafeBestFitFreeMap(const Allocator& allocator) : free_nodes_(allocator) {
  }
  SafeBestFitFreeMap(const SafeBestFitFreeMap& other) = delete;
  SafeBestFitFreeMap(SafeBestFitFreeMap&& other) :
      free_nodes_(move(other.free_nodes_)) {
  }
  SafeBestFitFreeMap& operator=(const SafeBestFitFreeMap& rhs) = delete;
  SafeBestFitFreeMap& operator=(SafeBestFitFreeMap&& rhs) {
    free_nodes_ = move(rhs.free_nodes_);
  }

  /* Functions */
  /**
   * Insert a new free node in to the map.
   */
  void InsertNode(void* addr, size_t size) {
    free_nodes_[addr] = size;
  }
  /**
   * Try to allocate some memory with the input specifications.
   *
   * @return The allocated memory address, **OR** `-1ul` to indicate failure to
   * allocate.
   */
  void* Alloc(size_t size, align_t align) {
    void* best_fit_addr = reinterpret_cast<void*>(-1ul);
    tuple<size_t, size_t, size_t> best_fit_waste{
      numeric_limits<size_t>::max(),
      numeric_limits<size_t>::max(),
      numeric_limits<size_t>::max()
    };
    /* Look through the free nodes for the best fit. */
    for (auto& [node_addr, node_size] : free_nodes_) {
      if (auto waste = Fit(node_addr, node_size, size, align)) {
        if (*waste < best_fit_waste) {
          best_fit_addr = node_addr;
          best_fit_waste = *waste;
        }
      }
    }
    /* No fit. */
    if (reinterpret_cast<uint64_t>(best_fit_addr) == -1ul) {
      return reinterpret_cast<void*>(-1ul);
    }
    /* Split the best fit node. */
    size_t l_padding = get<1>(best_fit_waste);
    size_t r_padding = get<2>(best_fit_waste);
    /* Remove the node. */
    free_nodes_.erase(best_fit_addr);
    /* Add new nodes. */
    if (l_padding) free_nodes_[best_fit_addr] = l_padding;
    if (r_padding)
      free_nodes_[reinterpret_cast<void*>(
          reinterpret_cast<size_t>(best_fit_addr) + l_padding + size)] =
          r_padding;
    return reinterpret_cast<void*>(reinterpret_cast<size_t>(best_fit_addr)
                                   + l_padding);
  }
  /**
   * Deallocate a piece of memory.
   */
  void Dealloc(void* addr, size_t size, align_t align) {
    /* Look at left & right for merging. */
    size_t l_free = 0, r_free = 0;
    {
      void* l_addr;
      void* r_addr;

      auto r_node = free_nodes_.lower_bound(addr);
      if (r_node != free_nodes_.end()) {
        r_addr = r_node->first;
        size_t r_size = r_node->second;
        if (reinterpret_cast<size_t>(addr) + size
            == reinterpret_cast<size_t>(r_addr)) {
          r_free = r_size;
        }
      }
      if (r_node != free_nodes_.begin()) {
        auto l_node = prev(r_node);
        l_addr = l_node->first;
        size_t l_size = l_node->second;
        if (reinterpret_cast<size_t>(l_addr) + l_size
            == reinterpret_cast<size_t>(addr)) {
          l_free = l_size;
        }
      }

      if (l_free) free_nodes_.erase(l_addr);
      if (r_free) free_nodes_.erase(r_addr);
    }

    /* Insert new free node. */
    free_nodes_[reinterpret_cast<void*>(reinterpret_cast<size_t>(addr)
                                        - l_free)] = l_free + size + r_free;
  }

 private:
  /* Variables */
  map<void*, size_t, less<void*>, Allocator> free_nodes_;

  /* Functions */
  auto Fit(void* node_addr, size_t node_size, size_t size, align_t align)
      -> optional<tuple<size_t, size_t, size_t>> {
    uint64_t align_val = static_cast<uint64_t>(align);
    uint64_t current_addr_val = reinterpret_cast<uint64_t>(node_addr);
    uint64_t aligned_block_start_val =
        (current_addr_val + align_val - 1) & ~(align_val - 1);
    if (aligned_block_start_val + size > current_addr_val + node_size) {
      return {}; // No fit
    }
    size_t l_padding = aligned_block_start_val - current_addr_val;
    size_t r_padding = node_size - l_padding - size;
    return make_tuple(l_padding + r_padding, l_padding, r_padding);
  }
};

} // namespace crystal::mem

#endif