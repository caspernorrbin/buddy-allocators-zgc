#include "binary_buddy.hpp"

#include <cstdlib>
#include <cstring>
#include <unistd.h>

uint8_t mempool[1 << 21];
uint8_t allocatorpool[1 << 21];
static BinaryBuddyAllocator *allocator = nullptr;

extern "C" {
void init_buddy() {
  if (sizeof(BinaryBuddyAllocator) > sizeof(allocatorpool)) {
    // Handle the case where the allocator pool is too small
    // You may want to log an error message or terminate the program
    abort();
  }

  // allocator = reinterpret_cast<BinaryBuddyAllocator *>(allocatorpool);
  allocator =
      BinaryBuddyAllocator::create(allocatorpool, mempool, 1 << 21, 4, 21);
  // reinterpret_cast<BinaryBuddyAllocator*>(sbrk(sizeof(BinaryBuddyAllocator)));
  //*(allocator) = BinaryBuddyAllocator(mempool, 1 << 21, 4, 21);
}

void *malloc(size_t size) {
  if (allocator == nullptr) {
    init_buddy();
  }

  return allocator->allocate(size);
}

void *calloc(size_t num, size_t size) {
  if (allocator == nullptr) {
    init_buddy();
  }

  void *p = allocator->allocate(num * size);
  memset(p, 0, num * size);
  return p;
}

void free(void *p) { allocator->deallocate(p); }
}