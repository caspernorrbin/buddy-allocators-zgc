#include "../include/btbuddy.hpp"
#include "../include/btbuddy_instantiations.hpp"
#include "../include/buddy_allocator.hpp"
#include "../include/buddy_config.hpp"
#include "../include/buddy_instantiations.hpp"
#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestSuite.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cstddef>
#include <cstdint>
#include <vector>

class SmallSingleAllocatorTests : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SmallSingleAllocatorTests);
  CPPUNIT_TEST(testAllocateSingle);
  CPPUNIT_TEST(testAllocateDouble);
  CPPUNIT_TEST(testAllocateWhole);
  CPPUNIT_TEST(testAllocateDoubleHalf);
  CPPUNIT_TEST(testSize);
  CPPUNIT_TEST(testSizeFree);
  CPPUNIT_TEST(testSizeDecrease);
  CPPUNIT_TEST(testSizeIncrease);
  CPPUNIT_TEST(testAllocateFillBlocks);
  CPPUNIT_TEST(testAllocateFillLargeBlocks);
  CPPUNIT_TEST(testAllocateAllSizes);
  CPPUNIT_TEST(testAllocateFillAllSizes);
  CPPUNIT_TEST(testAllCombined);
  CPPUNIT_TEST_SUITE_END();

public:
  void testAllocateSingle() {
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        smallSingleAllocator == nullptr ? get_small_single_allocator()
                                        : smallSingleAllocator;

    void *p = allocator->allocate(_minSize);
    CPPUNIT_ASSERT(p != nullptr);
    allocator->deallocate(p);
  }

  void testAllocateDouble() {
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        smallSingleAllocator == nullptr ? get_small_single_allocator()
                                        : smallSingleAllocator;

    void *p = allocator->allocate(17);
    CPPUNIT_ASSERT(p != nullptr);
    allocator->deallocate(p);
  }

  void testAllocateWhole() {
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        smallSingleAllocator == nullptr ? get_small_single_allocator()
                                        : smallSingleAllocator;

    void *p = allocator->allocate(_maxSize);
    CPPUNIT_ASSERT(p != nullptr);
    allocator->deallocate(p);
  }

  void testAllocateDoubleHalf() {
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        smallSingleAllocator == nullptr ? get_small_single_allocator()
                                        : smallSingleAllocator;

    void *p = allocator->allocate(_maxSize / 2);
    void *p2 = allocator->allocate(_maxSize / 2);

    CPPUNIT_ASSERT(p != nullptr);
    CPPUNIT_ASSERT(p2 != nullptr);

    allocator->deallocate(p);
    allocator->deallocate(p2);
  }

  void testSize() {
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        smallSingleAllocator == nullptr ? get_small_single_allocator()
                                        : smallSingleAllocator;

    CPPUNIT_ASSERT(allocator->free_size() == _maxSize);
    void *p = allocator->allocate(_maxSize);
    CPPUNIT_ASSERT(p != nullptr);
    allocator->deallocate(p);
  }

  void testSizeFree() {
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        smallSingleAllocator == nullptr ? get_small_single_allocator()
                                        : smallSingleAllocator;
    const size_t size = allocator->free_size();

    void *p = allocator->allocate(_maxSize / 2);
    CPPUNIT_ASSERT(allocator->free_size() == size - _maxSize / 2);

    allocator->deallocate(p);
    CPPUNIT_ASSERT(allocator->free_size() == size);

    void *p2 = allocator->allocate(_maxSize);
    CPPUNIT_ASSERT(p2 != nullptr);
    allocator->deallocate(p2);
  }

  void testSizeDecrease() {
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        smallSingleAllocator == nullptr ? get_small_single_allocator()
                                        : smallSingleAllocator;
    const size_t size = allocator->free_size();

    void *p = allocator->allocate(_maxSize / 2);
    CPPUNIT_ASSERT(allocator->free_size() == size - (_maxSize / 2));

    void *p2 = allocator->allocate(_minSize * 2);
    CPPUNIT_ASSERT(allocator->free_size() ==
                   size - (_maxSize / 2) - (_minSize * 2));

    CPPUNIT_ASSERT(p != nullptr);
    CPPUNIT_ASSERT(p2 != nullptr);

    allocator->deallocate(p);
    allocator->deallocate(p2);
  }

  void testSizeIncrease() {
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        smallSingleAllocator == nullptr ? get_small_single_allocator()
                                        : smallSingleAllocator;
    const size_t size = allocator->free_size();

    void *p = allocator->allocate(_maxSize / 2);
    CPPUNIT_ASSERT(allocator->free_size() == size - (_maxSize / 2));

    allocator->deallocate(p);
    CPPUNIT_ASSERT(allocator->free_size() == size);

    void *p2 = allocator->allocate(_maxSize);
    CPPUNIT_ASSERT(allocator->free_size() == size - _maxSize);
    allocator->deallocate(p2);
  }

  void testAllocateFillBlocks() {
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        smallSingleAllocator == nullptr ? get_small_single_allocator()
                                        : smallSingleAllocator;
    std::vector<void *> blocks;
    const int size = _maxSize / _minSize;

    for (int i = 0; i < size; i++) {
      void *p = allocator->allocate(_minSize);
      CPPUNIT_ASSERT(p != nullptr);
      blocks.push_back(p);
    }

    CPPUNIT_ASSERT(allocator->allocate(_minSize) == nullptr);
    CPPUNIT_ASSERT(allocator->free_size() == 0);

    for (int i = 0; i < size; i++) {
      allocator->deallocate(blocks[i]);
    }

    CPPUNIT_ASSERT(allocator->free_size() == _maxSize);

    void *p2 = allocator->allocate(_maxSize);
    CPPUNIT_ASSERT(p2 != nullptr);
    allocator->deallocate(p2);
  }

  void testAllocateFillLargeBlocks() {
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        smallSingleAllocator == nullptr ? get_small_single_allocator()
                                        : smallSingleAllocator;
    std::vector<void *> blocks;
    const int size = _maxSize / (_minSize * 4);

    for (int i = 0; i < size; i++) {
      void *p = allocator->allocate(_minSize * 4);
      CPPUNIT_ASSERT(p != nullptr);
      blocks.push_back(p);
    }

    CPPUNIT_ASSERT(allocator->allocate(_minSize) == nullptr);
    CPPUNIT_ASSERT(allocator->free_size() == 0);

    for (int i = 0; i < size; i++) {
      allocator->deallocate(blocks[i]);
    }

    CPPUNIT_ASSERT(allocator->free_size() == _maxSize);

    void *p2 = allocator->allocate(_maxSize);
    CPPUNIT_ASSERT(p2 != nullptr);
    allocator->deallocate(p2);
  }

  void testAllocateAllSizes() {
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        smallSingleAllocator == nullptr ? get_small_single_allocator()
                                        : smallSingleAllocator;
    std::vector<void *> blocks;

    for (size_t i = _maxSize / 2; i >= _minSize; i /= 2) {
      void *p = allocator->allocate(i);
      CPPUNIT_ASSERT(p != nullptr);
      blocks.push_back(p);
    }

    CPPUNIT_ASSERT(allocator->free_size() == _minSize);

    void *p2 = allocator->allocate(_minSize);
    CPPUNIT_ASSERT(p2 != nullptr);
    CPPUNIT_ASSERT(allocator->allocate(_minSize) == nullptr);

    allocator->deallocate(p2);

    auto it = blocks.begin();
    while (it != blocks.end()) {
      allocator->deallocate(*it);
      it = blocks.erase(it);
    }
  }

  void testAllocateFillAllSizes() {
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        smallSingleAllocator == nullptr ? get_small_single_allocator()
                                        : smallSingleAllocator;
    std::vector<void *> blocks;

    for (size_t i = _maxSize; i >= _minSize; i /= 2) {
      CPPUNIT_ASSERT(allocator->free_size() == _maxSize);

      for (size_t j = 0; j < _maxSize / i; j++) {
        void *p = allocator->allocate(i);
        CPPUNIT_ASSERT(p != nullptr);
        blocks.push_back(p);
      }

      CPPUNIT_ASSERT(allocator->free_size() == 0);
      CPPUNIT_ASSERT(allocator->allocate(i) == nullptr);

      auto it = blocks.begin();
      while (it != blocks.end()) {
        allocator->deallocate(*it);
        it = blocks.erase(it);
      }

      CPPUNIT_ASSERT(allocator->free_size() == _maxSize);
    }

    void *p2 = allocator->allocate(_maxSize);
    CPPUNIT_ASSERT(p2 != nullptr);

    allocator->deallocate(p2);
  }

  void testAllCombined() {
    smallSingleAllocator = get_small_single_allocator();

    testAllocateSingle();
    testAllocateDouble();
    testAllocateWhole();
    testAllocateDoubleHalf();
    testSize();
    testSizeFree();
    testSizeDecrease();
    testSizeIncrease();
    testAllocateFillBlocks();
    testAllocateFillLargeBlocks();
    testAllocateAllSizes();
    testAllocateFillAllSizes();

    CPPUNIT_ASSERT(smallSingleAllocator->free_size() == _maxSize);
    void *p = smallSingleAllocator->allocate(_maxSize);
    CPPUNIT_ASSERT(p != nullptr);
    smallSingleAllocator->deallocate(p);
  }

