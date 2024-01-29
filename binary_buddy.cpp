#include "binary_buddy.hpp"
#include <cstdlib>
#include <iostream>

#ifdef DEBUG
#define BUDDY_DBG(x) std::cout << x << std::endl;
#else
#define BUDDY_DBG(x)                                                           \
  do {                                                                         \
  } while (0)
#endif

static inline bool list_empty(double_link *head) { return head->next == head; }

static inline void list_remove(double_link *node) {
  node->prev->next = node->next;
  node->next->prev = node->prev;
}

static inline void push_back(double_link *head, double_link *node) {
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

int BinaryBuddyAllocator::size_of_level(int level) {
  return _totalSize >> level;
}

int BinaryBuddyAllocator::index_in_level(void *p, int level) {
  return (reinterpret_cast<uintptr_t>(p) - _start) / size_of_level(level);
}

int BinaryBuddyAllocator::index_of_level(int level) { return (1 << level) - 1; }

int BinaryBuddyAllocator::block_index(void *p, int level) {
  return index_of_level(level) + index_in_level(p, level);
}

int BinaryBuddyAllocator::buddy_index(void *p, int level) {
  int block_idx = block_index(p, level);
  if (block_idx % 2 == 0) {
    return block_idx - 1;
  } else {
    return block_idx + 1;
  }
  // return (block_idx + 1) ^ 1;
}

int BinaryBuddyAllocator::get_level(void *p) {
  for (int i = NUM_LISTS - 1; i > 0; i--) {
    BUDDY_DBG("block_index: " << block_index(p, i - 1));
    if (bit_is_set(_splitBlocks, block_index(p, i - 1))) {
      BUDDY_DBG("level is: " << i);
      return i;
    }
  }
  return 0;
}

void *BinaryBuddyAllocator::get_buddy(void *p, int level) {
  if (level == 0) {
    return p;
  }
  // int block_idx = block_index(p, level);
  int buddy_idx = buddy_index(p, level);
  void *buddy = reinterpret_cast<void *>(
      _start + size_of_level(level) * (buddy_idx - index_of_level(level)));
  return buddy;
}

BinaryBuddyAllocator::BinaryBuddyAllocator(void *start, std::size_t totalSize,
                                           int minBlockSizeLog2,
                                           int maxBlockSizeLog2) {
  _start = reinterpret_cast<uintptr_t>(start);
  _totalSize = totalSize;

  _minBlockSizeLog2 = minBlockSizeLog2;
  _maxBlockSizeLog2 = maxBlockSizeLog2;
  _numLevels = maxBlockSizeLog2 - minBlockSizeLog2 + 1;

  if (totalSize < (1 << MAX_SIZE_LOG2)) {
    BUDDY_DBG("Total size must be at least 2^" << MAX_SIZE_LOG2);
    exit(1);
  }

  for (int i = 1; i < _numLevels; i++) {
    _freeList[i] = {&_freeList[i], &_freeList[i]};
  }

  double_link *start_link = reinterpret_cast<double_link *>(start);

  std::cout << "List size: " << sizeof(_freeList) << std::endl;
  std::cout << "Bitmap size: " << sizeof(BITMAP_SIZE) << std::endl;
  std::cout << "Split size: " << sizeof(_splitBlocks) << std::endl;

  _freeList[0] = {start_link, start_link};
  start_link->prev = &_freeList[0];
  start_link->next = &_freeList[0];
}

BinaryBuddyAllocator *BinaryBuddyAllocator::create(void *addr, void *start,
                                                   std::size_t totalSize,
                                                   int minBlockSizeLog2,
                                                   int maxBlockSizeLog2) {
  BinaryBuddyAllocator *ba = reinterpret_cast<BinaryBuddyAllocator *>(addr);
  ba->_start = reinterpret_cast<uintptr_t>(start);
  ba->_totalSize = totalSize;

  ba->_minBlockSizeLog2 = minBlockSizeLog2;
  ba->_maxBlockSizeLog2 = maxBlockSizeLog2;
  ba->_numLevels = maxBlockSizeLog2 - minBlockSizeLog2 + 1;

  if (totalSize < (1 << MAX_SIZE_LOG2)) {
    BUDDY_DBG("Total size must be at least 2^" << MAX_SIZE_LOG2);
    exit(1);
  }

  for (int i = 1; i < ba->_numLevels; i++) {
    ba->_freeList[i] = {&ba->_freeList[i], &ba->_freeList[i]};
  }

  double_link *start_link = reinterpret_cast<double_link *>(start);

  ba->_freeList[0] = {start_link, start_link};
  start_link->prev = &ba->_freeList[0];
  start_link->next = &ba->_freeList[0];

  return ba;
}

BinaryBuddyAllocator::~BinaryBuddyAllocator() {}

void *BinaryBuddyAllocator::allocate(std::size_t totalSize) {
  int start_block_list_idx = find_smallest_block_index(totalSize);
  int block_list_idx = start_block_list_idx;
  bool found = false;
  while (!found) {
    if (!list_empty(&_freeList[block_list_idx])) {
      found = true;
    } else if (block_list_idx > 0) {
      block_list_idx--;
      if (!list_empty(&_freeList[block_list_idx])) {
        // Larger block found, split it
        void *block =
            static_cast<void *>(pop_first(&_freeList[block_list_idx]));

        // if (block_list_idx + 1 < NUM_LISTS - 1) {
        BUDDY_DBG("setting split_idx " << block_index(block, block_list_idx));
        set_bit(_splitBlocks, block_index(block, block_list_idx));
        // }
        // BUDDY_DBG("setting alloc_idx " << block_index(block,
        // block_list_idx)); set_bit(_splitBlocks, block_index(block,
        // block_list_idx));

        if (block_list_idx > 0) {
          BUDDY_DBG("flipping map_idx "
                    << mapIndex(block_index(block, block_list_idx)));
          flip_bit(_allocatedBlocks,
                   mapIndex(block_index(block, block_list_idx)));
        }

        // Split the block into two halves
        void *block1 = block;
        void *block2 =
            reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(block) +
                                     size_of_level(block_list_idx + 1));
        // Add the two buddy blocks back to the free list
        push_back(&_freeList[block_list_idx + 1],
                  reinterpret_cast<double_link *>(block1));
        push_back(&_freeList[block_list_idx + 1],
                  reinterpret_cast<double_link *>(block2));

        block_list_idx = start_block_list_idx;
        // print_free_list();
      }
    } else {
      // std::cout << "Not enough memory" << std::endl;
      return nullptr;
    }
  }

  if (found) {
    void *block = static_cast<void *>(pop_first(&_freeList[block_list_idx]));
    // std::cout << "given block: " << block << std::endl;
    // std::cout << "level: " << block_list_idx << std::endl;
    // if (block_list_idx < NUM_LISTS - 1) {
    BUDDY_DBG("Flipping final map_idx "
              << mapIndex(block_index(block, block_list_idx)));
    // set_bit(_allocatedBlocks, mapIndex(block_index(block, block_list_idx)));
    flip_bit(_allocatedBlocks, mapIndex(block_index(block, block_list_idx)));
    // }
    BUDDY_DBG(
        "bit is: " << bit_is_set(_allocatedBlocks,
                                 mapIndex(block_index(block, block_list_idx))));
    // print_free_list();
    return block;
    // return reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(block)
    // + reinterpret_cast<std::uintptr_t>(_start));
  }

  return nullptr;

  // return malloc(totalSize);
}

