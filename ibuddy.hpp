#ifndef IBUDDY_HPP_
#define IBUDDY_HPP_

#include <cstddef>
#include <cstdint>

template <unsigned int MIN_BLOCK_SIZE_LOG2, unsigned int MAX_BLOCK_SIZE_LOG2,
          int NUM_REGIONS, bool USE_SIZEMAP, size_t SIZE_BITS>
struct IBuddyConfig {
  static const size_t minBlockSizeLog2 = MIN_BLOCK_SIZE_LOG2;
  static const size_t maxBlockSizeLog2 = MAX_BLOCK_SIZE_LOG2;
  static const size_t minBlockSize = 1U << MIN_BLOCK_SIZE_LOG2;
  static const size_t maxBlockSize = 1U << MAX_BLOCK_SIZE_LOG2;
  static const unsigned char numLevels =
      MAX_BLOCK_SIZE_LOG2 - MIN_BLOCK_SIZE_LOG2 + 1;
  static const int numRegions = NUM_REGIONS;
  static const bool useSizeMap = USE_SIZEMAP;
  static const int allocedBitmapSize = (1U << (numLevels)) / 8;
  static const size_t sizeBits = SIZE_BITS;
  static const int sizeBitmapSize =
      !USE_SIZEMAP       ? 0
      : (SIZE_BITS == 0) ? (1U << (numLevels - 1U)) / 8
                         : SIZE_BITS * maxBlockSize / minBlockSize / 8;
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
  size_t free_size();
  void fill();

  void print_free_list();
  void print_bitmaps();

private:
  void init_bitmaps(bool startFull);
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
  void deallocate_single(uintptr_t ptr);
  void split_bits(uintptr_t ptr, uint8_t region, uint8_t level_start,
                  uint8_t level_end);
  void set_level(uintptr_t ptr, uint8_t region, uint8_t level);

  int find_smallest_block_level(size_t size);

  uintptr_t _start;
  size_t _freeSize;
  size_t _totalSize;

  const uint8_t _numRegions = Config::numRegions;
  const uint8_t _numLevels = Config::numLevels;
  const size_t _minBlockSizeLog2 = Config::minBlockSizeLog2;
  const size_t _maxBlockSizeLog2 = Config::maxBlockSizeLog2;
  const size_t _minSize = Config::minBlockSize;
  const size_t _maxSize = Config::maxBlockSize;
  const size_t _sizeBits = Config::sizeBits;
  const bool _sizeMapEnabled = Config::useSizeMap;
  const bool _sizeMapIsBitmap = Config::sizeBits == 0;

  int _lazyThreshold = 0;
  int _topLevel[Config::numRegions] = {0};

  // Array of free lists for each block size
  double_link _freeList[Config::numRegions][Config::numLevels];

  double_link _lazyList = {&_lazyList, &_lazyList};
  int _lazyListSize = 0;

  // Bitmap of allocated blocks
  unsigned char _freeBlocks[Config::numRegions][Config::allocedBitmapSize];

  // Bitmap of either split blocks or allocated block sizes
  unsigned char _sizeMap[Config::numRegions][Config::sizeBitmapSize];
};

#endif // IBUDDY_HPP