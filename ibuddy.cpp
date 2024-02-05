#include "ibuddy.hpp"
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

template class IBuddyAllocator<IBuddyConfig<4, 25>>;
template class IBuddyAllocator<IBuddyConfig<4, 23>>;
template class IBuddyAllocator<IBuddyConfig<4, 21>>;
template class IBuddyAllocator<IBuddyConfig<4, 10>>;
template class IBuddyAllocator<IBuddyConfig<4, 8>>;

static inline bool list_empty(double_link *head) { return head->next == head; }

static inline void list_remove(double_link *node) {
  node->prev->next = node->next;
  node->next->prev = node->prev;
  node->prev = node;
  node->next = node;
}

static inline void push_back(double_link *head, double_link *node) {
  BUDDY_DBG("pushing back " << node);
  node->prev = head->prev;
  node->next = head;
  head->prev->next = node;
  head->prev = node;
}

static inline double_link *pop_first(double_link *head) {
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

static inline bool bit_is_set(unsigned char *bitmap, int index) {
  return bitmap[index / 8] & (1 << (index % 8));
}

static inline void set_bit(unsigned char *bitmap, int index) {
  bitmap[index / 8] |= (1 << (index % 8));
}

static inline void clear_bit(unsigned char *bitmap, int index) {
  bitmap[index / 8] &= ~(1 << (index % 8));
}

static inline void flip_bit(unsigned char *bitmap, int index) {
  bitmap[index / 8] ^= (1 << (index % 8));
}

static inline int mapIndex(int index) {
  // if (index == 0) {
  //   return 0;
  // } else {
  //   return (index - 1) / 2 + 1;
  // }
  return (index - 1) / 2;
}

static inline size_t round_up_pow2(size_t size) {
  if (size == 0)
    return 1;

  size--;
  size |= size >> 1;
  size |= size >> 2;
  size |= size >> 4;
  size |= size >> 8;
  size |= size >> 16;
  size++;

  return size;
}

// Returns the size of a block at the given level
template <typename Config>
int IBuddyAllocator<Config>::size_of_level(int level) {
  return _totalSize >> level;
}
template <typename Config>
int IBuddyAllocator<Config>::index_in_level(void *ptr, int level) {
  return (reinterpret_cast<uintptr_t>(ptr) - _start) / size_of_level(level);
}

template <typename Config>
int IBuddyAllocator<Config>::index_of_level(int level) {
  return (1 << level) - 1;
}

template <typename Config>
int IBuddyAllocator<Config>::block_index(void *ptr, int level) {
  return index_of_level(level) + index_in_level(ptr, level);
}

template <typename Config>
int IBuddyAllocator<Config>::buddy_index(void *ptr, int level) {
  int block_idx = block_index(ptr, level);
  if (block_idx % 2 == 0) {
    return block_idx - 1;
  } else {
    return block_idx + 1;
  }
  // return (block_idx + 1) ^ 1;
}

template <typename Config> int IBuddyAllocator<Config>::get_level(void *ptr) {
  for (int i = _numLevels - 1; i > 0; i--) {
    BUDDY_DBG("block_index: " << block_index(ptr, i - 1));
    if (bit_is_set(_splitBlocks, block_index(ptr, i - 1))) {
      BUDDY_DBG("level is: " << i);
      return i;
    }
  }
  return 0;
}

// Returns the number of blocks needed to fill the given size
template <typename Config>
int IBuddyAllocator<Config>::num_blocks(size_t size, int level) {
  return size / size_of_level(level) + (size % size_of_level(level) != 0);
}

// Returns the buddy of the given block
template <typename Config>
void *IBuddyAllocator<Config>::get_buddy(void *ptr, int level) {
  if (level == 0) {
    return ptr;
  }
  // int block_idx = block_index(p, level);
  int buddy_idx = buddy_index(ptr, level);
  void *buddy = reinterpret_cast<void *>(
      _start + size_of_level(level) * (buddy_idx - index_of_level(level)));
  return buddy;
}

// Aligns the given pointer to the left-most pointer of the block
template <typename Config>
void *IBuddyAllocator<Config>::align_left(void *ptr, int level) {
  return reinterpret_cast<void *>(
      _start +
      size_of_level(level) * (block_index(ptr, level) - index_of_level(level)));
}

template <typename Config>
IBuddyAllocator<Config>::IBuddyAllocator(void *start, size_t totalSize,
                                         int lazyThreshold) {
  if (start == nullptr) {
    start = mmap(nullptr, totalSize, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (start == MAP_FAILED) {
      BUDDY_DBG("mmap failed");
      exit(1);
    }
  }

  _start = reinterpret_cast<uintptr_t>(start);

  _lazyThreshold = lazyThreshold;
  _totalSize = totalSize;

  if (totalSize < static_cast<size_t>(1 << _maxBlockSizeLog2)) {
    BUDDY_DBG("Total size must be at least 2^" << _maxBlockSizeLog2);
    exit(1);
  }

  // Initialize bitmaps
  for (int i = 0; i < Config::bitmapSize; i++) {
    // _freeBlocks[i] = 0xAA; // 10101010
    _freeBlocks[i] = 0x55; // 01010101
  }

  double_link *start_link = reinterpret_cast<double_link *>(_start);

  // Initialize free lists
  for (int i = 0; i < _numLevels; i++) {
    _freeList[i] = {&_freeList[i], &_freeList[i]};
  }

  // Insert blocks into the free lists
  for (int lvl = _numLevels - 1; lvl > 0; lvl--) {
    for (uintptr_t i = _start + (1 << (_maxBlockSizeLog2 - lvl));
         i < _start + totalSize; i += (2 << (_maxBlockSizeLog2 - lvl))) {
      push_back(&_freeList[lvl], reinterpret_cast<double_link *>(i));
    }
  }
  push_back(&_freeList[0], start_link);

  // Initialize
  start_link->prev = &_freeList[0];
  start_link->next = &_freeList[0];

  _lazyList = {&_lazyList, &_lazyList};
}

// Creates a buddy allocator at the given address
template <typename Config>
IBuddyAllocator<Config> *
IBuddyAllocator<Config>::create(void *addr, void *start, size_t totalSize,
                                int lazyThreshold) {

  IBuddyAllocator *ba = nullptr;

  if (addr == nullptr) {
    addr = mmap(nullptr, sizeof(IBuddyAllocator), PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ba == MAP_FAILED) {
      BUDDY_DBG("mmap failed");
      return nullptr;
    }
  }
  ba = static_cast<IBuddyAllocator *>(addr);

  return new (ba)
      IBuddyAllocator(start, totalSize, lazyThreshold);
}

template <typename Config> IBuddyAllocator<Config>::~IBuddyAllocator() {}

// Marks blocks as split above the given level
template <typename Config>
void IBuddyAllocator<Config>::split_bits(void *ptr, int level_start,
                                         int level_end) {
  BUDDY_DBG("splitting bits from " << level_start << " to " << level_end);
  for (int i = level_start; i < level_end && i < _numLevels - 1; i++) {
    BUDDY_DBG("splitting bit at " << block_index(ptr, i));
    set_bit(_splitBlocks, block_index(ptr, i));
  }
}

// Returns the size of the allocated block
template <typename Config>
size_t IBuddyAllocator<Config>::get_alloc_size(void *ptr) {
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
    BUDDY_DBG("allocated block " << block_index(block, _numLevels - 1) << " at "
                                 << block - _start
                                 << " level: " << _numLevels - 1);
    _lazyListSize--;

    return block;
  }

  // Move down if the current level has been exhausted
  while (list_empty(&_freeList[_topLevel]) && _topLevel < _numLevels - 1) {
    _topLevel++;
  }

  int level = _topLevel;
  BUDDY_DBG("at level: " << level
                         << " with block size: " << size_of_level(level));

  // No free block large enough available
  if (level == _numLevels || list_empty(&_freeList[level]) ||
      static_cast<size_t>(size_of_level(level)) < totalSize) {
    BUDDY_DBG("No free blocks available");
    return nullptr;
  }

  // Get the first free block
  void *block = pop_first(&_freeList[level]);

  int block_level = find_smallest_block_level(totalSize);

  // Mark above blocks as split
  split_bits(block, level, block_level);

  level = block_level;

  // Mark the block as allocated
  clear_bit(_freeBlocks, block_index(block, level));
  BUDDY_DBG("allocated block " << block_index(block, level) << " at "
                               << block - _start << " level: " << level);

  // Can fit in one block
  if (totalSize <= static_cast<size_t>(_minSize)) {
    return block;
  }

  // Multiple blocks needed, align the block for its level
  void *block_left = align_left(block, level);
  BUDDY_DBG("aligned block " << block_index(block_left, level) << " at "
                             << block_left - _start << " level: " << level);

  // Clear all smaller levels
  int start_level = find_smallest_block_level(totalSize);
  int new_size = round_up_pow2(totalSize);

  BUDDY_DBG("STARTING LEVEL: " << start_level << " SIZE: " << totalSize);

  for (int i = start_level; i < _numLevels; i++) {
    int start_block_idx = block_index(block_left, i);
    for (int j = start_block_idx; j < start_block_idx + num_blocks(new_size, i);
         j++) {
      BUDDY_DBG("clearing block " << j << " level: " << i);
      clear_bit(_freeBlocks, j);
    }
  }

  // Clear free list
  uintptr_t start = reinterpret_cast<uintptr_t>(block_left);
  for (uintptr_t i = start; i < start + new_size;
       i += size_of_level(_numLevels - 1)) {
    if (i != reinterpret_cast<uintptr_t>(block)) {
      BUDDY_DBG("clearing free list at " << i - _start);
      list_remove(reinterpret_cast<double_link *>(i));
    }
  }

  return block_left;
}

// Deallocates a single (smallest) block of memory
template <typename Config>
void IBuddyAllocator<Config>::deallocate_single(void *ptr) {

  int level = _numLevels - 1;
  int block_idx = block_index(ptr, level);
  int buddy_idx = buddy_index(ptr, level);

  BUDDY_DBG("block index: " << block_idx << ", buddy index: " << buddy_idx
                            << ", is set: "
                            << bit_is_set(_freeBlocks, buddy_idx));

  // While the buddy is free, go up a level
  while (bit_is_set(_freeBlocks, buddy_idx) && level > 0) {
    level--;
    block_idx = block_index(ptr, level);
    buddy_idx = buddy_index(ptr, level);
    BUDDY_DBG("new block index: " << block_idx << " buddy index: " << buddy_idx
                                  << " level: " << level);

    clear_bit(_splitBlocks, block_idx);
    BUDDY_DBG("clearing split_idx " << block_idx);
  }

  // Mark the block as free and insert it into the free list
  push_back(&_freeList[level], reinterpret_cast<double_link *>(ptr));
  set_bit(_freeBlocks, block_index(ptr, level));
  BUDDY_DBG("inserting " << ptr - _start << " at level: " << level
                         << " marking bit: " << block_index(ptr, level)
                         << " as free");

  // Set the level of the topmost free block
  if (level < _topLevel) {
    _topLevel = level;
  }
}

// Deallocates a range of blocks of memory as if they were the smallest block
template <typename Config>
void IBuddyAllocator<Config>::deallocate_range(void *ptr, size_t size) {
  uintptr_t start = reinterpret_cast<uintptr_t>(ptr);
  for (uintptr_t i = start; i < start + size;
       i += size_of_level(_numLevels - 1)) {

    BUDDY_DBG("deallocating block at " << i - _start);
    deallocate_single(reinterpret_cast<void *>(i));
  }
  return;
}

// Deallocates a block of memory of the given size
template <typename Config>
void IBuddyAllocator<Config>::deallocate(void *ptr, size_t size) {
  if (size <= static_cast<size_t>(_minSize) && _lazyListSize < _lazyThreshold) {
    BUDDY_DBG("inserting " << ptr - _start << " with index "
                           << block_index(ptr, _numLevels - 1)
                           << " into lazy list");
    push_back(&_lazyList, reinterpret_cast<double_link *>(ptr));
    _lazyListSize++;
    BUDDY_DBG("lazy list size: " << _lazyListSize);
    return;
  }

  return deallocate_range(ptr, round_up_pow2(size));
}

// Deallocates a block of memory
template <typename Config> void IBuddyAllocator<Config>::deallocate(void *ptr) {
  if (ptr == nullptr || reinterpret_cast<uintptr_t>(ptr) < _start ||
      reinterpret_cast<uintptr_t>(ptr) >= _start + _totalSize) {
    return;
  }

  size_t size = get_alloc_size(ptr);
  BUDDY_DBG("deallocate size: " << size);
  return deallocate(ptr, size);
}

template <typename Config> void IBuddyAllocator<Config>::empty_lazy_list() {
  while (_lazyListSize > 0) {
    void *block = pop_first(&_lazyList);
    _lazyListSize--;
    deallocate_single(block);
  }
}

// Prints the free list
template <typename Config> void IBuddyAllocator<Config>::print_free_list() {
  for (size_t i = 0; i < static_cast<size_t>(_numLevels); i++) {
    std::cout << "Free list " << i << "(" << (1 << (_maxBlockSizeLog2 - i))
              << "): ";
    for (double_link *link = _freeList[i].next; link != &_freeList[i];
         link = link->next) {
      std::cout << reinterpret_cast<uintptr_t>(
                       reinterpret_cast<uintptr_t>(link) - _start)
                << " ";
    }
    std::cout << std::endl;
  }
  std::cout << "Lazy list size: " << _lazyListSize << std::endl;
}

// Prints the bitmaps
template <typename Config> void IBuddyAllocator<Config>::print_bitmaps() {
  std::cout << "Allocated blocks: ";
  for (int i = 0; i < ((1 << (_numLevels - 1)) / 16) + 1; i++) {
    std::cout << static_cast<int>(_freeBlocks[i]) << " ";
  }
  std::cout << std::endl;

  std::cout << "Split blocks: ";
  for (int i = 0; i < ((1 << (_numLevels - 1)) / 8) + 1; i++) {
    std::cout << static_cast<int>(_splitBlocks[i]) << " ";
  }
  std::cout << std::endl;
}

// Returns the level of the smallest block that can fit the given size
template <typename Config>
int IBuddyAllocator<Config>::find_smallest_block_level(size_t size) {
  for (int i = _minBlockSizeLog2; i <= _maxBlockSizeLog2; i++) {
    if (size <= static_cast<size_t>(1 << i)) {
      BUDDY_DBG("level: " << _maxBlockSizeLog2 - i << " size: " << (1 << i));
      return _maxBlockSizeLog2 - i;
    }
  }
  return -1;
}