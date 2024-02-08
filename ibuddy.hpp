#ifndef IBUDDY_HPP_
#define IBUDDY_HPP_

#include <array>
#include <cstddef>
#include <cstdint>

template <unsigned int MIN_BLOCK_SIZE_LOG2, unsigned int MAX_BLOCK_SIZE_LOG2,
          int NUM_REGIONS, int SIZE_BITS>
struct IBuddyConfig {
  static const int minBlockSizeLog2 = MIN_BLOCK_SIZE_LOG2;
  static const int maxBlockSizeLog2 = MAX_BLOCK_SIZE_LOG2;
  static const int minBlockSize = 1U << MIN_BLOCK_SIZE_LOG2;
  static const int maxBlockSize = 1U << MAX_BLOCK_SIZE_LOG2;
  static const unsigned char numLevels =
      MAX_BLOCK_SIZE_LOG2 - MIN_BLOCK_SIZE_LOG2 + 1;
  static const int numRegions = NUM_REGIONS;
  static const int allocedBitmapSize = (1U << (numLevels)) / 8;
  static const int sizeBits = SIZE_BITS;
  static const int sizeBitmapSize =
      (SIZE_BITS == 0)
          ? (1U << (numLevels - 1U)) / 8
          : SIZE_BITS * (maxBlockSize * NUM_REGIONS) / minBlockSize / 8;
};

struct double_link {
  double_link *prev;
  double_link *next;
};

template <typename Config> class IBuddyAllocator {
public:
  IBuddyAllocator(void *start, int lazyThreshold, bool startFull);
  ~IBuddyAllocator() = default;
  IBuddyAllocator(const IBuddyAllocator &) = delete;
  IBuddyAllocator &operator=(const IBuddyAllocator &) = delete;

  static IBuddyAllocator *create(void *addr, void *start, int lazyThreshold,
                                 bool startFull);

  size_t get_alloc_size(uintptr_t ptr);

  void *allocate(size_t size);
  void deallocate(void *ptr);
  void deallocate(void *ptr, size_t size);
  void deallocate_range(void *ptr, size_t size);
  void empty_lazy_list();

  void print_free_list();
  void print_bitmaps();

private:
  unsigned int size_of_level(uint8_t level);
  unsigned int index_in_level(uintptr_t ptr, uint8_t level);
  unsigned int index_of_level(uint8_t level);
  unsigned int block_index(uintptr_t ptr, uint8_t level);
  unsigned int buddy_index(uintptr_t ptr, uint8_t level);
  uint8_t get_level(uintptr_t ptr);
  unsigned int num_blocks(size_t size, uint8_t level);
  uintptr_t get_buddy(uintptr_t ptr, uint8_t level);
  uintptr_t align_left(uintptr_t ptr, uint8_t level);
  void deallocate_single(uintptr_t ptr);
  void split_bits(uintptr_t ptr, uint8_t level_start, uint8_t level_end);
  void set_level(uintptr_t ptr, uint8_t level);
  void alloc_size(void *ptr);

  int find_smallest_block_level(size_t size);

  uintptr_t _start;
  size_t _totalSize;

  const uint8_t _numLevels = Config::numLevels;
  const int _minBlockSizeLog2 = Config::minBlockSizeLog2;
  const int _maxBlockSizeLog2 = Config::maxBlockSizeLog2;
  const int _minSize = 1 << Config::minBlockSizeLog2;
  const int _maxSize = 1 << Config::maxBlockSizeLog2;
  const int _sizeBits = Config::sizeBits;
  const bool _sizeMapEnabled = Config::sizeBits == 0;

  int _lazyThreshold = 0;
  int _topLevel = 0;

  // Array of free lists for each block size
  double_link _freeList[Config::numLevels];

  double_link _lazyList = {&_lazyList, &_lazyList};
  int _lazyListSize = 0;

  // Bitmap of allocated blocks
  unsigned char _freeBlocks[Config::allocedBitmapSize];

  // Bitmap of either split blocks or allocated block sizes
  unsigned char _sizeMap[Config::sizeBitmapSize];
};

#endif // IBUDDY_HPP