private:
  BTBuddyAllocator<SmallSingleConfig> *smallSingleAllocator = nullptr;
  static const size_t _minSize = 16;
  static const size_t _maxSize = 256;

  static BTBuddyAllocator<SmallSingleConfig> *get_small_single_allocator() {
    return BTBuddyAllocator<SmallSingleConfig>::create(nullptr, nullptr, 0,
                                                           false);
  }
};

class SmallDoubleAllocatorTests : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SmallDoubleAllocatorTests);
  CPPUNIT_TEST(testAllocateFillBlocksDouble);
  CPPUNIT_TEST(testAllocateFillLargeBlocksDouble);
  CPPUNIT_TEST(testAllocateAllSizesDouble);
  CPPUNIT_TEST(testAllocateFillAllSizesDouble);
  CPPUNIT_TEST(testAllCombined);
  CPPUNIT_TEST_SUITE_END();

  void testAllocateFillBlocksDouble() {
    BTBuddyAllocator<SmallDoubleConfig> *allocator =
        smallDoubleAllocator == nullptr ? get_small_double_allocator()
                                        : smallDoubleAllocator;
    std::vector<void *> blocks;
    const int size = _maxSize * 2 / _minSize;

    for (int i = 0; i < size; i++) {
      void *p = allocator->allocate(_minSize);
      CPPUNIT_ASSERT(p != nullptr);
      blocks.push_back(p);
    }

    CPPUNIT_ASSERT(allocator->allocate(_minSize) == nullptr);
    CPPUNIT_ASSERT(allocator->free_size() == 0);

    for (int i = 0; i < size; i++) {
      allocator->deallocate(blocks[i]);
    }

    CPPUNIT_ASSERT(allocator->free_size() == _maxSize * 2);

    void *p2 = allocator->allocate(_maxSize);
    void *p3 = allocator->allocate(_maxSize);

    CPPUNIT_ASSERT(p2 != nullptr);
    CPPUNIT_ASSERT(p3 != nullptr);

    allocator->deallocate(p2);
    allocator->deallocate(p3);
  }

  void testAllocateFillLargeBlocksDouble() {
    BTBuddyAllocator<SmallDoubleConfig> *allocator =
        smallDoubleAllocator == nullptr ? get_small_double_allocator()
                                        : smallDoubleAllocator;
    std::vector<void *> blocks;
    const int size = _maxSize * 2 / (_minSize * 4);

    for (int i = 0; i < size; i++) {
      void *p = allocator->allocate(_minSize * 4);
      CPPUNIT_ASSERT(p != nullptr);
      blocks.push_back(p);
    }

    CPPUNIT_ASSERT(allocator->allocate(_minSize) == nullptr);
    CPPUNIT_ASSERT(allocator->free_size() == 0);

    for (int i = 0; i < size; i++) {
      allocator->deallocate(blocks[i]);
    }

    CPPUNIT_ASSERT(allocator->free_size() == _maxSize * 2);

    void *p2 = allocator->allocate(_maxSize);
    void *p3 = allocator->allocate(_maxSize);

    CPPUNIT_ASSERT(p2 != nullptr);
    CPPUNIT_ASSERT(p3 != nullptr);

    allocator->deallocate(p2);
    allocator->deallocate(p3);
  }

  void testAllocateAllSizesDouble() {
    BTBuddyAllocator<SmallDoubleConfig> *allocator =
        smallDoubleAllocator == nullptr ? get_small_double_allocator()
                                        : smallDoubleAllocator;
    std::vector<void *> blocks;
    const int size = _maxSize / 2;

    for (int i = 0; i < 2; i++) {
      for (size_t j = size; j >= _minSize; j /= 2) {
        void *p = allocator->allocate(j);
        CPPUNIT_ASSERT(p != nullptr);
        blocks.push_back(p);
      }
    }

    CPPUNIT_ASSERT(allocator->free_size() == (_minSize * 2));

    void *p2 = allocator->allocate(_minSize);
    void *p3 = allocator->allocate(_minSize);

    CPPUNIT_ASSERT(allocator->allocate(_minSize) == nullptr);
    CPPUNIT_ASSERT(p2 != nullptr);
    CPPUNIT_ASSERT(p3 != nullptr);

    allocator->deallocate(p2);
    allocator->deallocate(p3);

    auto it = blocks.begin();
    while (it != blocks.end()) {
      allocator->deallocate(*it);
      it = blocks.erase(it);
    }
  }

  void testAllocateFillAllSizesDouble() {
    BTBuddyAllocator<SmallDoubleConfig> *allocator =
        smallDoubleAllocator == nullptr ? get_small_double_allocator()
                                        : smallDoubleAllocator;
    std::vector<void *> blocks;

    for (size_t i = _maxSize; i >= _minSize; i /= 2) {
      CPPUNIT_ASSERT(allocator->free_size() == (_maxSize * 2));

      for (size_t j = 0; j < (_maxSize * 2) / i; j++) {
        void *p = allocator->allocate(i);
        CPPUNIT_ASSERT(p != nullptr);
        blocks.push_back(p);
      }

      CPPUNIT_ASSERT(allocator->free_size() == 0);
      CPPUNIT_ASSERT(allocator->allocate(i) == nullptr);

      auto it = blocks.begin();
      while (it != blocks.end()) {
        allocator->deallocate(*it);
        it = blocks.erase(it);
      }

      CPPUNIT_ASSERT(allocator->free_size() == (_maxSize * 2));
    }

    void *p2 = allocator->allocate(_maxSize);
    void *p3 = allocator->allocate(_maxSize);

    CPPUNIT_ASSERT(p2 != nullptr);
    CPPUNIT_ASSERT(p3 != nullptr);

    allocator->deallocate(p2);
    allocator->deallocate(p3);
  }

  void testAllCombined() {
    smallDoubleAllocator = get_small_double_allocator();

    testAllocateFillBlocksDouble();
    testAllocateFillLargeBlocksDouble();
    testAllocateAllSizesDouble();
    testAllocateFillAllSizesDouble();

    CPPUNIT_ASSERT(smallDoubleAllocator->free_size() == (_maxSize * 2));

    void *p = smallDoubleAllocator->allocate(_maxSize);
    void *p2 = smallDoubleAllocator->allocate(_maxSize);

    CPPUNIT_ASSERT(p != nullptr);
    CPPUNIT_ASSERT(p2 != nullptr);

    smallDoubleAllocator->deallocate(p);
    smallDoubleAllocator->deallocate(p2);
  }

