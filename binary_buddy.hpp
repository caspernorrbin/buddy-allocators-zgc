#pragma once

#include <cstddef>
#include <cstdint>

#define MAX_SIZE_LOG2 21 // 2^21 = 2MB
#define MIN_SIZE_LOG2 4  // 2^4 = 16
#define NUM_LISTS (MAX_SIZE_LOG2 - MIN_SIZE_LOG2 + 1)
#define BITMAP_SIZE (1 << (NUM_LISTS - 1)) / 8

struct double_link {
  double_link *prev;
  double_link *next;
};

class BinaryBuddyAllocator {
public:
  BinaryBuddyAllocator(void *start, size_t totalSize, int minBlockSizeLog2,
                       int maxBlockSizeLog2);
  ~BinaryBuddyAllocator();

  static BinaryBuddyAllocator *create(void *addr, void *start, size_t totalSize,
                                      int minBlockSizeLog2,
                                      int maxBlockSizeLog2);

  void *allocate(size_t size);
  void deallocate(void *ptr);

  void print_free_list();
  void print_bitmaps();

private:
  int index_in_level(void *p, int level);
  int index_of_level(int level);
  int size_of_level(int level);
  int block_index(void *p, int level);
  int buddy_index(void *p, int level);
  int get_level(void *p);
  void *get_buddy(void *p, int level);

  int find_smallest_block_index(size_t size);

private:
  // Private member variables and helper functions

  uintptr_t _start;
  size_t _totalSize;
  int _numLevels;
  int _minBlockSizeLog2;
  int _maxBlockSizeLog2;

  // Array of free lists for each block size
  double_link _freeList[NUM_LISTS];
  // double_link _freeList[];

  // XOR Bitmap of allocated blocks
//   unsigned char _allocatedBlocks[((1 << (NUM_LISTS - 1)) / 16) + 2] = {0}; 
  unsigned char _allocatedBlocks[BITMAP_SIZE] = {0}; 

  // Bitmap of split blocks
  unsigned char _splitBlocks[((1 << (NUM_LISTS - 1)) / 8) + 1] = {0};
};
