#ifndef CRYSTALMEM_POOL_SLUB_BUCKET_H_
#define CRYSTALMEM_POOL_SLUB_BUCKET_H_

#include "CrystalMem/vendor/concept.h" // Vendor
#include "block.h" // SLUBBlock

namespace crystal::mem {

using std::allocator_traits;

/**
 * A bucket of blocks that have the same slot size.
 *
 * All blocks are of the same size and are organized in a linked list data
 * structure.
 * 
 * Memory Responsibilities:
 * This class is responsible for releasing all the block nodes.
 */
template <size_t kBlockSize, size_t kSlotSize, AnyVendor Vendor>
class SLUBBucket {
 public:
  using BlockNode = SLUBBlockNode<kBlockSize, kSlotSize>;

  /* Constructor */
  SLUBBucket(Vendor& vendor = {}) : vendor_(vendor) {
  }
  /* Destructor */
  ~SLUBBucket() {
    Clear();
  }
  /* No Copying */
  SLUBBucket(const SLUBBucket& other) = delete;
  /* Move Constructor */
  SLUBBucket(SLUBBucket&& other) :
      vendor_(other.vendor_), block_head_(other.block_head_) {
    other.block_head_ = nullptr;
  }
  /* No Copying */
  SLUBBucket& operator=(const SLUBBucket& rhs) = delete;
  /* Move Assignment */
  SLUBBucket& operator=(SLUBBucket&& rhs) {
    vendor_ = rhs.vendor_;
    block_head_ = rhs.block_head_;
    rhs.block_head_ = nullptr;
  }

  /* Functions */
  /**
   * Allocate a slot with size of `kSlotSize`.
   */
  [[nodiscard]] void* AllocSlot() {
    BlockNode& block = AvailableBlock();
    void* slot = block.AllocSlot();
    return slot;
  }
  /**
   * Deallocate a slot with the input address.
   */
  void DeallocSlot(void* addr) {
    BlockNode* block =
        BlockNode::FromSlot(reinterpret_cast<BlockNode::SlotNode*>(addr));
    block->DeallocSlot(addr);
    /* Move block to front. */
    MovetoFront(*block);
  }
  /**
   * Clear the bucket by deallocating all blocks.
   */
  void Clear() {
    while (block_head_) {
      BlockNode* nxt = block_head_->next_;
      vendor_.Dealloc(block_head_, sizeof(BlockNode),  alignof(BlockNode));
      block_head_ = nxt;
    }
  }

 private:
  /* Variables */
  Vendor vendor_;
  /**
   * A **LIST** of blocks.
   *
   * @note It is assumed that if there are non-filled blocks among all of the
   * blocks, then one of them is at the front of the list.
   */
  BlockNode* block_head_ = nullptr;

  /* Functions */
  /**
   * Get an available block.
   *
   * If there are no available blocks, this function will allocate a new block.
   */
  BlockNode& AvailableBlock() {
    /* Expand blocks. */
    if (!block_head_ || block_head_->Full()) [[unlikely]] {
      BlockNode* curr_head = block_head_;
      BlockNode* new_block = reinterpret_cast<BlockNode*>(vendor_.Alloc(
          sizeof(BlockNode), static_cast<align_t>(alignof(BlockNode))));
      new_block->next_ = curr_head;
      new_block->prev_ = nullptr;
      curr_head->prev_ = new_block;
      block_head_ = new_block;
    }
    return *block_head_;
  }
  void MovetoFront(BlockNode& node) {
    BlockNode* curr = &node;
    BlockNode* prev = node.prev_;
    BlockNode* next = node.next_;
    prev->next_ = next;
    next->prev_ = prev;
    curr->next_ = block_head_;
    curr->prev_ = nullptr;
    block_head_->prev_ = curr;
    block_head_ = curr;
  }
};

} // namespace crystal::mem

#endif