#include "ibuddy.hpp"
#include "buddy_allocator.hpp"
#include "buddy_helper.hpp"
#include "buddy_instantiations.hpp"
#include "ibuddy_instantiations.hpp"
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sys/mman.h>

#ifdef DEBUG
#define BUDDY_DBG(x) std::cout << x << std::endl;
#else
#define BUDDY_DBG(x)                                                           \
  do {                                                                         \
  } while (0)
#endif

template <typename Config>
void IBuddyAllocator<Config>::init_bitmaps(bool startFull) {
  for (int i = 0; i < Config::numRegions; i++) {
    for (int j = 0; j < Config::allocedBitmapSize; j++) {
      BuddyAllocator<Config>::_freeBlocks[i][j] =
          startFull ? 0x0 : 0x55; // 0x55 = 01010101
    }
  }

  if (!BuddyAllocator<Config>::_sizeMapEnabled) {
    return;
  }

  if (Config::sizeBits == 0) { // Bitmap indicates split blocks
    if (startFull) {
      for (int i = 0; i < Config::numRegions; i++) {
        for (int j = 0; j < Config::sizeBitmapSize; j++) {
          BuddyAllocator<Config>::_sizeMap[i][j] = 0xFF; // 11111111
        }
      }
    } else {
      for (int i = 0; i < Config::numRegions; i++) {
        for (int j = 0; j < Config::sizeBitmapSize; j++) {
          BuddyAllocator<Config>::_sizeMap[i][j] = 0x00; // 00000000
        }
      }
    }
  }
}

template <typename Config>
IBuddyAllocator<Config>::IBuddyAllocator(void *start, int lazyThreshold,
                                         bool startFull)
    : BuddyAllocator<Config>(start, lazyThreshold, startFull) {
  // Initialize bitmaps
  init_bitmaps(startFull);

  // Insert all blocks into the free lists if starting empty
  if (!startFull) {
    for (int r = 0; r < Config::numRegions; r++) {
      uintptr_t curr_start = BuddyAllocator<Config>::region_start(r);
      BUDDY_DBG("start: " << reinterpret_cast<void *>(_start));
      BUDDY_DBG("curr_start: " << reinterpret_cast<void *>(curr_start));
      // auto *start_link = reinterpret_cast<double_link *>(curr_start);

      // Insert blocks into the free lists
      for (uint8_t lvl = BuddyAllocator<Config>::_numLevels - 1; lvl > 0;
           lvl--) {
        for (uintptr_t i =
                 curr_start +
                 (1U << static_cast<unsigned int>(
                      BuddyAllocator<Config>::_maxBlockSizeLog2 - lvl));
             i < curr_start + BuddyAllocator<Config>::_maxSize;
             i += (2U << static_cast<unsigned int>(
                       BuddyAllocator<Config>::_maxBlockSizeLog2 - lvl))) {
          BuddyAllocator<Config>::push_free_list(i, r, lvl);
        }
      }
      // BuddyHelper::push_back(&_freeList[r][0], start_link);
      BuddyAllocator<Config>::push_free_list(curr_start, r, 0);

      // Initialize
      // start_link->prev = &BuddyAllocator<Config>::_freeList[r][0];
      // start_link->next = &BuddyAllocator<Config>::_freeList[r][0];
    }
  }
}

