#pragma once

#include <cstddef>
#include <cstdint>

#define MAX_SIZE_LOG2 25 // 2^21 = 2MB
#define MIN_SIZE_LOG2 4  // 2^4 = 16
#define NUM_LEVELS (MAX_SIZE_LOG2 - MIN_SIZE_LOG2 + 1)
#define BITMAP_SIZE (1 << (NUM_LEVELS)) / 8
#define LAZY_THRESHOLD 31

struct double_link {
  double_link *prev;
  double_link *next;
};

class IBuddyAllocator {
public:
  IBuddyAllocator(void *start, size_t totalSize, int minBlockSizeLog2,
                  int maxBlockSizeLog2);
  ~IBuddyAllocator();

  static IBuddyAllocator *create(void *addr, void *start, size_t totalSize,
                                 int minBlockSizeLog2, int maxBlockSizeLog2);

  size_t get_alloc_size(void *ptr);

  void *allocate(size_t size);
  void deallocate(void *ptr);
  void deallocate(void *ptr, size_t size);
  void deallocate_range(void *ptr, size_t size);

  void print_free_list();
  void print_bitmaps();

private:
  int index_in_level(void *ptr, int level);
  int index_of_level(int level);
  int size_of_level(int level);
  int block_index(void *ptr, int level);
  int buddy_index(void *ptr, int level);
  int get_level(void *ptr);
  int num_blocks(size_t size, int level);
  void *get_buddy(void *ptr, int level);
  void *align_left(void *ptr, int level);
  void deallocate_single(void *ptr);
  void split_bits(void *ptr, int level_start, int level_end);
  void alloc_size(void *ptr);

  int find_smallest_block_level(size_t size);

private:
  // Private member variables and helper functions

  uintptr_t _start;
  size_t _totalSize;
  int _numLevels;
  int _minBlockSizeLog2;
  int _maxBlockSizeLog2;
  int _minSize = 1 << MIN_SIZE_LOG2;
  int _maxSize = 1 << MAX_SIZE_LOG2;

  int _topLevel = 0;

  // Array of free lists for each block size
  double_link _freeList[NUM_LEVELS];

  double_link _lazyList;
  int _lazyListSize = 0;

  // Bitmap of allocated blocks
  //   unsigned char _allocatedBlocks[((1 << (NUM_LEVELS - 1)) / 16) + 2] = {0};
  unsigned char _freeBlocks[BITMAP_SIZE] = {0};

  // Bitmap of split blocks
  unsigned char _splitBlocks[((1 << (NUM_LEVELS - 1)) / 8) + 1] = {0};
};
