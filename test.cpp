#include "binary_buddy.hpp"
#include <cstdlib>
#include <iostream>
#include <vector>

int main() {
  // Create a binary buddy allocator with a total size of 2 MB
  int maxSize = 21;
  size_t totalSize = 1 << maxSize; // 2^21 = 2MB
                                   //   size_t totalSize = 1024; // 2^10
  void *start = malloc(totalSize);
  void *ba = malloc(sizeof(BinaryBuddyAllocator));
  std::cout << "Size of BinaryBuddyAllocator: " << sizeof(BinaryBuddyAllocator)
            << std::endl;
  BinaryBuddyAllocator allocator(start, totalSize, 4, maxSize);
  //   BinaryBuddyAllocator allocator = BinaryBuddyAllocator::create(ba,start,
  //   totalSize, 4, 21);

  // Allocate memory blocks of different sizes
  //   void *block3 = allocator.allocate(72704);
  //   void *block3 = allocator.allocate(3);
  //   allocator.deallocate(block3);
  //   void *block1 = allocator.allocate(128);
  //   void *block2 = allocator.allocate(256);

  //   allocator.print_free_list();

  //   // Print the addresses of the allocated blocks
  //   std::cout << "Block 1: " << block1 << std::endl;
  //   std::cout << "Block 2: " << block2 << std::endl;
  //   std::cout << "Block 3: " << block3 << std::endl;

  //   // Deallocate the memory blocks
  //   allocator.deallocate(block1);
  //   allocator.print_free_list();
  //   allocator.deallocate(block2);
  //   allocator.print_free_list();

  // allocator.print_free_list();

  //   void *block6 = allocator.allocate(62);
  //   void *block4 = allocator.allocate(16);
  //   void *block5 = allocator.allocate(16);

  //   std::cout << "Block 4: " << block4 << std::endl;
  //   std::cout << "Block 5: " << block5 << std::endl;
  //   std::cout << "Block 6: " << block6 << std::endl;

  //   allocator.deallocate(block4);
  //   allocator.deallocate(block5);
  //   allocator.deallocate(block6);

  //   allocator.print_free_list();

  // allocator.print_bitmaps();

  std::vector<void *> blocks;

  for (int i = 0; i < (1 << 17); i++) {
    void *p = allocator.allocate(13);
    // std::cout << "Block " << i << ": " << p << std::endl;
    blocks.push_back(p);
  }

  allocator.print_free_list();

  int stride = 3;
  int size = 1 << 17;

  for (int i = 0; i < stride; ++i) {
    for (int j = i; j < size; j += stride) {
      allocator.deallocate(blocks[j]);
    }
  }

  // Deallocate blocks
  //   for (int i = 0; i < (1 << 17); i += 2) {
  //     allocator.deallocate(blocks[i]);
  //   }

  //   for (int i = 1; i < (1 << 17); i += 2) {
  //     allocator.deallocate(blocks[i]);
  //   }

  // allocator.print_free_list();

  // Deallocate the remaining blocks
  //   for (int i = 1; i < (2 << 12) - 1; i += 2) {
  // allocator.deallocate(blocks[i]);
  //   }
  //
  allocator.print_free_list();

  free(start);
  return 0;
}