// Creates a buddy allocator at the given address
template <typename Config>
IBuddyAllocator<Config> *
IBuddyAllocator<Config>::create(void *addr, void *start, int lazyThreshold,
                                bool startFull) {
  if (addr == nullptr) {
    addr = mmap(nullptr, sizeof(IBuddyAllocator), PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (addr == MAP_FAILED) {
      BUDDY_DBG("mmap failed");
      return nullptr;
    }
  }

  return new (addr) IBuddyAllocator(start, lazyThreshold, startFull);
}

// Allocates a block of memory of the given size
template <typename Config>
void *IBuddyAllocator<Config>::allocate_internal(size_t totalSize) {

  uint8_t region = 0;

  while (region < BuddyAllocator<Config>::_numRegions) {
    // Move down if the current level inside the region has been exhausted

    while (BuddyAllocator<Config>::free_list_empty(
               region, BuddyAllocator<Config>::_topLevel[region]) &&
           BuddyAllocator<Config>::_topLevel[region] <
               BuddyAllocator<Config>::_numLevels) {
      BuddyAllocator<Config>::_topLevel[region]++;
    }
    if (BuddyAllocator<Config>::size_of_level(
            BuddyAllocator<Config>::_topLevel[region]) >= totalSize) {
      break;
    }
    region++;
  }

  uint8_t level = BuddyAllocator<Config>::_topLevel[region];
  BUDDY_DBG("at region: " << (int)region << ", level: " << (int)level
                          << " with block size: " << size_of_level(level));

  // No free block large enough available
  if (region == BuddyAllocator<Config>::_numRegions ||
      level == BuddyAllocator<Config>::_numLevels ||
      BuddyAllocator<Config>::free_list_empty(region, level) ||
      static_cast<size_t>(BuddyAllocator<Config>::size_of_level(level)) <
          totalSize) {
    BUDDY_DBG("No free blocks available");
    return nullptr;
  }

  // Get the first free block
  auto block = BuddyAllocator<Config>::pop_free_list(region, level);

  const int block_level =
      BuddyAllocator<Config>::find_smallest_block_level(totalSize);

  // Multiple blocks needed, align the block for its level
  const uintptr_t block_left =
      BuddyAllocator<Config>::align_left(block, block_level);
  BUDDY_DBG("aligned block " << block_index(block_left, region, level) << " at "
                             << block_left << " level: " << level);

  // Mark above blocks as split
  if (BuddyAllocator<Config>::_sizeMapEnabled) {
    if (BuddyAllocator<Config>::_sizeMapIsBitmap) {
      BuddyAllocator<Config>::split_bits(block, region, level, block_level);
    } else {
      BuddyAllocator<Config>::set_level(block_left, region, block_level);
    }
  }

  // Mark the block as allocated
  BuddyHelper::clear_bit(
      BuddyAllocator<Config>::_freeBlocks[region],
      BuddyAllocator<Config>::block_index(block, region, level));
  BUDDY_DBG("cleared block " << block_index(block, region, level) << " at "
                             << block << " level: " << (int)level
                             << " region: " << (int)region);

  // Can fit in one block
  if (totalSize <= static_cast<size_t>(BuddyAllocator<Config>::_minSize)) {
    BuddyAllocator<Config>::_freeSize -= BuddyAllocator<Config>::_minSize;
    return reinterpret_cast<void *>(block);
  }

  level = block_level;

  // Clear all smaller levels
  const int start_level =
      BuddyAllocator<Config>::find_smallest_block_level(totalSize);
  const size_t new_size = BuddyHelper::round_up_pow2(totalSize);

  BUDDY_DBG("STARTING LEVEL: " << start_level << " SIZE: " << totalSize);

  for (int i = start_level + 1; i < BuddyAllocator<Config>::_numLevels; i++) {
    const unsigned int start_block_idx =
        BuddyAllocator<Config>::block_index(block_left, region, i);
    BUDDY_DBG("start block index: " << start_block_idx);
    for (unsigned int j = start_block_idx;
         j < start_block_idx + BuddyAllocator<Config>::num_blocks(new_size, i);
         j++) {
      BUDDY_DBG("clearing block " << j << " level: " << i);
      BuddyHelper::clear_bit(BuddyAllocator<Config>::_freeBlocks[region], j);
    }
  }

  // Clear free list
  for (uintptr_t i = block_left; i < block_left + new_size;
       i += BuddyAllocator<Config>::size_of_level(
           BuddyAllocator<Config>::_numLevels - 1)) {
    if (i != block) {
      BUDDY_DBG("clearing free list at " << i - region_start(region));
      BuddyHelper::list_remove(reinterpret_cast<double_link *>(i));
    }
  }

  BuddyAllocator<Config>::_freeSize -= new_size;

  return reinterpret_cast<void *>(block_left);
}

// Deallocates a single (smallest) block of memory
template <typename Config>
void IBuddyAllocator<Config>::deallocate_single(uintptr_t ptr) {

  const uint8_t region = BuddyAllocator<Config>::get_region(ptr);
  uint8_t level = BuddyAllocator<Config>::_numLevels - 1;
  int block_idx = BuddyAllocator<Config>::block_index(ptr, region, level);
  int buddy_idx = BuddyAllocator<Config>::buddy_index(ptr, region, level);

  BUDDY_DBG("block index: "
            << block_idx << ", buddy index: " << buddy_idx << ", is set: "
            << BuddyHelper::bit_is_set(_freeBlocks[region], buddy_idx));

  // While the buddy is free, go up a level
  while (level > 0 &&
         BuddyHelper::bit_is_set(BuddyAllocator<Config>::_freeBlocks[region],
                                 buddy_idx)) {
    level--;
    block_idx = BuddyAllocator<Config>::block_index(ptr, region, level);
    buddy_idx = BuddyAllocator<Config>::buddy_index(ptr, region, level);
    BUDDY_DBG("new block index: " << block_idx << " buddy index: " << buddy_idx
                                  << " level: " << level);

    if (BuddyAllocator<Config>::_sizeMapIsBitmap &&
        BuddyAllocator<Config>::_sizeMapEnabled) {
      BuddyHelper::clear_bit(BuddyAllocator<Config>::_sizeMap[region],
                             block_idx);
      BUDDY_DBG("clearing split_idx " << block_idx);
    }
  }

  // Mark the block as free and insert it into the free list
  BuddyAllocator<Config>::push_free_list(ptr, region, level);
  if (level > 0) {

    BuddyHelper::set_bit(
        BuddyAllocator<Config>::_freeBlocks[region],
        BuddyAllocator<Config>::block_index(ptr, region, level));
    BUDDY_DBG("inserting " << ptr - region_start(region)
                           << " at level: " << (int)level << " marking bit: "
                           << block_index(ptr, region, level) << " as free");
  }

  // Set the level of the topmost free block
  if (level < BuddyAllocator<Config>::_topLevel[region]) {
    BuddyAllocator<Config>::_topLevel[region] = level;
  }

  BuddyAllocator<Config>::_freeSize += BuddyAllocator<Config>::_minSize;
}

// Deallocates a range of blocks of memory as if they were the smallest block
template <typename Config>
void IBuddyAllocator<Config>::deallocate_range(void *ptr, size_t size) {
  const auto start = reinterpret_cast<uintptr_t>(ptr);
  const uintptr_t aligned_start = BuddyAllocator<Config>::align_left(
      start + BuddyAllocator<Config>::_minSize - 1,
      BuddyAllocator<Config>::_numLevels - 1);
  const uintptr_t end = BuddyAllocator<Config>::align_left(
      start + size, BuddyAllocator<Config>::_numLevels - 1);
  for (uintptr_t i = aligned_start; i < end;
       i += BuddyAllocator<Config>::size_of_level(
           BuddyAllocator<Config>::_numLevels - 1)) {

    BUDDY_DBG("deallocating block at " << i);
    deallocate_single(i);
  }
}

// Deallocates a block of memory of the given size
template <typename Config>
void IBuddyAllocator<Config>::deallocate_internal(void *ptr, size_t size) {
  return deallocate_range(ptr, BuddyHelper::round_up_pow2(size));
}

// Fills the memory, marking all blocks as allocated
// This overwrites previous allocations
template <typename Config> void IBuddyAllocator<Config>::fill() {
  init_bitmaps(true);
  BuddyAllocator<Config>::init_free_lists();
  BuddyAllocator<Config>::_freeSize = 0;
}
