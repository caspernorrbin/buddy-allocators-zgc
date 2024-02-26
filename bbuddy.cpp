#include "bbuddy.hpp"
#include "bbuddy_instantiations.hpp"
#include "buddy_allocator.hpp"
#include "buddy_helper.hpp"
#include "buddy_instantiations.hpp"
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
void BinaryBuddyAllocator<Config>::init_bitmaps(bool startFull) {
  for (int i = 0; i < Config::numRegions; i++) {
    for (int j = 0; j < Config::allocedBitmapSize; j++) {
      BuddyAllocator<Config>::_freeBlocks[i][j] = 0x0;
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
BinaryBuddyAllocator<Config>::BinaryBuddyAllocator(void *start,
                                                   int lazyThreshold,
                                                   bool startFull)
    : BuddyAllocator<Config>(start, lazyThreshold, startFull) {
  // Initialize bitmaps
  init_bitmaps(startFull);

  // Insert all blocks into the free lists if starting empty
  if (!startFull) {
    for (int r = 0; r < Config::numRegions; r++) {
      uintptr_t curr_start = BuddyAllocator<Config>::region_start(r);
      // BUDDY_DBG("start: " << reinterpret_cast<void *>(_start));
      BUDDY_DBG("curr_start: " << reinterpret_cast<void *>(curr_start));
      BuddyAllocator<Config>::push_free_list(curr_start, r, 0);

      // Initialize
      // start_link->prev = &BuddyAllocator<Config>::_freeList[r][0];
      // start_link->next = &BuddyAllocator<Config>::_freeList[r][0];
    }
  }
}

// Creates a buddy allocator at the given address
template <typename Config>
BinaryBuddyAllocator<Config> *
BinaryBuddyAllocator<Config>::create(void *addr, void *start, int lazyThreshold,
                                     bool startFull) {
  if (addr == nullptr) {
    addr = mmap(nullptr, sizeof(BinaryBuddyAllocator), PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (addr == MAP_FAILED) {
      BUDDY_DBG("mmap failed");
      return nullptr;
    }
  }

  return new (addr) BinaryBuddyAllocator(start, lazyThreshold, startFull);
}

// Allocates a block of memory of the given size
template <typename Config>
void *BinaryBuddyAllocator<Config>::allocate_internal(size_t totalSize) {

  uint8_t r = 0;
  bool found = false;

  const int start_block_level =
      BuddyAllocator<Config>::find_smallest_block_level(totalSize);
  int block_level = start_block_level;

  for (r = 0; r < BuddyAllocator<Config>::_numRegions; r++) {
    block_level = start_block_level;

    while (!found) {
      if (!BuddyAllocator<Config>::free_list_empty(r, block_level)) {
        found = true;
      } else if (block_level > 0) {
        block_level--;

        if (!BuddyAllocator<Config>::free_list_empty(r, block_level)) {
          // Larger block found, split it
          const uintptr_t block =
              BuddyAllocator<Config>::pop_free_list(r, block_level);

          // Mark block as split
          const unsigned int block_idx =
              BuddyAllocator<Config>::block_index(block, r, block_level);
          if (BuddyAllocator<Config>::_sizeMapIsBitmap &&
              BuddyAllocator<Config>::_sizeMapEnabled) {
            BuddyHelper::set_bit(BuddyAllocator<Config>::_sizeMap[r],
                                 block_idx);
            BUDDY_DBG("setting split_idx " << block_idx);
          }

          // Mark block as allocated
          if (block_level > 0) {
            BuddyHelper::flip_bit(BuddyAllocator<Config>::_freeBlocks[r],
                                  map_index(block_idx));
            BUDDY_DBG("flipping bit: " << map_index(block_idx));
          }

          // Split the block into two
          const uintptr_t buddy =
              block + BuddyAllocator<Config>::size_of_level(block_level + 1);

          // Insert the two blocks into the free list
          BuddyAllocator<Config>::push_free_list(block, r, block_level + 1);
          BuddyAllocator<Config>::push_free_list(buddy, r, block_level + 1);

          block_level = start_block_level;
        }
      } else {
        break;
      }
    }

    if (found) {
      break;
    }
  }

  if (!found) {
    return nullptr;
  }

  const uintptr_t block = BuddyAllocator<Config>::pop_free_list(r, block_level);

  // Mark block as allocated
  BuddyHelper::flip_bit(
      BuddyAllocator<Config>::_freeBlocks[r],
      map_index(BuddyAllocator<Config>::block_index(block, r, block_level)));
  BUDDY_DBG("flipping final bit: " << map_index(
                BuddyAllocator<Config>::block_index(block, r, block_level)));

  // Store the size if not a bitmap
  if (!BuddyAllocator<Config>::_sizeMapIsBitmap &&
      BuddyAllocator<Config>::_sizeMapEnabled) {
    BuddyAllocator<Config>::set_level(block, r, block_level);
  }

  BuddyAllocator<Config>::_freeSize -= BuddyHelper::round_up_pow2(totalSize);
  return reinterpret_cast<void *>(block);
}

// Deallocates a block of memory of the given size
template <typename Config>
void BinaryBuddyAllocator<Config>::deallocate_internal(void *ptr, size_t size) {
  auto block = reinterpret_cast<uintptr_t>(ptr);
  const uint8_t region = BuddyAllocator<Config>::get_region(block);
  uint8_t level = BuddyAllocator<Config>::get_level(block);

  // Mark block as free
  BUDDY_DBG("flipping bit: " << map_index(
                BuddyAllocator<Config>::block_index(block, region, level)));
  BuddyHelper::flip_bit(
      BuddyAllocator<Config>::_freeBlocks[region],
      map_index(BuddyAllocator<Config>::block_index(block, region, level)));

  uintptr_t buddy = BuddyAllocator<Config>::get_buddy(
      block, BuddyAllocator<Config>::get_level(block));

  // Merge until buddy is no longer free
  while (level > 0 &&
         !BuddyHelper::bit_is_set(BuddyAllocator<Config>::_freeBlocks[region],
                                  map_index(BuddyAllocator<Config>::block_index(
                                      buddy, region, level)))) {

    BUDDY_DBG("merging blocks: " << reinterpret_cast<void *>(block) << " and "
                                 << reinterpret_cast<void *>(buddy));

    // Mark above block as no longer split
    if (level < BuddyAllocator<Config>::_numLevels - 1 &&
        BuddyAllocator<Config>::_sizeMapIsBitmap &&
        BuddyAllocator<Config>::_sizeMapEnabled) {
      BuddyHelper::clear_bit(
          BuddyAllocator<Config>::_sizeMap[region],
          BuddyAllocator<Config>::block_index(block, region, level));
      BUDDY_DBG("clearing split_idx "
                << BuddyAllocator<Config>::block_index(block, region, level));
    }

    // Remove buddy from free list
    BuddyHelper::list_remove(reinterpret_cast<double_link *>(buddy));

    // Align to the left block
    if (buddy < block) {
      block = buddy;
    }

    level--;

    // Get and mark the next buddy
    if (level > 0) {
      buddy = BuddyAllocator<Config>::get_buddy(block, level);
      BuddyHelper::flip_bit(
          BuddyAllocator<Config>::_freeBlocks[region],
          map_index(BuddyAllocator<Config>::block_index(block, region, level)));
      BUDDY_DBG("flipping bit: " << map_index(
                    BuddyAllocator<Config>::block_index(block, region, level)));
    }
  }

  // Clear the final split block
  if (level < BuddyAllocator<Config>::_numLevels - 1 &&
      BuddyAllocator<Config>::_sizeMapIsBitmap &&
      BuddyAllocator<Config>::_sizeMapEnabled) {
    BuddyHelper::clear_bit(
        BuddyAllocator<Config>::_sizeMap[region],
        BuddyAllocator<Config>::block_index(block, region, level));
    BUDDY_DBG("clearing split_idx "
              << BuddyAllocator<Config>::block_index(block, region, level));
  }

  BuddyAllocator<Config>::_freeSize += size;
  BuddyAllocator<Config>::push_free_list(block, region, level);
}

template <typename Config>
void BinaryBuddyAllocator<Config>::deallocate_range(void *ptr, size_t size) {
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
    deallocate_internal(reinterpret_cast<void *>(i),
                        BuddyAllocator<Config>::_minSize);
  }
}

// Fills the memory, marking all blocks as allocated
// This overwrites previous allocations
template <typename Config> void BinaryBuddyAllocator<Config>::fill() {
  init_bitmaps(true);
  BuddyAllocator<Config>::init_free_lists();
  BuddyAllocator<Config>::_freeSize = 0;
}

template <typename Config>
unsigned int BinaryBuddyAllocator<Config>::map_index(unsigned int index) {
  if (index == 0) {
    return 0;
  }

  return (index - 1) / 2 + 1;

  // return (index - 1) / 2;
}
