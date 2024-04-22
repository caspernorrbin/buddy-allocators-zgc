#include "btbuddy.hpp"
#include "btbuddy_instantiations.hpp"
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
void BTBuddyAllocator<Config>::init_bitmaps(bool startFull) {
  const unsigned char freeBlocksPattern = 0x0; // 0x55 = 01010101
  const unsigned char sizeMapPattern =
      startFull ? 0xFF : 0x0; // 0xFF = 11111111

  BuddyAllocator<Config>::set_bitmaps(freeBlocksPattern, sizeMapPattern);

  for (int r = 0; r < Config::numRegions; r++) {
    if (startFull) {
      for (unsigned int i = 0;
           i < (1U << BuddyAllocator<Config>::_numLevels) - 1; i++) {
        set_tree(r, i, 0);
      }
    } else {
      for (int l = 0; l < BuddyAllocator<Config>::_numLevels; l++) {
        const int tree_height = BuddyAllocator<Config>::_numLevels - l;
        for (unsigned int i = BuddyAllocator<Config>::index_of_level(l);
             i < BuddyAllocator<Config>::index_of_level(l + 1); i++) {
          set_tree(r, i, tree_height);
        }
      }
    }
  }
}

template <typename Config>
BTBuddyAllocator<Config>::BTBuddyAllocator(void *start, int lazyThreshold,
                                           bool startFull)
    : BuddyAllocator<Config>(start, lazyThreshold, startFull) {

  // Insert all blocks into the free lists if starting empty
  if (!startFull) {
    for (int r = 0; r < Config::numRegions; r++) {
      uintptr_t curr_start = BuddyAllocator<Config>::region_start(r);
      BUDDY_DBG("curr_start: " << reinterpret_cast<void *>(curr_start));
      BuddyAllocator<Config>::push_free_list(curr_start, r, 0);
    }
  }

  _btBits[Config::numLevels - 1] = 1;
  _btBits[Config::numLevels - 2] = 2;
  _btBits[Config::numLevels - 3] = 2;
  for (int i = Config::numLevels - 4; i >= 0; i--) {
    _btBits[i] = 8;
  }
  // for (int i = Config::numLevels - 4; i >= Config::numLevels - 16 && i >= 0;
  //      i--) {
  //   _btBits[i] = 4;
  // }
  // for (int i = Config::numLevels - 16; i >= 0; i--) {
  //   _btBits[i] = 8;
  // }

  unsigned int start_offset = 0;
  for (int i = 0; i < Config::numLevels - 1; i++) {
    unsigned int level_blocks = 1U << i;
    unsigned int level_size = level_blocks * _btBits[i];
    // std::cout << "Level " << i << " bits: " << (int)_btBits[i]
    // << " size: " << level_size << std::endl;
    // round up to nearest multiple of 8
    level_size = (level_size + 7) & ~7;
    // std::cout << "Level " << i << " blocks: " << level_blocks
    // << " size: " << level_size << std::endl;
    start_offset += level_size / 8;
    _levelOffsets[i + 1] = start_offset;
  }

  // print start_offset
  // for (int i = 0; i < Config::numLevels; i++) {
  //   std::cout << "Level " << i << " offset: " << _levelOffsets[i] <<
  //   std::endl;
  // }

  // Initialize bitmaps
  init_bitmaps(startFull);
}

