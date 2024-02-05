#include "ibuddy.hpp"
#include <cstdlib>
#include <iostream>
#include <vector>

int main() {
  const int minSizeLog2 = 4;
  const int maxSizeLog2 = 23;
  const int lazyThreshold = 31;
  size_t totalSize = 1 << maxSizeLog2;

  using Config = IBuddyConfig<minSizeLog2, maxSizeLog2>;

  IBuddyAllocator<Config> *allocator = IBuddyAllocator<Config>::create(
      nullptr, nullptr, totalSize, lazyThreshold);

  // allocator->print_free_list();

  void *block1 = allocator->allocate(16);
  void *block2 = allocator->allocate(17);
  allocator->deallocate(block2);
  void *block3 = allocator->allocate(62);
  allocator->deallocate(block3);
  allocator->deallocate(block1);

  // allocator->print_free_list();

  // Print the addresses of the allocated blocks
  // std::cout << "Block 1: " << block1 << std::endl;
  // std::cout << "Block 2: " << block2 << std::endl;
  // std::cout << "Block 3: " << block3 << std::endl;

  // allocator->print_free_list();

  //   // Deallocate the memory blocks
  void *block4 = allocator->allocate(17);
  allocator->deallocate(block4);
  void *block5 = allocator->allocate(17);
  void *block6 = allocator->allocate(16);
  allocator->deallocate(block5);
  allocator->deallocate(block6);

  //   std::cout << "Block 4: " << block4 << std::endl;
  //   std::cout << "Block 5: " << block5 << std::endl;
  //   std::cout << "Block 6: " << block6 << std::endl;

  // allocator->empty_lazy_list();
  // allocator->print_free_list();
  // allocator->print_bitmaps();

  // allocator.print_bitmaps();

  std::vector<void *> blocks;
  int size = (1 << (maxSizeLog2 - minSizeLog2)) + 10;

  int totals = 0;
  for (int i = 0; i < size; i++) {
    void *p = allocator->allocate(13);
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

  // allocator->print_free_list();
  // allocator->empty_lazy_list();
  // allocator->print_free_list();

  return 0;
}
