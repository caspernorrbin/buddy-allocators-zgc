#ifndef BUDDY_ALLOCATOR_HPP
#define BUDDY_ALLOCATOR_HPP

// Include necessary headers
#include "buddy_config.hpp"
#include "buddy_helper.hpp"
#include <cstddef>
#include <cstdint>

// Define the BuddyAllocator class

template <typename Config> class BuddyAllocator {
public:
  // Constructor
  BuddyAllocator(void *start, int lazyThreshold, bool startFull);
  BuddyAllocator() = default;
  BuddyAllocator(const BuddyAllocator &) = delete;

  // Destructor

  // Public member functions
  size_t get_alloc_size(uintptr_t ptr);
  size_t free_size();
  void *allocate(size_t size);
  void deallocate(void *ptr);
  void deallocate(void *ptr, size_t size);
  virtual void deallocate_range(void *ptr, size_t size) = 0;
  void empty_lazy_list();
  virtual void fill() = 0;

  void print_free_list();
  void print_bitmaps();

protected:
  void init_free_lists();
  uintptr_t region_start(uint8_t region);
  unsigned int size_of_level(uint8_t level);
  unsigned int index_in_level(uintptr_t ptr, uint8_t region, uint8_t level);
  unsigned int index_of_level(uint8_t level);
  unsigned int block_index(uintptr_t ptr, uint8_t region, uint8_t level);
  unsigned int buddy_index(uintptr_t ptr, uint8_t region, uint8_t level);
  uint8_t get_level(uintptr_t ptr);
  uint8_t get_region(uintptr_t ptr);
  unsigned int num_blocks(size_t size, uint8_t level);
  uintptr_t get_buddy(uintptr_t ptr, uint8_t level);
  uintptr_t align_left(uintptr_t ptr, uint8_t level);
  void split_bits(uintptr_t ptr, uint8_t region, uint8_t level_start,
                  uint8_t level_end);
  void set_level(uintptr_t ptr, uint8_t region, uint8_t level);
  int find_smallest_block_level(size_t size);
  void push_free_list(uintptr_t ptr, uint8_t region, uint8_t level);
  bool free_list_empty(uint8_t region, uint8_t level);
  uintptr_t pop_free_list(uint8_t region, uint8_t level);

  virtual void *allocate_internal(size_t size) = 0;
  virtual void deallocate_internal(void *ptr, size_t size) = 0;

  size_t _freeSize;

  const uint8_t _numRegions = Config::numRegions;
  const uint8_t _numLevels = Config::numLevels;
  const size_t _minBlockSizeLog2 = Config::minBlockSizeLog2;
  const size_t _maxBlockSizeLog2 = Config::maxBlockSizeLog2;
  const size_t _minSize = Config::minBlockSize;
  const size_t _maxSize = Config::maxBlockSize;
  const size_t _sizeBits = Config::sizeBits;
  const bool _sizeMapEnabled = Config::useSizeMap;
  const bool _sizeMapIsBitmap = Config::sizeBits == 0;

  int _topLevel[Config::numRegions] = {0};

  // Bitmap of either split blocks or allocated block sizes
  unsigned char _sizeMap[Config::numRegions][Config::sizeBitmapSize];

  // Bitmap of allocated blocks
  unsigned char _freeBlocks[Config::numRegions][Config::allocedBitmapSize];

// private:
  uintptr_t _start;
  size_t _totalSize;

  // Array of free lists for each block size
  double_link _freeList[Config::numRegions][Config::numLevels];
  // Private member variables
  int _lazyThreshold = 0;

  double_link _lazyList = {&_lazyList, &_lazyList};
  int _lazyListSize = 0;

  // Private member functions
};

#endif // BUDDY_ALLOCATOR_HPP