private:
  BTBuddyAllocator<SmallDoubleConfig> *smallDoubleAllocator = nullptr;
  static const size_t _minSize = 16;
  static const size_t _maxSize = 256;

  static BTBuddyAllocator<SmallDoubleConfig> *get_small_double_allocator() {
    return BTBuddyAllocator<SmallDoubleConfig>::create(nullptr, nullptr, 0,
                                                           false);
  }
};

class SmallSingleFilledAllocatorTests : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SmallSingleFilledAllocatorTests);
  CPPUNIT_TEST(testStartFull);
  CPPUNIT_TEST(testClearSingle);
  CPPUNIT_TEST(testClearStartOffset);
  CPPUNIT_TEST(testClearEndOffset);
  CPPUNIT_TEST(testClearStartEndOffset);
  CPPUNIT_TEST(testClearZeroSize);
  CPPUNIT_TEST(testClearZeroSizeOffset);
  CPPUNIT_TEST(testClearSmall);
  CPPUNIT_TEST(testClearSmallOffset);
  CPPUNIT_TEST(testClearFill);
  CPPUNIT_TEST(testClearFillTwice);
  CPPUNIT_TEST(testClearFull);
  CPPUNIT_TEST(testClearFullParts);
  CPPUNIT_TEST(testClearPart);
  CPPUNIT_TEST(testClearParts);
  CPPUNIT_TEST(testClearFillAlternate);
  CPPUNIT_TEST(testClearFullAlternate);
  CPPUNIT_TEST(testClearFullAlternateFill);
  CPPUNIT_TEST_SUITE_END();

