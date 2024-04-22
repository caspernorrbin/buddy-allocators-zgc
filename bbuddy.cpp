#include "bbuddy.hpp"
#include "bbuddy_instantiations.hpp"
#include "buddy_allocator.hpp"
#include "buddy_helper.hpp"
#include "buddy_instantiations.hpp"
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sys/mman.h>
#include <thread>

#ifdef DEBUG
#define BUDDY_DBG(x) std::cout << x << std::endl;
#else
#define BUDDY_DBG(x)                                                           \
  do {                                                                         \
  } while (0)
#endif

template <typename Config>
void BinaryBuddyAllocator<Config>::init_bitmaps(bool startFull) {
  const unsigned char freeBlocksPattern = 0x0; // 0x55 = 01010101
  const unsigned char sizeMapPattern =
      startFull ? 0xFF : 0x0; // 0xFF = 11111111

  BuddyAllocator<Config>::set_bitmaps(freeBlocksPattern, sizeMapPattern);
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
      BUDDY_DBG("curr_start: " << reinterpret_cast<void *>(curr_start));
      BuddyAllocator<Config>::push_free_list(curr_start, r, 0);
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

  const uint8_t start_block_level =
      BuddyAllocator<Config>::find_smallest_block_level(totalSize);
  uint8_t block_level = start_block_level;

  // const std::thread::id this_id = std::this_thread::get_id();
  // const std::hash<std::thread::id> hasher;
  const size_t threadOffset = 0;
  // const size_t threadOffset =
      // hasher(this_id) % BuddyAllocator<Config>::_numRegions;
  bool all_checked = true;

  for (int attempt = 0; attempt < 2; attempt++) {
    if (attempt == 1 && all_checked) {
      break;
    }

    for (size_t r_offset = threadOffset;
         r_offset < BuddyAllocator<Config>::_numRegions + threadOffset;
         r_offset++) {

      r = r_offset % BuddyAllocator<Config>::_numRegions;
      if (attempt == 0 &&
          !BuddyAllocator<Config>::_regionMutexes[r].try_lock()) {
        all_checked = false;
        continue;
      }
      if (attempt == 1) {
        BuddyAllocator<Config>::_regionMutexes[r].lock();
      }

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
              BuddyAllocator<Config>::set_split_block(r, block_idx, true);
              BUDDY_DBG("setting split_idx " << block_idx);
            }

            // Mark block as allocated
            if (block_level > 0) {
              BuddyAllocator<Config>::flip_allocated_block(
                  r, map_index(block_idx));
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
        goto block_found;
      }
      BuddyAllocator<Config>::_regionMutexes[r].unlock();
    }
  }

  return nullptr;

block_found:
  const uintptr_t block = BuddyAllocator<Config>::pop_free_list(r, block_level);

  // Mark block as allocated
  BuddyAllocator<Config>::flip_allocated_block(
      r, map_index(BuddyAllocator<Config>::block_index(block, r, block_level)));
  BUDDY_DBG("flipping final bit: " << map_index(
                BuddyAllocator<Config>::block_index(block, r, block_level)));

  // Store the size if not a bitmap
  if (!BuddyAllocator<Config>::_sizeMapIsBitmap &&
      BuddyAllocator<Config>::_sizeMapEnabled) {
    BuddyAllocator<Config>::set_level(block, r, block_level);
  }

  BuddyAllocator<Config>::_freeSizes[r] -=
      BuddyHelper::round_up_pow2(totalSize);
  BuddyAllocator<Config>::_regionMutexes[r].unlock();
  return reinterpret_cast<void *>(block);
}

// Deallocates a block of memory of the given size
template <typename Config>
void BinaryBuddyAllocator<Config>::deallocate_internal(void *ptr, size_t size) {
  auto block = reinterpret_cast<uintptr_t>(ptr);
  const uint8_t region = BuddyAllocator<Config>::get_region(block);
  uint8_t level = BuddyAllocator<Config>::get_level(block, size);
  BUDDY_DBG("deallocating! block "
            << BuddyAllocator<Config>::block_index(block, region, level)
            << " region " << static_cast<int>(region)
            << " level: " << static_cast<int>(level) << " size: " << size);

  // Mark block as free
  BUDDY_DBG("flipping bit: " << map_index(
                BuddyAllocator<Config>::block_index(block, region, level)));
  BuddyAllocator<Config>::flip_allocated_block(
      region,
      map_index(BuddyAllocator<Config>::block_index(block, region, level)));

  uintptr_t buddy = BuddyAllocator<Config>::get_buddy(
      block, BuddyAllocator<Config>::get_level(block, size));

  // Merge until buddy is no longer free
  while (level > 0 && !BuddyAllocator<Config>::block_is_allocated(
                          region, map_index(BuddyAllocator<Config>::block_index(
                                      buddy, region, level)))) {

    BUDDY_DBG("merging blocks: " << reinterpret_cast<void *>(block) << " and "
                                 << reinterpret_cast<void *>(buddy));

    // Mark above block as no longer split
    if (level < BuddyAllocator<Config>::_numLevels - 1 &&
        BuddyAllocator<Config>::_sizeMapIsBitmap &&
        BuddyAllocator<Config>::_sizeMapEnabled) {
      BuddyAllocator<Config>::set_split_block(
          region, BuddyAllocator<Config>::block_index(block, region, level),
          false);
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
      BuddyAllocator<Config>::flip_allocated_block(
          region,
          map_index(BuddyAllocator<Config>::block_index(block, region, level)));
      BUDDY_DBG("flipping bittt: " << map_index(
                    BuddyAllocator<Config>::block_index(block, region, level)));
    }
  }

  // Clear the final split block
  if (level < BuddyAllocator<Config>::_numLevels - 1 &&
      BuddyAllocator<Config>::_sizeMapIsBitmap &&
      BuddyAllocator<Config>::_sizeMapEnabled) {
    BuddyAllocator<Config>::set_split_block(
        region, BuddyAllocator<Config>::block_index(block, region, level),
        false);
    BUDDY_DBG("clearing split_idx "
              << BuddyAllocator<Config>::block_index(block, region, level));
  }

  BuddyAllocator<Config>::_freeSizes[region] += size;
  BuddyAllocator<Config>::push_free_list(block, region, level);
}

template <typename Config>
unsigned int BinaryBuddyAllocator<Config>::map_index(unsigned int index) {
  if (index == 0) {
    return 0;
  }

  return (index - 1) / 2 + 1;

  // return (index - 1) / 2;
}

// Gets the highest level that the pointer is aligned to
template <typename Config>
uint8_t BinaryBuddyAllocator<Config>::level_alignment(uintptr_t ptr,
                                                      uint8_t region,
                                                      uint8_t start_level) {
  if (start_level >= BuddyAllocator<Config>::_numLevels) {
    return BuddyAllocator<Config>::_numLevels - 1;
  }

  const uintptr_t offset = ptr - BuddyAllocator<Config>::region_start(region);
  uint8_t level = start_level;
  uintptr_t mask =
      (0x1UL << (BuddyAllocator<Config>::_maxBlockSizeLog2 - start_level)) - 1;
  while ((offset & mask) != 0) {
    level++;
    mask >>= 1U;
  }

  return level;
}
