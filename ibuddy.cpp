#include "ibuddy.hpp"
#include "ibuddy_instantiations.hpp"
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <sys/mman.h>

#ifdef DEBUG
#define BUDDY_DBG(x) std::cout << x << std::endl;
#else
#define BUDDY_DBG(x)                                                           \
  do {                                                                         \
  } while (0)
#endif

namespace {
bool list_empty(double_link *head) { return head->next == head; }

void list_remove(double_link *node) {
  node->prev->next = node->next;
  node->next->prev = node->prev;
  node->prev = node;
  node->next = node;
}

void push_back(double_link *head, double_link *node) {
  BUDDY_DBG("pushing back " << node);
  node->prev = head->prev;
  node->next = head;
  head->prev->next = node;
  head->prev = node;
}

double_link *pop_first(double_link *head) {
  if (list_empty(head)) {
    return nullptr;
  }

  double_link *first = head->next;
  first->next->prev = head;
  head->next = first->next;

  first->prev = first;
  first->next = first;
  return first;
}

bool bit_is_set(const unsigned char *bitmap, int index) {
  return static_cast<bool>(bitmap[index / 8] &
                           (1U << (static_cast<unsigned int>(index) % 8)));
}

void set_bit(unsigned char *bitmap, int index) {
  bitmap[index / 8] |= (1U << (static_cast<unsigned int>(index) % 8));
}

void clear_bit(unsigned char *bitmap, int index) {
  bitmap[index / 8] &= ~(1U << (static_cast<unsigned int>(index) % 8));
}

// void flip_bit(unsigned char *bitmap, int index) {
//   bitmap[index / 8] ^= (1U << (static_cast<unsigned int>(index) % 8));
// }

// int map_index(int index) {
//   // if (index == 0) {
//   //   return 0;
//   // } else {
//   //   return (index - 1) / 2 + 1;
//   // }
//   return (index - 1) / 2;
// }

size_t round_up_pow2(size_t size) {
  if (size == 0) {
    return 1;
  }

  size--;
  size |= size >> 1U;
  size |= size >> 2U;
  size |= size >> 4U;
  size |= size >> 8U;
  size |= size >> 16U;
  size++;

  return size;
}
} // namespace

template <typename Config>
inline uintptr_t IBuddyAllocator<Config>::region_start(uint8_t region) {
  return _start + (region << _maxBlockSizeLog2);
}

// Returns the size of a block at the given level
template <typename Config>
inline unsigned int IBuddyAllocator<Config>::size_of_level(uint8_t level) {
  return _maxSize >> level;
}

template <typename Config>
inline unsigned int IBuddyAllocator<Config>::index_in_level(uintptr_t ptr,
                                                            uint8_t region,
                                                            uint8_t level) {
  // return (ptr - region_start(region)) / size_of_level(level);
  return (ptr - region_start(region)) >>
         (_numLevels - level - 1 + _minBlockSizeLog2);
}

template <typename Config>
inline unsigned int IBuddyAllocator<Config>::index_of_level(uint8_t level) {
  return (1U << level) - 1;
}

template <typename Config>
unsigned int IBuddyAllocator<Config>::block_index(uintptr_t ptr, uint8_t region,
                                                  uint8_t level) {
  return index_of_level(level) + index_in_level(ptr, region, level);
}

template <typename Config>
unsigned int IBuddyAllocator<Config>::buddy_index(uintptr_t ptr, uint8_t region,
                                                  uint8_t level) {
  int block_idx = block_index(ptr, region, level);
  if (block_idx % 2 == 0) {
    return block_idx - 1;
  }
  return block_idx + 1;
  // return (block_idx + 1) ^ 1;
}

template <typename Config>
uint8_t IBuddyAllocator<Config>::get_level(uintptr_t ptr) {
  uint8_t region = get_region(ptr);
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
    if (bit_is_set(_sizeMap[region], block_index(ptr, region, i - 1))) {
      BUDDY_DBG("level is: " << (int)i);
      return i;
    }
  }
  return 0;
}

template <typename Config>
inline uint8_t IBuddyAllocator<Config>::get_region(uintptr_t ptr) {
  return (ptr - _start) >> Config::maxBlockSizeLog2;
  // return (ptr - _start) / Config::maxBlockSize;
}

// Returns the number of blocks needed to fill the given size
template <typename Config>
unsigned int IBuddyAllocator<Config>::num_blocks(size_t size, uint8_t level) {
  return round_up_pow2(size) / size_of_level(level);
  // return size / size_of_level(level) + (size % size_of_level(level) != 0);
}