public:
  void testStartFull() {
    uint8_t mempool[_maxSize];
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        get_small_filled_allocator(mempool);

    CPPUNIT_ASSERT(allocator->free_size() == 0);
    CPPUNIT_ASSERT(allocator->allocate(_minSize) == nullptr);
  }

  void testClearSingle() {
    uint8_t mempool[_maxSize];
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        get_small_filled_allocator(mempool);

    allocator->deallocate_range(mempool, _minSize);

    CPPUNIT_ASSERT(allocator->free_size() == _minSize);
    CPPUNIT_ASSERT(allocator->allocate(_minSize * 2) == nullptr);
    CPPUNIT_ASSERT(allocator->allocate(_minSize) != nullptr);
  }

  void testClearStartOffset() {
    uint8_t mempool[_maxSize];
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        get_small_filled_allocator(mempool);

    allocator->deallocate_range(mempool + 7, _minSize * 4 - 7);

    CPPUNIT_ASSERT(allocator->free_size() == _minSize * 3);
    CPPUNIT_ASSERT(allocator->allocate(_minSize * 2) != nullptr);
    CPPUNIT_ASSERT(allocator->allocate(_minSize) != nullptr);
  }

  void testClearEndOffset() {
    uint8_t mempool[_maxSize];
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        get_small_filled_allocator(mempool);

    allocator->deallocate_range(mempool, _minSize * 4 + 7);

    CPPUNIT_ASSERT(allocator->free_size() == _minSize * 4);
    CPPUNIT_ASSERT(allocator->allocate(_minSize * 4) != nullptr);
  }

  void testClearStartEndOffset() {
    uint8_t mempool[_maxSize];
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        get_small_filled_allocator(mempool);

    allocator->deallocate_range(mempool + _minSize / 2, _minSize * 4);

    CPPUNIT_ASSERT(allocator->free_size() == _minSize * 3);
    CPPUNIT_ASSERT(allocator->allocate(_minSize * 2) != nullptr);
    CPPUNIT_ASSERT(allocator->allocate(_minSize) != nullptr);
  }

  void testClearZeroSize() {
    uint8_t mempool[_maxSize];
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        get_small_filled_allocator(mempool);

    allocator->deallocate_range(mempool, 0);

    CPPUNIT_ASSERT(allocator->free_size() == 0);
    CPPUNIT_ASSERT(allocator->allocate(_minSize) == nullptr);
  }

  void testClearZeroSizeOffset() {
    uint8_t mempool[_maxSize];
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        get_small_filled_allocator(mempool);

    allocator->deallocate_range(mempool + _minSize / 2, 0);

    CPPUNIT_ASSERT(allocator->free_size() == 0);
    CPPUNIT_ASSERT(allocator->allocate(_minSize) == nullptr);
  }

  void testClearSmall() {
    uint8_t mempool[_maxSize];
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        get_small_filled_allocator(mempool);

    allocator->deallocate_range(mempool + _minSize / 2, _minSize);

    CPPUNIT_ASSERT(allocator->free_size() == 0);
    CPPUNIT_ASSERT(allocator->allocate(_minSize) == nullptr);
  }

  void testClearSmallOffset() {
    uint8_t mempool[_maxSize];
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        get_small_filled_allocator(mempool);

    allocator->deallocate_range(mempool, _minSize / 2);

    CPPUNIT_ASSERT(allocator->free_size() == 0);
    CPPUNIT_ASSERT(allocator->allocate(_minSize) == nullptr);
  }

  void testClearFill() {
    uint8_t mempool[_maxSize];
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        get_small_filled_allocator(mempool);

    allocator->deallocate_range(mempool, _minSize);

    CPPUNIT_ASSERT(allocator->free_size() == _minSize);

    void *p = allocator->allocate(_minSize);
    CPPUNIT_ASSERT(p != nullptr);

    allocator->fill();

    CPPUNIT_ASSERT(allocator->free_size() == 0);
    CPPUNIT_ASSERT(allocator->allocate(_minSize) == nullptr);

    allocator->deallocate_range(mempool, _maxSize);

    CPPUNIT_ASSERT(allocator->free_size() == _maxSize);
    CPPUNIT_ASSERT(allocator->allocate(_maxSize) != nullptr);
  }

  void testClearFillTwice() {
    uint8_t mempool[_maxSize];
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        get_small_filled_allocator(mempool);

    allocator->deallocate_range(mempool, _maxSize);

    CPPUNIT_ASSERT(allocator->free_size() == _maxSize);

    void *p = allocator->allocate(_maxSize);
    CPPUNIT_ASSERT(p != nullptr);

    allocator->fill();

    CPPUNIT_ASSERT(allocator->free_size() == 0);
    CPPUNIT_ASSERT(allocator->allocate(_minSize) == nullptr);

    allocator->deallocate_range(p, _minSize);

    CPPUNIT_ASSERT(allocator->free_size() == _minSize);
    CPPUNIT_ASSERT(allocator->allocate(_minSize * 2) == nullptr);
    CPPUNIT_ASSERT(allocator->allocate(_minSize) != nullptr);

    allocator->fill();

    CPPUNIT_ASSERT(allocator->free_size() == 0);
    CPPUNIT_ASSERT(allocator->allocate(_minSize) == nullptr);
  }

  void testClearFull() {
    uint8_t mempool[_maxSize];
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        get_small_filled_allocator(mempool);

    allocator->deallocate_range(mempool, _maxSize);

    CPPUNIT_ASSERT(allocator->free_size() == _maxSize);
    CPPUNIT_ASSERT(allocator->allocate(_maxSize) != nullptr);
  }

  void testClearFullParts() {
    uint8_t mempool[_maxSize];
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        get_small_filled_allocator(mempool);

    for (size_t i = 0; i < _maxSize; i += (_minSize * 4)) {
      allocator->deallocate_range(mempool + i, _minSize * 4);
    }

    CPPUNIT_ASSERT(allocator->free_size() == _maxSize);
    CPPUNIT_ASSERT(allocator->allocate(_maxSize) != nullptr);
  }

  void testClearPart() {
    uint8_t mempool[_maxSize];
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        get_small_filled_allocator(mempool);

    allocator->deallocate_range(mempool + _minSize, _minSize * 4);

    CPPUNIT_ASSERT(allocator->free_size() == _minSize * 4);
    CPPUNIT_ASSERT(allocator->allocate(_minSize * 4) == nullptr);
    CPPUNIT_ASSERT(allocator->allocate(_minSize * 2) != nullptr);
    CPPUNIT_ASSERT(allocator->allocate(_minSize) != nullptr);
    CPPUNIT_ASSERT(allocator->allocate(_minSize) != nullptr);
  }

  void testClearParts() {
    uint8_t mempool[_maxSize];
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        get_small_filled_allocator(mempool);

    for (uint8_t *i = mempool; i < mempool + _maxSize; i += (_minSize * 2)) {
      allocator->deallocate_range(i, _minSize);
    }

    CPPUNIT_ASSERT(allocator->free_size() == (_maxSize / 2));
    CPPUNIT_ASSERT(allocator->allocate(_minSize * 2) == nullptr);
    CPPUNIT_ASSERT(allocator->allocate(_minSize) != nullptr);
  }

  void testClearFillAlternate() {
    uint8_t mempool[_maxSize];
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        get_small_filled_allocator(mempool);
    std::vector<void *> blocks;

    for (uint8_t *i = mempool; i < mempool + _maxSize; i += _minSize) {
      allocator->deallocate_range(i, _minSize);
      void *p = allocator->allocate(_minSize);
      CPPUNIT_ASSERT(p != nullptr);
      blocks.push_back(p);
    }

    CPPUNIT_ASSERT(allocator->free_size() == 0);
    CPPUNIT_ASSERT(allocator->allocate(_minSize) == nullptr);

    auto it = blocks.begin();
    while (it != blocks.end()) {
      allocator->deallocate(*it);
      it = blocks.erase(it);
    }

    CPPUNIT_ASSERT(allocator->free_size() == _maxSize);
    CPPUNIT_ASSERT(allocator->allocate(_maxSize) != nullptr);
  }

  void testClearFullAlternate() {
    uint8_t mempool[_maxSize];
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        get_small_filled_allocator(mempool);

    for (uint8_t *i = mempool; i < mempool + _maxSize; i += (_minSize * 2)) {
      allocator->deallocate_range(i, _minSize);
    }

    CPPUNIT_ASSERT(allocator->free_size() == (_maxSize / 2));
    CPPUNIT_ASSERT(allocator->allocate(_minSize * 2) == nullptr);

    for (uint8_t *i = mempool + _minSize; i < mempool + _maxSize;
         i += (_minSize * 2)) {
      allocator->deallocate_range(i, _minSize);
    }

    CPPUNIT_ASSERT(allocator->free_size() == _maxSize);
    CPPUNIT_ASSERT(allocator->allocate(_maxSize) != nullptr);
  }

  void testClearFullAlternateFill() {
    uint8_t mempool[_maxSize];
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        get_small_filled_allocator(mempool);
    std::vector<void *> blocks;

    for (uint8_t *i = mempool; i < mempool + _maxSize; i += (_minSize * 2)) {
      allocator->deallocate_range(i, _minSize);
    }

    for (size_t i = 0; i < _maxSize; i += (_minSize * 2)) {
      void *p = allocator->allocate(_minSize);
      CPPUNIT_ASSERT(p != nullptr);
      blocks.push_back(p);
    }

    for (uint8_t *i = mempool + _minSize; i < mempool + _maxSize;
         i += (_minSize * 2)) {
      allocator->deallocate_range(i, _minSize);
    }

    for (size_t i = 0; i < _maxSize; i += (_minSize * 2)) {
      void *p = allocator->allocate(_minSize);
      CPPUNIT_ASSERT(p != nullptr);
      blocks.push_back(p);
    }

    CPPUNIT_ASSERT(allocator->free_size() == 0);
    CPPUNIT_ASSERT(allocator->allocate(_maxSize) == nullptr);

    auto it = blocks.begin();
    while (it != blocks.end()) {
      allocator->deallocate(*it);
      it = blocks.erase(it);
    }

    CPPUNIT_ASSERT(allocator->free_size() == _maxSize);
    CPPUNIT_ASSERT(allocator->allocate(_maxSize) != nullptr);
  }

