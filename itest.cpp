#include "ibuddy.hpp"
#include <cstdlib>
#include <iostream>
#include <vector>

int main() {
  // Create a binary buddy allocator with a total size of 2 MB
  int maxSize = MAX_SIZE_LOG2;
  size_t totalSize = 1 << maxSize; // 2^21 = 2MB
  void *start = malloc(totalSize);
  
  IBuddyAllocator *ba =
      static_cast<IBuddyAllocator *>(malloc(sizeof(IBuddyAllocator)));
  std::cout << "Size of IBuddyAllocator: " << sizeof(IBuddyAllocator)
            << " start: " << start << " totalSize: " << totalSize
            << " maxSize: " << maxSize << std::endl;
  // IBuddyAllocator allocator(start, totalSize, 4, maxSize);
  IBuddyAllocator *allocator =
      IBuddyAllocator::create(nullptr, nullptr, totalSize, MIN_SIZE_LOG2, maxSize);

  // allocator->print_free_list();
  // Allocate memory blocks of different sizes
  //   void *block3 = allocator.allocate(72704);
  void *block1 = allocator->allocate(16);
  // allocator->print_free_list();
  // allocator->print_free_list();
  void *block2 = allocator->allocate(63);
  allocator->deallocate(block2);
  // allocator->print_free_list();
  void *block3 = allocator->allocate(62);
  allocator->deallocate(block3);
  allocator->deallocate(block1);

  // allocator->print_free_list();

  //   // Print the addresses of the allocated blocks
  //   std::cout << "Block 1: " << block1 << std::endl;
  //   std::cout << "Block 2: " << block2 << std::endl;
  // std::cout << "Block 3: " << block3 << std::endl;

  //   // Deallocate the memory blocks
  //   allocator.print_free_list();
  //   allocator.print_free_list();

  // allocator->print_free_list();

    void *block4 = allocator->allocate(16);
    allocator->deallocate(block4);
    void *block5 = allocator->allocate(16);
    void *block6 = allocator->allocate(16);
    allocator->deallocate(block5);
    allocator->deallocate(block6);

  //   std::cout << "Block 4: " << block4 << std::endl;
  //   std::cout << "Block 5: " << block5 << std::endl;
  //   std::cout << "Block 6: " << block6 << std::endl;


    // allocator->print_free_list();

  // allocator.print_bitmaps();

  std::vector<void *> blocks;
  int size = (1 << (NUM_LEVELS - 1)) + 50;

  int totals = 0;
  for (int i = 0; i < size; i++) {
    void *p = allocator->allocate(13);
    // std::cout << "Block " << i << ": " << p << std::endl;
    blocks.push_back(p);
    if (p != nullptr) {
      totals++;
    }
  }

  std::cout << "Total blocks allocated: " << totals << std::endl;

  allocator->print_free_list();
  // allocator->print_bitmaps();

  int stride = 3;

  for (int i = 0; i < stride; ++i) {
    for (int j = i; j < size; j += stride) {
      allocator->deallocate(blocks[j]);
    }
  }

  // allocator.print_free_list();

  free(start);
  free(ba);
  return 0;
}
