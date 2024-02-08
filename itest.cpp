#include "ibuddy.hpp"
#include <iostream>
#include <vector>

int main() {
  const int minSizeLog2 = 4;
  const int maxSizeLog2 = 8;
  const int numRegions = 1;
  const int sizeBits = 4;
  const int lazyThreshold = 31;
  const bool startFull = false;

  using Config = IBuddyConfig<minSizeLog2, maxSizeLog2, numRegions, sizeBits>;

  IBuddyAllocator<Config> *allocator = IBuddyAllocator<Config>::create(
      nullptr, nullptr, lazyThreshold, startFull);

  allocator->print_free_list();
  allocator->print_bitmaps();

  void *block1 = allocator->allocate(17);
  void *block2 = allocator->allocate(17);
  allocator->deallocate(block2);
  void *block3 = allocator->allocate(62);
  allocator->deallocate(block3);
  allocator->deallocate(block1);

  allocator->print_free_list();
  allocator->print_bitmaps();

  // Print the addresses of the allocated blocks
  // std::cout << "Block 1: " << block1 << std::endl;
  // std::cout << "Block 2: " << block2 << std::endl;
  // std::cout << "Block 3: " << block3 << std::endl;

  // allocator->print_free_list();

  // Deallocate the memory blocks
  void *block4 = allocator->allocate(17);
  void *block5 = allocator->allocate(17);
  void *block6 = allocator->allocate(16);
  allocator->deallocate(block4);
  allocator->deallocate(block5);
  allocator->deallocate(block6);

  //   std::cout << "Block 4: " << block4 << std::endl;
  //   std::cout << "Block 5: " << block5 << std::endl;
  //   std::cout << "Block 6: " << block6 << std::endl;

  // allocator->empty_lazy_list();
  allocator->print_free_list();
  allocator->print_bitmaps();

  std::vector<void *> blocks;
  const int size = (1U << static_cast<unsigned int>(maxSizeLog2 - minSizeLog2)) + 10;

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
  allocator->print_bitmaps();

  const int stride = 3;

  for (int i = 0; i < stride; ++i) {
    for (int j = i; j < size; j += stride) {
      allocator->deallocate(blocks[j]);
    }
  }

  allocator->print_free_list();
  allocator->empty_lazy_list();
  allocator->print_free_list();

  return 0;
}