void BinaryBuddyAllocator::deallocate(void *ptr) {
  if (ptr == nullptr) {
    return;
  }
  // std::cout << "deallocate: " << ptr - _start << std::endl;
  int level = get_level(ptr);
  // std::cout << "level: " << level << std::endl;
  BUDDY_DBG("deallocating block " << block_index(ptr, level) << " at "
                                  << ptr - _start << " level: " << level);
  BUDDY_DBG("bit is set? " << bit_is_set(_allocatedBlocks,
                                         mapIndex(block_index(ptr, level))));

  BUDDY_DBG("flipping map index: " << mapIndex(block_index(ptr, level)));
  flip_bit(_allocatedBlocks, mapIndex(block_index(ptr, level)));
  void *buddy = get_buddy(ptr, level);

  // std::cout << "dealloc map index: " << mapIndex(block_index(ptr, level)) <<
  // std::endl; std::cout << "allocated? " << bit_is_set(_allocatedBlocks,
  // mapIndex(block_index(ptr, level))) << std::endl;
  while (level > 0 &&
         !bit_is_set(_allocatedBlocks, mapIndex(block_index(ptr, level)))) {

    BUDDY_DBG("merging " << block_index(ptr, level) << " with buddy "
                         << buddy_index(ptr, level) << " at "
                         << reinterpret_cast<void *>(buddy) - _start
                         << std::endl);

    if (level < NUM_LISTS - 1) {
      BUDDY_DBG("clearing split_idx " << block_index(ptr, level));
      clear_bit(_splitBlocks, block_index(static_cast<void *>(ptr), level));
    }
    list_remove(reinterpret_cast<double_link *>(buddy));
    if (buddy < ptr) {
      ptr = buddy;
    }

    level--;

    if (level > 0) {
      buddy = get_buddy(ptr, level);
      BUDDY_DBG("flipping map index: " << mapIndex(block_index(ptr, level)));
      flip_bit(_allocatedBlocks, mapIndex(block_index(ptr, level)));
    }
  }

  if (level < NUM_LISTS - 1) {
    BUDDY_DBG("clearing split_idx " << block_index(ptr, level));
    clear_bit(_splitBlocks, block_index(static_cast<void *>(ptr), level));
  }
  push_back(&_freeList[level], reinterpret_cast<double_link *>(ptr));

  // print_free_list();
}

