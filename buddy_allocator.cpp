#include "buddy_allocator.hpp"
#include "buddy_helper.hpp"
#include "buddy_instantiations.hpp"

#include <atomic>
#include <cmath>
#include <cstddef>
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
inline uintptr_t BuddyAllocator<Config>::region_start(uint8_t region) {
  return _start + (region << _maxBlockSizeLog2);
}

// Returns the size of a block at the given level
template <typename Config>
inline unsigned int BuddyAllocator<Config>::size_of_level(uint8_t level) {
  return _maxSize >> level;
}

template <typename Config>
inline unsigned int BuddyAllocator<Config>::index_in_level(uintptr_t ptr,
                                                           uint8_t region,
                                                           uint8_t level) {
  // return (ptr - region_start(region)) / size_of_level(level);
  return (ptr - region_start(region)) >>
         (_numLevels - level - 1 + _minBlockSizeLog2);
}

template <typename Config>
inline unsigned int BuddyAllocator<Config>::index_of_level(uint8_t level) {
  return (1U << level) - 1;
}

template <typename Config>
inline uint8_t BuddyAllocator<Config>::level_of_index(unsigned int index) {
  return 31 - __builtin_clz(index + 1);
}

template <typename Config>
unsigned int BuddyAllocator<Config>::block_index(uintptr_t ptr, uint8_t region,
                                                 uint8_t level) {
  return index_of_level(level) + index_in_level(ptr, region, level);
}

template <typename Config>
unsigned int BuddyAllocator<Config>::buddy_index(uintptr_t ptr, uint8_t region,
                                                 uint8_t level) {
  int block_idx = block_index(ptr, region, level);
  if (block_idx % 2 == 0) {
    return block_idx - 1;
  }
  return block_idx + 1;
  // return (block_idx + 1) ^ 1;
}

template <typename Config>
uint8_t BuddyAllocator<Config>::get_level(uintptr_t ptr, size_t size) {
  uint8_t region = get_region(ptr);
  if (size > 0) {
    return find_smallest_block_level(size);
  }

  if (!_sizeMapIsBitmap) {
    int index = (ptr - region_start(region)) / _minSize;

    if (_sizeBits == 8) {
      return _sizeMap[region][index];
    }

    const int byteIndex = index / 2;
    const int bitOffset = (index % 2) * 4;

    BUDDY_DBG("getting level at " << index << " byte index: " << byteIndex
                                  << " bit offset: " << bitOffset);

    return (_sizeMap[region][byteIndex] >> bitOffset) & 0xF;
  }

  for (uint8_t i = _numLevels - 1; i > 0; i--) {
    // BUDDY_DBG("block_index: " << block_index(ptr, region, i - 1));
    if (BuddyHelper::bit_is_set(_sizeMap[region],
                                block_index(ptr, region, i - 1))) {
      BUDDY_DBG("level is: " << (int)i);
      return i;
    }
  }
  return 0;
}

template <typename Config>
inline uint8_t BuddyAllocator<Config>::get_region(uintptr_t ptr) {
  return (ptr - _start) >> Config::maxBlockSizeLog2;
  // return (ptr - _start) / Config::maxBlockSize;
}

// Returns the number of blocks needed to fill the given size
template <typename Config>
unsigned int BuddyAllocator<Config>::num_blocks(size_t size, uint8_t level) {
  return BuddyHelper::round_up_pow2(size) / size_of_level(level);
  // return size / size_of_level(level) + (size % size_of_level(level) != 0);
}

// Returns the number of blocks needed to fill the given size
// template <typename Config>
// unsigned int BuddyAllocator<Config>::num_blocks(size_t size, uint8_t level)
// { return size / size_of_level(level) + (size % size_of_level(level) != 0);
// }

// Returns the buddy of the given block
template <typename Config>
uintptr_t BuddyAllocator<Config>::get_buddy(uintptr_t ptr, uint8_t level) {
  if (level == 0) {
    return ptr;
  }
  // int block_idx = block_index(p, level);
  uint8_t region = get_region(ptr);
  int buddy_idx = buddy_index(ptr, region, level);
  uintptr_t buddy =
      (region_start(region) +
       size_of_level(level) * (buddy_idx - index_of_level(level)));
  return buddy;
}