private:
  static const size_t _minSize = 16;
  static const size_t _maxSize = 256;

  static BTBuddyAllocator<SmallSingleConfig> *
  get_small_filled_allocator(void *mempool) {
    return BTBuddyAllocator<SmallSingleConfig>::create(nullptr, mempool, 0,
                                                           true);
  }
};

class SmallSingleLazyAllocatorTests : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SmallSingleLazyAllocatorTests);
  CPPUNIT_TEST(testDeallocateToLazy);
  CPPUNIT_TEST(testEmptyLazy);
  CPPUNIT_TEST(testAllocateFromLazy);
  CPPUNIT_TEST(testAllCombined);
  CPPUNIT_TEST_SUITE_END();

public:
  void testDeallocateToLazy() {
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        smallLazyAllocator == nullptr ? get_small_lazy_allocator()
                                      : smallLazyAllocator;
    std::vector<void *> blocks;

    for (size_t i = 0; i < _maxSize; i += _minSize) {
      void *p = allocator->allocate(_minSize);
      CPPUNIT_ASSERT(p != nullptr);
      blocks.push_back(p);
    }

    auto it = blocks.begin();
    while (it != blocks.end()) {
      allocator->deallocate(*it);
      it = blocks.erase(it);
    }

    CPPUNIT_ASSERT(allocator->free_size() == _maxSize);
    CPPUNIT_ASSERT(allocator->allocate(_minSize * 2) == nullptr);
  }

  void testEmptyLazy() {
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        smallLazyAllocator == nullptr ? get_small_lazy_allocator()
                                      : smallLazyAllocator;
    std::vector<void *> blocks;

    for (size_t i = 0; i < _maxSize; i += _minSize) {
      void *p = allocator->allocate(_minSize);
      CPPUNIT_ASSERT(p != nullptr);
      blocks.push_back(p);
    }

    auto it = blocks.begin();
    while (it != blocks.end()) {
      allocator->deallocate(*it);
      it = blocks.erase(it);
    }

    CPPUNIT_ASSERT(allocator->free_size() == _maxSize);
    CPPUNIT_ASSERT(allocator->allocate(_minSize * 2) == nullptr);

    allocator->empty_lazy_list();

    CPPUNIT_ASSERT(allocator->free_size() == _maxSize);

    void *p2 = allocator->allocate(_maxSize);
    CPPUNIT_ASSERT(p2 != nullptr);
    allocator->deallocate(p2);
  }

  void testAllocateFromLazy() {
    BTBuddyAllocator<SmallSingleConfig> *allocator =
        smallLazyAllocator == nullptr ? get_small_lazy_allocator()
                                      : smallLazyAllocator;
    std::vector<void *> blocks;

    for (size_t i = 0; i < _maxSize; i += _minSize) {
      void *p = allocator->allocate(_minSize);
      CPPUNIT_ASSERT(p != nullptr);
      blocks.push_back(p);
    }

    auto it = blocks.begin();
    while (it != blocks.end()) {
      allocator->deallocate(*it);
      it = blocks.erase(it);
    }

    CPPUNIT_ASSERT(allocator->free_size() == _maxSize);
    CPPUNIT_ASSERT(allocator->allocate(_minSize * 2) == nullptr);

    for (size_t i = 0; i < _maxSize; i += _minSize) {
      void *p = allocator->allocate(_minSize);
      CPPUNIT_ASSERT(p != nullptr);
      blocks.push_back(p);
    }

    CPPUNIT_ASSERT(allocator->free_size() == 0);
    CPPUNIT_ASSERT(allocator->allocate(_minSize) == nullptr);

    auto it2 = blocks.begin();
    while (it2 != blocks.end()) {
      allocator->deallocate(*it2);
      it2 = blocks.erase(it2);
    }
  }

  void testAllCombined() {
    smallLazyAllocator = get_small_lazy_allocator();

    testDeallocateToLazy();
    smallLazyAllocator->empty_lazy_list();
    testEmptyLazy();
    smallLazyAllocator->empty_lazy_list();
    testAllocateFromLazy();
    smallLazyAllocator->empty_lazy_list();

    CPPUNIT_ASSERT(smallLazyAllocator->free_size() == _maxSize);
    void *p = smallLazyAllocator->allocate(_maxSize);
    CPPUNIT_ASSERT(p != nullptr);
    smallLazyAllocator->deallocate(p);
  }