void BinaryBuddyAllocator::print_free_list() {
  for (std::size_t i = 0; i < NUM_LISTS; i++) {
    std::cout << "Free list " << i << "(" << (1 << (MAX_SIZE_LOG2 - i))
              << "): ";
    for (double_link *link = _freeList[i].next; link != &_freeList[i];
         link = link->next) {
      std::cout << reinterpret_cast<void *>(
                       reinterpret_cast<std::uintptr_t>(link) - _start)
                << " ";
    }
    std::cout << std::endl;
  }
}

void BinaryBuddyAllocator::print_bitmaps() {
  std::cout << "Allocated blocks: ";
  for (int i = 0; i < ((1 << (NUM_LISTS - 1)) / 16) + 1; i++) {
    std::cout << static_cast<int>(_allocatedBlocks[i]) << " ";
  }
  std::cout << std::endl;

  std::cout << "Split blocks: ";
  for (int i = 0; i < ((1 << (NUM_LISTS - 1)) / 8) + 1; i++) {
    std::cout << static_cast<int>(_splitBlocks[i]) << " ";
  }
  std::cout << std::endl;
}

// Finds the smallest block index that can fit the given size
// int BinaryBuddyAllocator::find_smallest_block_index(std::size_t size) {
//   for (int i = MAX_SIZE_LOG2 - MIN_SIZE_LOG2; i >= 0; i--) {
//     BUDDY_DBG("find block atleast size: " << size);
//     BUDDY_DBG("1 << (i + MIN_SIZE_LOG2): " << (1 << (MAX_SIZE_LOG2 - i)));
//     // std::cout << "i: " << i << std::endl;
//     if (size <= static_cast<size_t>(1 << (MAX_SIZE_LOG2 - i))) {
//       // std::cout << "i return: " << i << std::endl;
//       return i;
//     }
//   }
//   return -1;
// }

int BinaryBuddyAllocator::find_smallest_block_index(std::size_t size) {
  for (int i = MIN_SIZE_LOG2; i <= MAX_SIZE_LOG2; i++) {
    // BUDDY_DBG("find block atleast size: " << size);
    // BUDDY_DBG("1 << i: " << (1 << i));
    // std::cout << "i: " << i << std::endl;
    if (size <= static_cast<size_t>(1 << i)) {
      // std::cout << "i return: " << i << std::endl;
      BUDDY_DBG("level: " << MAX_SIZE_LOG2 - i << " size: " << (1 << i));
      return MAX_SIZE_LOG2 - i;
    }
  }
  return -1;
}