// Aligns the given pointer to the left-most pointer of the block
template <typename Config>
uintptr_t BuddyAllocator<Config>::align_left(uintptr_t ptr, uint8_t level) {
  uint8_t region = get_region(ptr);
  return (region_start(region) +
          size_of_level(level) * index_in_level(ptr, region, level));
}

template <typename Config>
uintptr_t BuddyAllocator<Config>::get_address(uint8_t region,
                                              unsigned int blockIndex) {
  unsigned int n = blockIndex + 1;
  uint8_t level = 0;
  while (n >>= 1U) {
    level++;
  }

  return region_start(region) +
         (blockIndex - index_of_level(level)) * size_of_level(level);
}

// Returns the size of the allocated block
template <typename Config>
size_t BuddyAllocator<Config>::get_alloc_size(uintptr_t ptr) {
  return size_of_level(get_level(ptr));
}

// Returns the level of the smallest block that can fit the given size
template <typename Config>
uint8_t BuddyAllocator<Config>::find_smallest_block_level(size_t size) {
  for (size_t i = _minBlockSizeLog2; i <= _maxBlockSizeLog2; i++) {
    if (size <= 1U << i) {
      BUDDY_DBG("level: " << _maxBlockSizeLog2 - i << " size: " << (1 << i));
      return _maxBlockSizeLog2 - i;
    }
  }
  return -1;
}

// Returns the level of the smallest block that can fit the given size
// template <typename Config>
// uint8_t BuddyAllocator<Config>::find_smallest_block_level(size_t size) {
//   // Determine the level directly using logarithmic calculations
//   if (size == 0) {
//     return -1; // Invalid size
//   }

//   // Calculate the logarithm base 2 of size
//   int logSize = std::log2(size);

//   // Ensure we consider the next higher level if size is not a power of 2
//   if (size & (size - 1)) {
//     logSize += 1;
//   }

//   // Ensure logSize is within the valid range of block levels
//   int level = _maxBlockSizeLog2 - logSize;
//   if (level >= 0 && level <= (_maxBlockSizeLog2 - _minBlockSizeLog2)) {
//     BUDDY_DBG("level: " << level
//                         << " size: " << (1 << (_maxBlockSizeLog2 - level)));
//     return level;
//   }

//   return -1; // Invalid size or level
// }

// Sets the level of the given block in the size map
template <typename Config>
void BuddyAllocator<Config>::set_level(uintptr_t ptr, uint8_t region,
                                       uint8_t level) {
  const unsigned int index = (ptr - region_start(region)) / _minSize;
  if (_sizeBits == 8) {
    _sizeMap[region][index] = level;
    return;
  }

  const unsigned int byteIndex = index / 2;
  const unsigned int bitOffset = (index % 2) * 4;
  BUDDY_DBG("setting level at " << index << " byte index: " << byteIndex
                                << " bit offset: " << bitOffset);

  _sizeMap[region][byteIndex] &=
      ~(0xFU << bitOffset); // Clear the bits for the current level
  _sizeMap[region][byteIndex] |=
      (level << bitOffset); // Set the bits for the new level
}

// Marks blocks as split above the given level
template <typename Config>
void BuddyAllocator<Config>::split_bits(uintptr_t ptr, uint8_t region,
                                        uint8_t level_start,
                                        uint8_t level_end) {
  BUDDY_DBG("splitting bits from " << (int)level_start << " to "
                                   << (int)level_end << " in region "
                                   << region);
  for (uint8_t i = level_start; i < level_end && i < _numLevels - 1; i++) {
    BUDDY_DBG("splitting bit at " << block_index(ptr, region, i));
    BuddyHelper::set_bit(_sizeMap[region], block_index(ptr, region, i));
  }
}