// Returns the number of blocks needed to fill the given size
// template <typename Config>
// unsigned int IBuddyAllocator<Config>::num_blocks(size_t size, uint8_t level)
// { return size / size_of_level(level) + (size % size_of_level(level) != 0);
// }

// Returns the buddy of the given block
template <typename Config>
uintptr_t IBuddyAllocator<Config>::get_buddy(uintptr_t ptr, uint8_t level) {
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
uintptr_t IBuddyAllocator<Config>::align_left(uintptr_t ptr, uint8_t level) {
  uint8_t region = get_region(ptr);
  return (region_start(region) +
          size_of_level(level) * index_in_level(ptr, region, level));
}

template <typename Config>
void IBuddyAllocator<Config>::init_bitmaps(bool startFull) {
  for (int i = 0; i < Config::numRegions; i++) {
    for (int j = 0; j < Config::allocedBitmapSize; j++) {
      _freeBlocks[i][j] = startFull ? 0x0 : 0x55; // 0x55 = 01010101
    }
  }

  if (!_sizeMapEnabled) {
    return;
  }

  if (Config::sizeBits == 0) { // Bitmap indicates split blocks
    if (startFull) {
      for (int i = 0; i < Config::numRegions; i++) {
        for (int j = 0; j < Config::sizeBitmapSize; j++) {
          _sizeMap[i][j] = 0xFF; // 11111111
        }
      }
    } else {
      for (int i = 0; i < Config::numRegions; i++) {
        for (int j = 0; j < Config::sizeBitmapSize; j++) {
          _sizeMap[i][j] = 0x00; // 00000000
        }
      }
    }
  }
}

template <typename Config> void IBuddyAllocator<Config>::init_free_lists() {
  for (int r = 0; r < Config::numRegions; r++) {
    for (int l = 0; l < Config::numLevels; l++) {
      _freeList[r][l] = {&_freeList[r][l], &_freeList[r][l]};
    }
  }

  _lazyList = {&_lazyList, &_lazyList};
}

template <typename Config>
IBuddyAllocator<Config>::IBuddyAllocator(void *start, int lazyThreshold,
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
      std::terminate();
    }
  }

  _start = reinterpret_cast<uintptr_t>(start);

  _lazyThreshold = lazyThreshold;
  _totalSize = Config::numRegions * Config::maxBlockSize;
  _freeSize = startFull ? 0 : _totalSize;

  // Initialize bitmaps
  init_bitmaps(startFull);

  // Initialize free lists
  init_free_lists();

  // Insert all blocks into the free lists if starting empty
  if (!startFull) {
    for (int r = 0; r < Config::numRegions; r++) {
      uintptr_t curr_start = region_start(r);
      BUDDY_DBG("start: " << reinterpret_cast<void *>(_start));
      BUDDY_DBG("curr_start: " << reinterpret_cast<void *>(curr_start));
      auto *start_link = reinterpret_cast<double_link *>(curr_start);

      // Insert blocks into the free lists
      for (uint8_t lvl = _numLevels - 1; lvl > 0; lvl--) {
        for (uintptr_t i = curr_start + (1U << static_cast<unsigned int>(
                                             _maxBlockSizeLog2 - lvl));
             i < curr_start + _maxSize;
             i += (2U << static_cast<unsigned int>(_maxBlockSizeLog2 - lvl))) {
          push_back(&_freeList[r][lvl], reinterpret_cast<double_link *>(i));
        }
      }
      push_back(&_freeList[r][0], start_link);

      // Initialize
      start_link->prev = &_freeList[r][0];
      start_link->next = &_freeList[r][0];
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

// Marks blocks as split above the given level
template <typename Config>
void IBuddyAllocator<Config>::split_bits(uintptr_t ptr, uint8_t region,
                                         uint8_t level_start,
                                         uint8_t level_end) {
  BUDDY_DBG("splitting bits from " << (int)level_start << " to "
                                   << (int)level_end << " in region "
                                   << region);
  for (uint8_t i = level_start; i < level_end && i < _numLevels - 1; i++) {
    BUDDY_DBG("splitting bit at " << block_index(ptr, region, i));
    set_bit(_sizeMap[region], block_index(ptr, region, i));
  }
}

// Sets the level of the given block in the size map
template <typename Config>
void IBuddyAllocator<Config>::set_level(uintptr_t ptr, uint8_t region,
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

// Returns the size of the allocated block
template <typename Config>
size_t IBuddyAllocator<Config>::get_alloc_size(uintptr_t ptr) {
  return size_of_level(get_level(ptr));
}

// Allocates a block of memory of the given size
template <typename Config>
void *IBuddyAllocator<Config>::allocate(size_t totalSize) {
  // Allocation too large
  if (totalSize > static_cast<size_t>(_maxSize)) {
    BUDDY_DBG("Requested size is too large");
    return nullptr;
  }

  // Allocate from lazy list if possible
  if (_lazyListSize > 0 && totalSize <= static_cast<size_t>(_minSize)) {
    BUDDY_DBG("Allocating from lazy list");
    void *block = pop_first(&_lazyList);
    BUDDY_DBG("allocated block "
              << block_index(reinterpret_cast<uintptr_t>(block),
                             get_region(reinterpret_cast<uintptr_t>(block)),
                             _numLevels - 1)
              << " at " << reinterpret_cast<uintptr_t>(block) - _start
              << " level: " << _numLevels - 1);
    _lazyListSize--;
    _freeSize -= _minSize;

    return block;
  }

  uint8_t region = 0;

  while (region < _numRegions) {
    // Move down if the current level inside the region has been exhausted
    while (list_empty(&_freeList[region][_topLevel[region]]) &&
           _topLevel[region] < _numLevels) {
      _topLevel[region]++;
    }
    if (size_of_level(_topLevel[region]) >= totalSize) {
      break;
    }
    region++;
  }

  uint8_t level = _topLevel[region];
  BUDDY_DBG("at region: " << (int)region << ", level: " << (int)level
                          << " with block size: " << size_of_level(level));

  // No free block large enough available
  if (region == _numRegions || level == _numLevels ||
      list_empty(&_freeList[region][level]) ||
      static_cast<size_t>(size_of_level(level)) < totalSize) {
    BUDDY_DBG("No free blocks available");
    return nullptr;
  }

  // Get the first free block
  auto block =
      reinterpret_cast<uintptr_t>(pop_first(&_freeList[region][level]));

  int block_level = find_smallest_block_level(totalSize);

  // Multiple blocks needed, align the block for its level
  uintptr_t block_left = align_left(block, block_level);
  BUDDY_DBG("aligned block " << block_index(block_left, region, level) << " at "
                             << block_left - _start << " level: " << level);

  // Mark above blocks as split
  if (_sizeMapEnabled) {
    if (_sizeMapIsBitmap) {
      split_bits(block, region, level, block_level);
    } else {
      set_level(block_left, region, block_level);
    }
  }

  // Mark the block as allocated
  clear_bit(_freeBlocks[region], block_index(block, region, level));
  BUDDY_DBG("cleared block " << block_index(block, region, level) << " at "
                             << block - _start << " level: " << (int)level
                             << " region: " << (int)region);

  // Can fit in one block
  if (totalSize <= static_cast<size_t>(_minSize)) {
    _freeSize -= _minSize;
    return reinterpret_cast<void *>(block);
  }

  level = block_level;

  // Clear all smaller levels
  int start_level = find_smallest_block_level(totalSize);
  size_t new_size = round_up_pow2(totalSize);

  BUDDY_DBG("STARTING LEVEL: " << start_level << " SIZE: " << totalSize);

  for (int i = start_level + 1; i < _numLevels; i++) {
    unsigned int start_block_idx = block_index(block_left, region, i);
    BUDDY_DBG("start block index: " << start_block_idx);
    for (unsigned int j = start_block_idx;
         j < start_block_idx + num_blocks(new_size, i); j++) {
      BUDDY_DBG("clearing block " << j << " level: " << i);
      clear_bit(_freeBlocks[region], j);
    }
  }

  // Clear free list
  for (uintptr_t i = block_left; i < block_left + new_size;
       i += size_of_level(_numLevels - 1)) {
    if (i != block) {
      BUDDY_DBG("clearing free list at " << i - region_start(region));
      list_remove(reinterpret_cast<double_link *>(i));
    }
  }

  _freeSize -= new_size;

  return reinterpret_cast<void *>(block_left);
}

// Deallocates a single (smallest) block of memory
template <typename Config>
void IBuddyAllocator<Config>::deallocate_single(uintptr_t ptr) {

  uint8_t region = get_region(ptr);
  uint8_t level = _numLevels - 1;
  int block_idx = block_index(ptr, region, level);
  int buddy_idx = buddy_index(ptr, region, level);

  BUDDY_DBG("block index: " << block_idx << ", buddy index: " << buddy_idx
                            << ", is set: "
                            << bit_is_set(_freeBlocks[region], buddy_idx));

  // While the buddy is free, go up a level
  while (level > 0 && bit_is_set(_freeBlocks[region], buddy_idx)) {
    level--;
    block_idx = block_index(ptr, region, level);
    buddy_idx = buddy_index(ptr, region, level);
    BUDDY_DBG("new block index: " << block_idx << " buddy index: " << buddy_idx
                                  << " level: " << level);

    if (_sizeMapIsBitmap && _sizeMapEnabled) {
      clear_bit(_sizeMap[region], block_idx);
      BUDDY_DBG("clearing split_idx " << block_idx);
    }
  }

  // Mark the block as free and insert it into the free list
  push_back(&_freeList[region][level], reinterpret_cast<double_link *>(ptr));
  if (level > 0) {

    set_bit(_freeBlocks[region], block_index(ptr, region, level));
    BUDDY_DBG("inserting " << ptr - region_start(region)
                           << " at level: " << (int)level << " marking bit: "
                           << block_index(ptr, region, level) << " as free");
  }

  // Set the level of the topmost free block
  if (level < _topLevel[region]) {
    _topLevel[region] = level;
  }

  _freeSize += _minSize;
}

// Deallocates a range of blocks of memory as if they were the smallest block
template <typename Config>
void IBuddyAllocator<Config>::deallocate_range(void *ptr, size_t size) {
  const auto start = reinterpret_cast<uintptr_t>(ptr);
  const uintptr_t aligned_start =
      align_left(start + _minSize - 1, _numLevels - 1);
  const uintptr_t end = align_left(start + size, _numLevels - 1);
  for (uintptr_t i = aligned_start; i < end;
       i += size_of_level(_numLevels - 1)) {

    BUDDY_DBG("deallocating block at " << i - _start);
    deallocate_single(i);
  }
}

// Deallocates a block of memory of the given size
template <typename Config>
void IBuddyAllocator<Config>::deallocate(void *ptr, size_t size) {
  if (size <= static_cast<size_t>(_minSize) && _lazyListSize < _lazyThreshold) {
    BUDDY_DBG("inserting " << reinterpret_cast<uintptr_t>(ptr) - _start
                           << " with index "
                           << block_index(
                                  reinterpret_cast<uintptr_t>(ptr),
                                  get_region(reinterpret_cast<uintptr_t>(ptr)),
                                  _numLevels - 1)
                           << " into lazy list");
    push_back(&_lazyList, static_cast<double_link *>(ptr));
    _lazyListSize++;
    _freeSize += _minSize;
    BUDDY_DBG("lazy list size: " << _lazyListSize);
    return;
  }

  return deallocate_range(ptr, round_up_pow2(size));
}

// Deallocates a block of memory
template <typename Config> void IBuddyAllocator<Config>::deallocate(void *ptr) {
  if (ptr == nullptr || reinterpret_cast<uintptr_t>(ptr) < _start ||
      reinterpret_cast<uintptr_t>(ptr) >= _start + _numRegions * _maxSize) {
    return;
  }

  size_t size = get_alloc_size(reinterpret_cast<uintptr_t>(ptr));
  BUDDY_DBG("deallocate size: " << size);
  return deallocate(ptr, size);
}

// Empties the lazy list, inserting blocks back into the free list
template <typename Config> void IBuddyAllocator<Config>::empty_lazy_list() {
  while (_lazyListSize > 0) {
    auto block = reinterpret_cast<uintptr_t>(pop_first(&_lazyList));
    _lazyListSize--;
    deallocate_single(block);
    // Compensate for the increase size, as the block is just moved, not freed
    _freeSize -= _minSize;
  }
}

// Returns the free size
template <typename Config> size_t IBuddyAllocator<Config>::free_size() {
  return _freeSize;
}

// Fills the memory, marking all blocks as allocated
// This overwrites previous allocations
template <typename Config> void IBuddyAllocator<Config>::fill() {
  init_bitmaps(true);
  init_free_lists();
  _freeSize = 0;
}

// Prints the free list
template <typename Config> void IBuddyAllocator<Config>::print_free_list() {
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
    std::cout << "Lazy list size: " << _lazyListSize << std::endl;
  }
}

// Prints the bitmaps
template <typename Config> void IBuddyAllocator<Config>::print_bitmaps() {
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

// Returns the level of the smallest block that can fit the given size
template <typename Config>
int IBuddyAllocator<Config>::find_smallest_block_level(size_t size) {
  for (size_t i = _minBlockSizeLog2; i <= _maxBlockSizeLog2; i++) {
    if (size <= 1U << i) {
      BUDDY_DBG("level: " << _maxBlockSizeLog2 - i << " size: " << (1 << i));
      return _maxBlockSizeLog2 - i;
    }
  }
  return -1;
}