// Creates a buddy allocator at the given address
template <typename Config>
BTBuddyAllocator<Config> *
BTBuddyAllocator<Config>::create(void *addr, void *start, int lazyThreshold,
                                 bool startFull) {
  if (addr == nullptr) {
    addr = mmap(nullptr, sizeof(BTBuddyAllocator), PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (addr == MAP_FAILED) {
      BUDDY_DBG("mmap failed");
      return nullptr;
    }
  }

  return new (addr) BTBuddyAllocator(start, lazyThreshold, startFull);
}

// [num_bits][bit_offset]
static unsigned char bitmask_table[4][8] = {
    {0b11111110, 0b11111101, 0b11111011, 0b11110111, 0b11101111, 0b11011111,
     0b10111111, 0b01111111},
    {0b11111100, 0b0, 0b11110011, 0b0, 0b11001111, 0b0, 0b00111111, 0b0},
    {0b0, 0b0, 0b0, 0b0, 0b0, 0b0, 0b0, 0b0},
    {0b11110000, 0b0, 0b0, 0b0, 0b00001111, 0b0, 0b0, 0b0}};

template <typename Config>
inline void BTBuddyAllocator<Config>::set_tree(uint8_t region,
                                               unsigned int index,
                                               unsigned char value) {

  // _btTree[region][index] = value;
  // return;
  const uint8_t level = BuddyAllocator<Config>::level_of_index(index);
  if (level < BuddyAllocator<Config>::_numLevels - 3) {
    _btTree[region][index] = value;
    return;
  }
  const unsigned int level_start = BuddyAllocator<Config>::index_of_level(level);
  // std::cout << "index: " << index << " level: " << static_cast<int>(level)
  // << " value: " << (int)value << std::endl;
  const unsigned int offset = _levelOffsets[level];
  const unsigned char num_bits = _btBits[level];

  const unsigned int byte_offset =
      offset + (index - level_start) * num_bits / 8;
  const unsigned int bit_offset = (index - level_start) * num_bits % 8;

  // std::cout << "byte_offset: " << byte_offset
  // << " bit_offset: " << static_cast<int>(bit_offset) << std::endl;

  // Replace num_bits bits starting from bit_offset with value
  unsigned char *byte = &_btTree[region][byte_offset];
  *byte =
      (*byte & bitmask_table[num_bits - 1][bit_offset]) | (value << bit_offset);

  // print binary representation of byte
  // for (int i = 7; i >= 0; i--) {
  //   std::cout << ((*byte >> i) & 1);
  // }
  // std::cout << std::endl;
}

template <typename Config>
inline unsigned char BTBuddyAllocator<Config>::get_tree(uint8_t region,
                                                        unsigned int index) {

  // return _btTree[region][index];
  const uint8_t level = BuddyAllocator<Config>::level_of_index(index);
  if (level < BuddyAllocator<Config>::_numLevels - 3) {
    return _btTree[region][index];
  }

  const unsigned int level_start = BuddyAllocator<Config>::index_of_level(level);

  const unsigned int offset = _levelOffsets[level];
  const unsigned char num_bits = _btBits[level];

  // const unsigned int index_offset = index - level_start;
  const unsigned int byte_offset =
      offset + (((index - level_start) * num_bits) >> 3U);
  // const unsigned int bit_offset = index_offset * num_bits % 8;
  const unsigned int bit_offset = ((index - level_start) * num_bits) & 0x7U;

  const unsigned char byte = _btTree[region][byte_offset];
  // const unsigned char value = (byte >> bit_offset) & (0xFF >> (8 -
  // num_bits));
  // const unsigned char value = (byte >> bit_offset) & ((1U << num_bits) - 1U);
  return (byte >> bit_offset) & (0xFF >> (8 - num_bits));

  // return value;
}

// Function to read the CPU cycle counter
// inline unsigned long long rdtsc() {
//   unsigned int low, high;
//   asm volatile("rdtsc" : "=a"(low), "=d"(high));
//   return ((unsigned long long)high << 32) | low;
// }

// Allocates a block of memory of the given size
template <typename Config>
void *BTBuddyAllocator<Config>::allocate_internal(size_t totalSize) {

  // auto start = rdtsc();
  const uint8_t block_height = tree_height(totalSize);
  BUDDY_DBG("block_height: " << block_height);

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

    uint8_t r = 0;
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

      int tree_index = 0;

      if (get_tree(r, tree_index) < block_height) {
        BuddyAllocator<Config>::_regionMutexes[r].unlock();
        continue;
      }

      for (uint8_t l = BuddyAllocator<Config>::_numLevels; l > block_height;
           l--) {
        tree_index = 2 * tree_index + 1;
        if (get_tree(r, tree_index) < block_height) {
          tree_index += 1;
        }
        // const unsigned char left_value = get_tree(r, 2 * tree_index + 1);
        // const unsigned char right_value = get_tree(r, 2 * tree_index + 2);
        // chose the child with the lowest value that is at least block_height
        // if (left_value >= block_height && right_value >= block_height) {
        //   if (left_value <= right_value) {
        //     tree_index = 2 * tree_index + 1;
        //   } else {
        //     tree_index = 2 * tree_index + 2;
        //   }
        // } else if (left_value >= block_height) {
        //   tree_index = 2 * tree_index + 1;
        // } else if (right_value >= block_height) {
        //   tree_index = 2 * tree_index + 2;
        // } else {
        //   BUDDY_DBG("No block found");
        //   BuddyAllocator<Config>::_regionMutexes[r].unlock();
        //   return nullptr;
        // }
      }

      BUDDY_DBG("tree_index: " << tree_index);
      set_tree(r, tree_index, 0);

      const uintptr_t block =
          BuddyAllocator<Config>::get_address(r, tree_index);
      BUDDY_DBG("block: " << reinterpret_cast<void *>(block));
      // Store the size if not a bitmap
      if (!BuddyAllocator<Config>::_sizeMapIsBitmap &&
          BuddyAllocator<Config>::_sizeMapEnabled) {
        BuddyAllocator<Config>::set_level(
            block, r, BuddyAllocator<Config>::_numLevels - block_height);
      }

      // Set the index to the parent
      tree_index = (tree_index - 1) / 2;

      for (uint8_t l = block_height; l < BuddyAllocator<Config>::_numLevels;
           l++) {
        const int left_child = 2 * tree_index + 1;
        const int right_child = 2 * tree_index + 2;

        // Set the parent value to the maximum of the children
        const unsigned char left_value = get_tree(r, left_child);
        const unsigned char right_value = get_tree(r, right_child);

        if (left_value > right_value) {
          set_tree(r, tree_index, left_value);
        } else {
          set_tree(r, tree_index, right_value);
        }

        if (BuddyAllocator<Config>::_sizeMapIsBitmap &&
            BuddyAllocator<Config>::_sizeMapEnabled) {
          BuddyAllocator<Config>::set_split_block(r, tree_index, true);
          BUDDY_DBG("setting split_idx " << tree_index);
        }
        tree_index = (tree_index - 1) / 2;
      }

      BuddyAllocator<Config>::_freeSizes[r] -=
          BuddyHelper::round_up_pow2(totalSize);

      BuddyAllocator<Config>::_regionMutexes[r].unlock();
      // auto end = rdtsc();
      // std::cout << "Time taken: " << (end - start) << std::endl;
      return reinterpret_cast<void *>(block);
    }
  }

  return nullptr;
}

