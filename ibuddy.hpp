#pragma once

#include <cstddef>
#include <cstdint>

template <int MIN_BLOCK_SIZE_LOG2, int MAX_BLOCK_SIZE_LOG2>
struct IBuddyConfig {
  static const int minBlockSizeLog2 = MIN_BLOCK_SIZE_LOG2;
  static const int maxBlockSizeLog2 = MAX_BLOCK_SIZE_LOG2;
  static const int numLevels = MAX_BLOCK_SIZE_LOG2 - MIN_BLOCK_SIZE_LOG2 + 1;
  static const int bitmapSize = (1 << (numLevels)) / 8;
};

struct double_link {
  double_link *prev;
  double_link *next;
};

template <typename Config>
class IBuddyAllocator {
public:
  IBuddyAllocator(void *start, size_t totalSize, int lazyThreshold);
  ~IBuddyAllocator();

  static IBuddyAllocator *create(void *addr, void *start, size_t totalSize, int lazyThreshold);

  size_t get_alloc_size(void *ptr);

  void *allocate(size_t size);
  void deallocate(void *ptr);
  void deallocate(void *ptr, size_t size);
  void deallocate_range(void *ptr, size_t size);
  void empty_lazy_list();

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

  uintptr_t _start;
  size_t _totalSize;

  int _numLevels = Config::numLevels;
  int _minBlockSizeLog2 = Config::minBlockSizeLog2;
  int _maxBlockSizeLog2 = Config::maxBlockSizeLog2;
  int _minSize = 1 << _minBlockSizeLog2;
  int _maxSize = 1 << _maxBlockSizeLog2;

  int _lazyThreshold = 0;
  int _topLevel = 0;

  // Array of free lists for each block size
  double_link _freeList[Config::numLevels];

  double_link _lazyList;
  int _lazyListSize = 0;

  // Bitmap of allocated blocks
  //   unsigned char _allocatedBlocks[((1 << (NUM_LEVELS - 1)) / 16) + 2] = {0};
  unsigned char _freeBlocks[Config::bitmapSize] = {0};

  // Bitmap of split blocks
  // unsigned char _splitBlocks[((1 << (NUM_LEVELS - 1)) / 8) + 1] = {0};
  unsigned char _splitBlocks[Config::bitmapSize / 2] = {0};
};
