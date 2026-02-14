#ifndef CRYSTALMEM_POOL_SLUB_BLOCK_H_
#define CRYSTALMEM_POOL_SLUB_BLOCK_H_

#include <cstdint> // uint64_t

#include <array> // std::array

#include <CrystalBase/bitwise.h> // lowbit

#include "slot.h" // SLUBSlot

namespace crystal::mem {

using std::array;

/**
 * A block that holds a continuous array of slots.
 *
 * Memory Layout:
 * | block links | block head | padding | slot_0 | slot_1 | ... | slot_n |
 *
 * Ownership responsibilities:
 *  Objects of this class have **NO** memory ownership responsibilities.
 */
template <size_t kSize, size_t kSlotSize>
requires (lowbit(kSize) == kSize) // size must be some power of 2
class alignas(kSize) SLUBBlockNode {
 public:
  using SlotNode = SLUBSlotNode<kSize>;

  /* Constants */
  /* Available size for slots. */
  static constexpr size_t kCapacity = (kSize - sizeof(size_t));
  static constexpr size_t kNSlots = kCapacity / kSlotSize;
  static_assert(kCapacity > 0, "Slot size too big for a single block.");

  /* Static Functions */
  /**
   * Use alignment to infer the block address.
   */
  static SLUBBlockNode* FromSlot(SlotNode* slot) {
    /* Some bit casting. */
    uint64_t bits = reinterpret_cast<uint64_t>(slot);
    bits &= ~(lowbit(kSize) - 1);
    return reinterpret_cast<SLUBBlockNode*>(bits);
  }

  /* Linked list pointers. */
  SLUBBlockNode* next_ = nullptr;
  SLUBBlockNode* prev_ = nullptr;

  /* Constructor */
  constexpr SLUBBlockNode() {
    head_.free_head = 0; // point to the first slot
    size_t i = 1;
    for (auto slot : slots_) slot.free_nxt = i++;
    slots_.back().free_nxt = -1ul;
  }

  /* Functions */
  /**
   * Allocate a slot from the block.
   */
  void* AllocSlot() {
    if (head_.free_head == -1ul) [[unlikely]]
      return nullptr;
    void* ptr = &slots_[head_.free_head];
    head_.free_head = slots_[head_.free_head].free_nxt;
    return ptr;
  }
  /**
   * Deallocate a slot from the block.
   *
   * This function does not check if the input address is actually in the
   * block.
   */
  void DeallocSlot(void* ptr) {
    size_t slot_idx =
        reinterpret_cast<SlotNode*>(ptr) - slots_.data();
    slots_[slot_idx].free_nxt = head_.free_head;
    head_.free_head = slot_idx;
  }
  bool Full() const {
    return head_.free_head == -1ul;
  }

 private:
  /* Block Head */
  struct Head {
    size_t free_head;
  };
  /* Variables */
  Head head_;
  array<SlotNode, kNSlots> slots_;

};

} // namespace crystal::mem

#endif