// Deallocates a block of memory of the given size
template <typename Config>
void BTBuddyAllocator<Config>::deallocate_internal(void *ptr, size_t size) {
  auto block = reinterpret_cast<uintptr_t>(ptr);
  const uint8_t region = BuddyAllocator<Config>::get_region(block);
  uint8_t level = BuddyAllocator<Config>::get_level(block, size);
  BUDDY_DBG("deallocating! block "
            << BuddyAllocator<Config>::block_index(block, region, level)
            << " region " << static_cast<int>(region)
            << " level: " << static_cast<int>(level) << " size: " << size);

  const uint8_t block_height = tree_height(size);
  unsigned int block_index =
      BuddyAllocator<Config>::block_index(block, region, level);

  set_tree(region, block_index, block_height);

  // Set the index to the parent
  block_index = (block_index - 1) / 2;

  for (uint8_t l = block_height; l < BuddyAllocator<Config>::_numLevels; l++) {
    const unsigned char left_value = get_tree(region, 2 * block_index + 1);
    const unsigned char right_value = get_tree(region, 2 * block_index + 2);
    BUDDY_DBG("left child value: " << (int)left_value << " right child value: "
                                   << (int)right_value);

    // Set the parent value to the maximum of the children
    if (left_value == right_value && left_value == l) {
      // _btTree[region][block_index] = l + 1;
      set_tree(region, block_index, l + 1);
      BuddyAllocator<Config>::set_split_block(region, block_index, false);
    } else {
      set_tree(region, block_index,
               left_value > right_value ? left_value : right_value);
      // _btTree[region][block_index] =
      // left_value > right_value ? left_value : right_value;
    }
    block_index = (block_index - 1) / 2;
  }

  BuddyAllocator<Config>::_freeSizes[region] += size;
}

template <typename Config>
uint8_t BTBuddyAllocator<Config>::tree_height(size_t size) {
  return BuddyAllocator<Config>::_numLevels -
         BuddyAllocator<Config>::find_smallest_block_level(size);
}

template <typename Config> void BTBuddyAllocator<Config>::print_free_list() {
  for (int r = 0; r < BuddyAllocator<Config>::_numRegions; r++) {
    std::cout << "Region " << r << " tree: " << std::endl;
    unsigned int bits_per_line = 1;
    unsigned int count = 0;
    for (unsigned int i = 0; i < (1U << BuddyAllocator<Config>::_numLevels) - 1;
         i++) {
      std::cout << static_cast<int>(get_tree(r, i)) << " ";
      if (++count % bits_per_line == 0) {
        std::cout << std::endl;
        bits_per_line *= 2;
        count = 0;
      }
    }
    std::cout << std::endl;
  }
}