private:
  static const size_t _minSize = 16;
  static const size_t _maxSize = 256;

  BTBuddyAllocator<SmallSingleConfig> *smallLazyAllocator = nullptr;

  static BTBuddyAllocator<SmallSingleConfig> *get_small_lazy_allocator() {
    return BTBuddyAllocator<SmallSingleConfig>::create(nullptr, nullptr, 16,
                                                           false);
  }
};

class LargeQuadAllocatorTests : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(LargeQuadAllocatorTests);
  CPPUNIT_TEST(testAllocateWholeLarge);
  CPPUNIT_TEST(testAllocateFillBlocksLarge);
  CPPUNIT_TEST(testAllocateFillLargeBlocksQuad);
  CPPUNIT_TEST(testAllocateAllSizesQuad);
  CPPUNIT_TEST(testAllocateFillAllSizesQuad);
  CPPUNIT_TEST(testAllCombined);
  CPPUNIT_TEST_SUITE_END();

public:
  void testAllocateWholeLarge() {
    BTBuddyAllocator<LargeQuadConfig> *allocator =
        largeQuadAllocator == nullptr ? get_large_quad_allocator()
                                      : largeQuadAllocator;

    void *p = allocator->allocate(_maxSize);
    CPPUNIT_ASSERT(p != nullptr);
    allocator->deallocate(p);
  }

  void testAllocateFillBlocksLarge() {
    BTBuddyAllocator<LargeQuadConfig> *allocator =
        largeQuadAllocator == nullptr ? get_large_quad_allocator()
                                      : largeQuadAllocator;
    std::vector<void *> blocks;
    const int size = _maxSize * 4 / _minSize;

    for (int i = 0; i < size; i++) {
      void *p = allocator->allocate(_minSize);
      CPPUNIT_ASSERT(p != nullptr);
      blocks.push_back(p);
    }

    CPPUNIT_ASSERT(allocator->allocate(_minSize) == nullptr);
    CPPUNIT_ASSERT(allocator->free_size() == 0);

    for (int i = 0; i < size; i++) {
      allocator->deallocate(blocks[i]);
    }

    CPPUNIT_ASSERT(allocator->free_size() == _maxSize * 4);

    void *p2 = allocator->allocate(_maxSize);
    void *p3 = allocator->allocate(_maxSize);
    void *p4 = allocator->allocate(_maxSize);
    void *p5 = allocator->allocate(_maxSize);

    CPPUNIT_ASSERT(p2 != nullptr);
    CPPUNIT_ASSERT(p3 != nullptr);
    CPPUNIT_ASSERT(p4 != nullptr);
    CPPUNIT_ASSERT(p5 != nullptr);

    allocator->deallocate(p2);
    allocator->deallocate(p3);
    allocator->deallocate(p4);
    allocator->deallocate(p5);
  }

  void testAllocateFillLargeBlocksQuad() {
    BTBuddyAllocator<LargeQuadConfig> *allocator =
        largeQuadAllocator == nullptr ? get_large_quad_allocator()
                                      : largeQuadAllocator;
    std::vector<void *> blocks;
    const int size = _maxSize * 4 / (_minSize * 4);

    for (int i = 0; i < size; i++) {
      void *p = allocator->allocate(_minSize * 4);
      CPPUNIT_ASSERT(p != nullptr);
      blocks.push_back(p);
    }

    CPPUNIT_ASSERT(allocator->allocate(_minSize) == nullptr);
    CPPUNIT_ASSERT(allocator->free_size() == 0);

    for (int i = 0; i < size; i++) {
      allocator->deallocate(blocks[i]);
    }

    CPPUNIT_ASSERT(allocator->free_size() == _maxSize * 4);

    void *p2 = allocator->allocate(_maxSize);
    void *p3 = allocator->allocate(_maxSize);
    void *p4 = allocator->allocate(_maxSize);
    void *p5 = allocator->allocate(_maxSize);

    CPPUNIT_ASSERT(p2 != nullptr);
    CPPUNIT_ASSERT(p3 != nullptr);
    CPPUNIT_ASSERT(p4 != nullptr);
    CPPUNIT_ASSERT(p5 != nullptr);

    allocator->deallocate(p2);
    allocator->deallocate(p3);
    allocator->deallocate(p4);
    allocator->deallocate(p5);
  }

  void testAllocateAllSizesQuad() {
    BTBuddyAllocator<LargeQuadConfig> *allocator =
        largeQuadAllocator == nullptr ? get_large_quad_allocator()
                                      : largeQuadAllocator;
    std::vector<void *> blocks;
    const int size = _maxSize / 2;

    for (int i = 0; i < 4; i++) {
      for (size_t j = size; j >= _minSize; j /= 2) {
        void *p = allocator->allocate(j);
        CPPUNIT_ASSERT(p != nullptr);
        blocks.push_back(p);
      }
    }

    CPPUNIT_ASSERT(allocator->free_size() == (_minSize * 4));

    void *p2 = allocator->allocate(_minSize);
    void *p3 = allocator->allocate(_minSize);
    void *p4 = allocator->allocate(_minSize);
    void *p5 = allocator->allocate(_minSize);

    CPPUNIT_ASSERT(allocator->allocate(_minSize) == nullptr);
    CPPUNIT_ASSERT(p2 != nullptr);
    CPPUNIT_ASSERT(p3 != nullptr);
    CPPUNIT_ASSERT(p4 != nullptr);
    CPPUNIT_ASSERT(p5 != nullptr);

    allocator->deallocate(p2);
    allocator->deallocate(p3);
    allocator->deallocate(p4);
    allocator->deallocate(p5);

    auto it = blocks.begin();
    while (it != blocks.end()) {
      allocator->deallocate(*it);
      it = blocks.erase(it);
    }
  }

  void testAllocateFillAllSizesQuad() {
    BTBuddyAllocator<LargeQuadConfig> *allocator =
        largeQuadAllocator == nullptr ? get_large_quad_allocator()
                                      : largeQuadAllocator;
    std::vector<void *> blocks;

    for (size_t i = _maxSize; i >= _minSize; i /= 2) {
      CPPUNIT_ASSERT(allocator->free_size() == _maxSize * 4);

      for (unsigned int j = 0; j < _maxSize * 4 / i; j++) {
        void *p = allocator->allocate(i);
        CPPUNIT_ASSERT(p != nullptr);
        blocks.push_back(p);
      }

      CPPUNIT_ASSERT(allocator->free_size() == 0);
      CPPUNIT_ASSERT(allocator->allocate(i) == nullptr);

      for (unsigned int j = 0; j < _maxSize * 4 / i; j++) {
        allocator->deallocate(blocks.back());
        blocks.pop_back();
      }

      CPPUNIT_ASSERT(allocator->free_size() == _maxSize * 4);
    }

    void *p2 = allocator->allocate(_maxSize);
    void *p3 = allocator->allocate(_maxSize);
    void *p4 = allocator->allocate(_maxSize);
    void *p5 = allocator->allocate(_maxSize);

    CPPUNIT_ASSERT(p2 != nullptr);
    CPPUNIT_ASSERT(p3 != nullptr);
    CPPUNIT_ASSERT(p4 != nullptr);
    CPPUNIT_ASSERT(p5 != nullptr);

    allocator->deallocate(p2);
    allocator->deallocate(p3);
    allocator->deallocate(p4);
    allocator->deallocate(p5);
  }

  void testAllCombined() {
    largeQuadAllocator = get_large_quad_allocator();

    testAllocateWholeLarge();
    testAllocateFillBlocksLarge();
    testAllocateFillLargeBlocksQuad();
    testAllocateAllSizesQuad();
    testAllocateFillAllSizesQuad();

    CPPUNIT_ASSERT(largeQuadAllocator->free_size() == _maxSize * 4);

    void *p = largeQuadAllocator->allocate(_maxSize);
    void *p2 = largeQuadAllocator->allocate(_maxSize);
    void *p3 = largeQuadAllocator->allocate(_maxSize);
    void *p4 = largeQuadAllocator->allocate(_maxSize);

    CPPUNIT_ASSERT(p != nullptr);
    CPPUNIT_ASSERT(p2 != nullptr);
    CPPUNIT_ASSERT(p3 != nullptr);
    CPPUNIT_ASSERT(p4 != nullptr);

    largeQuadAllocator->deallocate(p);
    largeQuadAllocator->deallocate(p2);
    largeQuadAllocator->deallocate(p3);
    largeQuadAllocator->deallocate(p4);
  }

private:
  static const size_t _minSize = 16;
  static const size_t _maxSize = (1U << 21U);

  BTBuddyAllocator<LargeQuadConfig> *largeQuadAllocator = nullptr;

  static BTBuddyAllocator<LargeQuadConfig> *get_large_quad_allocator() {
    return BTBuddyAllocator<LargeQuadConfig>::create(nullptr, nullptr, 0,
                                                         false);
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(SmallSingleAllocatorTests);
CPPUNIT_TEST_SUITE_REGISTRATION(SmallDoubleAllocatorTests);
CPPUNIT_TEST_SUITE_REGISTRATION(SmallSingleFilledAllocatorTests);
CPPUNIT_TEST_SUITE_REGISTRATION(SmallSingleLazyAllocatorTests);
CPPUNIT_TEST_SUITE_REGISTRATION(LargeQuadAllocatorTests);
int main() {
  // Run the tests
  CppUnit::TextTestRunner runner;
  CppUnit::TestFactoryRegistry &registry =
      CppUnit::TestFactoryRegistry::getRegistry();
  runner.addTest(registry.makeTest());
  runner.run();

  return 0;
}
