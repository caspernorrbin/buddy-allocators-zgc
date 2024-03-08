#ifndef BUDDY_CONFIG_HPP_
#define BUDDY_CONFIG_HPP_

#include <cstddef>

template <unsigned int MIN_BLOCK_SIZE_LOG2, unsigned int MAX_BLOCK_SIZE_LOG2,
          int NUM_REGIONS, bool USE_SIZEMAP, size_t SIZE_BITS>
struct BuddyConfig {
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

using ZConfig = BuddyConfig<4, 18, 8, true, 4>;
using SmallSingleConfig = BuddyConfig<4, 8, 1, true, 0>;
using SmallDoubleConfig = BuddyConfig<4, 8, 2, true, 4>;
using LargeQuadConfig = BuddyConfig<4, 21, 4, true, 0>;
using MallocConfig = BuddyConfig<4, 26, 20, true, 0>;

#endif // BUDDY_CONFIG_HPP_