template <typename Config> void BuddyAllocator<Config>::init_free_lists() {
  for (int r = 0; r < Config::numRegions; r++) {
    for (int l = 0; l < Config::numLevels; l++) {
      _freeList[r][l] = {&_freeList[r][l], &_freeList[r][l]};
    }
  }
}

// Gets the highest level that the pointer is aligned to
template <typename Config>
uint8_t BuddyAllocator<Config>::level_alignment(uintptr_t ptr, uint8_t region,
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

template <typename Config>
void BuddyAllocator<Config>::set_bitmaps(unsigned char freeBlocksPattern,
                                         unsigned char sizeMapPattern) {
  for (int i = 0; i < Config::numRegions; i++) {
    for (int j = 0; j < Config::allocedBitmapSize; j++) {
      _freeBlocks[i][j] = freeBlocksPattern;
    }
  }

  if (!_sizeMapEnabled) {
    return;
  }

  if (Config::sizeBits == 0) { // Bitmap indicates split blocks
    for (int i = 0; i < Config::numRegions; i++) {
      for (int j = 0; j < Config::sizeBitmapSize; j++) {
        _sizeMap[i][j] = sizeMapPattern;
      }
    }
  }
}

template <typename Config>
void BuddyAllocator<Config>::init_lazy_lists(int lazyThreshold) {
  for (int l = 0; l < Config::numLevels; l++) {
    _lazyList[l] = {&_lazyList[l], &_lazyList[l]};
  }

  uint8_t level = _numLevels - 1;
  while (lazyThreshold > 0 && level > 0) {
    _lazyThresholds[level] = lazyThreshold;
    lazyThreshold /= 2;
    level--;
  }
}

template <typename Config>
BuddyAllocator<Config>::BuddyAllocator(void *start, int lazyThreshold,
                                       bool startFull) {

  static_assert(Config::numLevels > 0,
                "Number of levels must be greater than 0");
  static_assert(Config::minBlockSizeLog2 >= 4,
                "Minimum block size must be greater than or equal to 4");
  static_assert(Config::maxBlockSizeLog2 > Config::minBlockSizeLog2,
                "Maximum block size must be greater than minimum block size");
  static_assert(Config::sizeBits == 0 || Config::sizeBits == 4 ||
                    Config::sizeBits == 8,
                "Size bits must be 0, 4, or 8");
  static_assert(!(Config::sizeBits == 4 &&
                  Config::maxBlockSizeLog2 - Config::minBlockSizeLog2 > 16),
                "Combination of sizeBits = 4 and maxBlockSizeLog2 - "
                "minBlockSizeLog2 > 16 is not allowed");

  if (start == nullptr) {
    start = mmap(nullptr, (Config::numRegions * Config::maxBlockSize),
                 PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (start == MAP_FAILED) {
      BUDDY_DBG("mmap failed");
      exit(1);
    }
  }

  // Initialize free lists
  init_free_lists();

  _start = reinterpret_cast<uintptr_t>(start);
  _totalSize = Config::numRegions * Config::maxBlockSize;
  for (int i = 0; i < Config::numRegions; i++) {
    _freeSizes[i] = startFull ? 0 : Config::maxBlockSize;
  }

  init_lazy_lists(lazyThreshold);
}

// Returns the free size
template <typename Config> size_t BuddyAllocator<Config>::free_size() {
  size_t total = 0;
  for (int i = 0; i <= Config::numRegions; i++) {
    total += _freeSizes[i];
  }
  return total;
}

// Allocates a block of memory of the given size
template <typename Config>
void *BuddyAllocator<Config>::allocate(size_t totalSize) {
  // Allocation too large
  if (totalSize > _maxSize) {
    BUDDY_DBG("Requested size is too large");
    return nullptr;
  }

  uint8_t level = find_smallest_block_level(totalSize);
  if (_lazyListSize[level] > 0) {
    bool locked = _lazyMutexes[level].try_lock();
    if (locked && _lazyListSize[level] > 0) {
      void *block = BuddyHelper::pop_first(&_lazyList[level]);
      _lazyListSize[level]--;
      _freeSizes[_numRegions] -= BuddyHelper::round_up_pow2(totalSize);
      _lazyMutexes[level].unlock();
      // BUDDY_DBG("Allocated block "
      //           << block_index(reinterpret_cast<uintptr_t>(block),
      //                          get_region(reinterpret_cast<uintptr_t>(block)),
      //                          _numLevels - 1)
      //           << " at " << reinterpret_cast<uintptr_t>(block) - _start
      //           << " level: " << _numLevels - 1);
      return block;
    }
    _lazyMutexes[level].unlock();
  }

  void *p = allocate_internal(totalSize);
  // if (p == nullptr) {
    // empty_lazy_list();
    // p = allocate_internal(totalSize);
  // }
  return p;
}

// Deallocates a block of memory of the given size
template <typename Config>
void BuddyAllocator<Config>::deallocate(void *ptr, size_t size) {
  // Extra checks needed for some programs
  if (ptr == nullptr || reinterpret_cast<uintptr_t>(ptr) < _start ||
      reinterpret_cast<uintptr_t>(ptr) >= _start + _numRegions * _maxSize) {
    return;
  }

  if (size < _minSize) {
    size = _minSize;
  }

  uint8_t level = find_smallest_block_level(size);

  if (_lazyListSize[level] < _lazyThresholds[level]) {
    _lazyMutexes[level].lock();
    BUDDY_DBG("inserting " << reinterpret_cast<uintptr_t>(ptr) - _start
                           << " with index "
                           << block_index(
                                  reinterpret_cast<uintptr_t>(ptr),
                                  get_region(reinterpret_cast<uintptr_t>(ptr)),
                                  _numLevels - 1)
                           << " into lazy list");
    BuddyHelper::push_back(&_lazyList[level], static_cast<double_link *>(ptr));
    _lazyListSize[level]++;
    _freeSizes[_numRegions] += BuddyHelper::round_up_pow2(size);
    _lazyMutexes[level].unlock();
    BUDDY_DBG("lazy list size: " << _lazyListSize[level]);
    return;
  }

  deallocate_internal(ptr, BuddyHelper::round_up_pow2(size));
}

// Deallocates a block of memory
template <typename Config> void BuddyAllocator<Config>::deallocate(void *ptr) {
  if (ptr == nullptr || reinterpret_cast<uintptr_t>(ptr) < _start ||
      reinterpret_cast<uintptr_t>(ptr) >= _start + _numRegions * _maxSize) {
    return;
  }

  size_t size = get_alloc_size(reinterpret_cast<uintptr_t>(ptr));
  BUDDY_DBG("deallocate size: " << size);
  return deallocate(ptr, size);
}

template <typename Config>
void BuddyAllocator<Config>::deallocate_range(void *ptr, size_t size) {
  const auto start = reinterpret_cast<uintptr_t>(ptr);
  const uintptr_t aligned_start = BuddyAllocator<Config>::align_left(
      start + BuddyAllocator<Config>::_minSize - 1,
      BuddyAllocator<Config>::_numLevels - 1);
  const uintptr_t end = BuddyAllocator<Config>::align_left(
      start + size, BuddyAllocator<Config>::_numLevels - 1);

  if (aligned_start >= end) {
    return;
  }

  const uint8_t start_region =
      BuddyAllocator<Config>::get_region(aligned_start);
  const uint8_t end_region = BuddyAllocator<Config>::get_region(end - 1);
  for (uint8_t r = start_region; r <= end_region; r++) {
    uintptr_t region_start = BuddyAllocator<Config>::region_start(r);
    uintptr_t region_end = BuddyAllocator<Config>::region_start(r + 1);
    region_start = region_start < aligned_start ? aligned_start : region_start;
    region_end = region_end < end ? region_end : end;

    size_t region_size = region_end - region_start;
    while (region_start < region_end) {
      BUDDY_DBG("size: " << region_size);
      BUDDY_DBG("aligned_start: " << reinterpret_cast<void *>(region_start)
                                  << " end: "
                                  << reinterpret_cast<void *>(region_end));

      const uint8_t max_level =
          BuddyAllocator<Config>::find_smallest_block_level(region_size);

      uint8_t level;
      size_t block_size;
      if (BuddyAllocator<Config>::size_of_level(max_level) == region_size &&
          max_level == level_alignment(region_start, r, max_level)) {
        level = max_level;
        block_size = region_size;
      } else {
        level = level_alignment(region_start, r, max_level + 1);
        block_size = BuddyAllocator<Config>::size_of_level(level);
      }

      BUDDY_DBG("deallocating block "
                << BuddyAllocator<Config>::block_index(region_start, r, level)
                << " region " << static_cast<int>(r) << " level: "
                << static_cast<int>(level) << " size: " << block_size);

      // Clear all smaller levels
      for (int i = level + 1; i < BuddyAllocator<Config>::_numLevels; i++) {
        BUDDY_DBG("deallocating level " << static_cast<int>(i));
        const unsigned int start_block_idx =
            BuddyAllocator<Config>::block_index(region_start, r, i);
        for (unsigned int j = start_block_idx;
             j < start_block_idx +
                     BuddyAllocator<Config>::num_blocks(block_size, i);
             j++) {
          if (BuddyAllocator<Config>::_sizeMapIsBitmap &&
              BuddyAllocator<Config>::_sizeMapEnabled &&
              i < BuddyAllocator<Config>::_numLevels - 1) {
            BuddyAllocator<Config>::set_split_block(r, j, false);
          }
        }
      }

      if (BuddyAllocator<Config>::_sizeMapEnabled) {
        // Clear final split bit
        if (BuddyAllocator<Config>::_sizeMapIsBitmap &&
            level < BuddyAllocator<Config>::_numLevels - 1) {
          BUDDY_DBG(
              "clearing split_idx "
              << BuddyAllocator<Config>::block_index(region_start, r, level));
          BuddyAllocator<Config>::set_split_block(
              r, BuddyAllocator<Config>::block_index(region_start, r, level),
              false);
        } else if (!BuddyAllocator<Config>::_sizeMapIsBitmap) {
          BuddyAllocator<Config>::set_level(region_start, r, level);
        }
      }

      deallocate_internal(reinterpret_cast<void *>(region_start), block_size);
      region_start += block_size;
      region_size -= block_size;
    }
  }
}

// Empties the lazy list, inserting blocks back into the free list
template <typename Config> void BuddyAllocator<Config>::empty_lazy_list() {
  for (uint8_t l = 0; l < _numLevels; l++) {
    while (_lazyListSize[l] > 0) {
      void *block = BuddyHelper::pop_first(&_lazyList[l]);
      // std::cout << "emptying lazy list: " << block << std::endl;
      _lazyListSize[l]--;

      unsigned int level_size = size_of_level(l);
      deallocate_internal(block, level_size);

      _freeSizes[_numRegions] -= level_size;
    }
  }
}

template <typename Config>
void BuddyAllocator<Config>::push_free_list(uintptr_t ptr, uint8_t region,
                                            uint8_t level) {
  auto *block = reinterpret_cast<double_link *>(ptr);
  double_link *head = &_freeList[region][level];
  BuddyHelper::push_back(head, block);
}

template <typename Config>
bool BuddyAllocator<Config>::free_list_empty(uint8_t region, uint8_t level) {
  return BuddyHelper::list_empty(&_freeList[region][level]);
}

template <typename Config>
uintptr_t BuddyAllocator<Config>::pop_free_list(uint8_t region, uint8_t level) {
  double_link *block = BuddyHelper::pop_first(&_freeList[region][level]);
  return reinterpret_cast<uintptr_t>(block);
}

template <typename Config>
void BuddyAllocator<Config>::set_split_block(uint8_t region,
                                             unsigned int blockIndex,
                                             bool split) {
  if (split) {
    BuddyHelper::set_bit(_sizeMap[region], blockIndex);
  } else {
    BuddyHelper::clear_bit(_sizeMap[region], blockIndex);
  }
}

template <typename Config>
void BuddyAllocator<Config>::set_allocated_block(uint8_t region,
                                                 unsigned int blockIndex,
                                                 bool allocated) {
  if (allocated) {
    BuddyHelper::set_bit(_freeBlocks[region], blockIndex);
  } else {
    BuddyHelper::clear_bit(_freeBlocks[region], blockIndex);
  }
}

template <typename Config>
void BuddyAllocator<Config>::flip_allocated_block(uint8_t region,
                                                  unsigned int blockIndex) {
  BuddyHelper::flip_bit(_freeBlocks[region], blockIndex);
}

template <typename Config>
bool BuddyAllocator<Config>::block_is_split(uint8_t region,
                                            unsigned int blockIndex) {
  return BuddyHelper::bit_is_set(_sizeMap[region], blockIndex);
}

template <typename Config>
bool BuddyAllocator<Config>::block_is_allocated(uint8_t region,
                                                unsigned int blockIndex) {
  return BuddyHelper::bit_is_set(_freeBlocks[region], blockIndex);
}

// Fills the memory, marking all blocks as allocated
// This overwrites previous allocations
template <typename Config> void BuddyAllocator<Config>::fill() {
  init_bitmaps(true);
  BuddyAllocator<Config>::init_free_lists();
  for (auto &size : BuddyAllocator<Config>::_freeSizes) {
    size = 0;
  }
}

// Prints the free list
template <typename Config> void BuddyAllocator<Config>::print_free_list() {
  for (int r = 0; r < _numRegions; r++) {
    for (size_t i = 0; i < static_cast<size_t>(_numLevels); i++) {
      std::cout << "Free list " << i << "(" << (1U << (_maxBlockSizeLog2 - i))
                << "): ";
      for (double_link *link = _freeList[r][i].next; link != &_freeList[r][i];
           link = link->next) {
        std::cout << reinterpret_cast<uintptr_t>(
                         reinterpret_cast<uintptr_t>(link) - _start)
                  << " ";
      }
      std::cout << std::endl;
    }
    std::cout << "Lazy list sizes: ";
    for (size_t i = 0; i < static_cast<size_t>(_numLevels); i++) {
      std::cout << _lazyListSize[i] << " ";
    }
    std::cout << std::endl;
  }
}

// Prints the bitmaps
template <typename Config> void BuddyAllocator<Config>::print_bitmaps() {
  for (int r = 0; r < _numRegions; r++) {
    std::cout << "Allocated blocks: " << std::endl;
    int bitsPerLine = 1;
    int bitsPrinted = 0;
    int spaces = 32;
    for (int i = 0; i < Config::allocedBitmapSize; i++) {
      unsigned int block = _freeBlocks[r][i];
      for (unsigned int j = 0; j < 8; j++) {
        std::cout << ((block >> j) & 1U);
        for (int k = 0; k < spaces - 1; k++) {
          std::cout << " ";
        }
        bitsPrinted++;
        if (bitsPrinted == bitsPerLine) {
          std::cout << std::endl;
          bitsPerLine *= 2;
          bitsPrinted = 0;
          spaces /= 2;
        }
      }
    }
    std::cout << std::endl;

    if (!_sizeMapEnabled) {
      continue;
    }

    std::cout << "Split blocks: " << std::endl;
    if (_sizeMapIsBitmap) {
      for (int i = 0; i < Config::sizeBitmapSize; i++) {
        unsigned int block = _sizeMap[r][i];
        for (unsigned int j = 0; j < 8; j++) {
          std::cout << ((block >> j) & 1U) << " ";
        }
      }
      std::cout << std::endl;
    } else {
      for (int i = 0; i < Config::sizeBitmapSize; i++) {
        unsigned int block = _sizeMap[r][i];
        if (Config::sizeBits == 8) {
          std::cout << size_of_level(block) << " ";
        } else {
          for (unsigned int j = 0; j < 2; j++) {
            std::cout << size_of_level((block >> (j * 4)) & 0xFU) << " ";
          }
        }
      }
      std::cout << std::endl;
    }
  }
}