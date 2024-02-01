#include "ibuddy.hpp"

#include <cstdlib>
#include <cstring>
#include <unistd.h>

uint8_t mempool[1 << MAX_SIZE_LOG2];
uint8_t allocatorpool[sizeof(IBuddyAllocator)];
static IBuddyAllocator *allocator = nullptr;

extern "C" {
void init_buddy() {
  if (sizeof(IBuddyAllocator) > sizeof(allocatorpool)) {
    // Handle the case where the allocator pool is too small
    // You may want to log an error message or terminate the program
    abort();
  }

  // allocator = reinterpret_cast<BinaryBuddyAllocator *>(allocatorpool);
  allocator =
      IBuddyAllocator::create(allocatorpool, mempool, 1 << MAX_SIZE_LOG2, MIN_SIZE_LOG2, MAX_SIZE_LOG2);
  // reinterpret_cast<BinaryBuddyAllocator*>(sbrk(sizeof(BinaryBuddyAllocator)));
  // *(allocator) = IBuddyAllocator(mempool, 1 << 21, 4, 21);
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

void *realloc(void *ptr, size_t size) {
  if (allocator == nullptr) {
    init_buddy();
  }

  if (ptr == nullptr) {
    return malloc(size);
  }

  void *p = allocator->allocate(size);
  if (p == nullptr) {
    return nullptr;
  }

  size_t old_size = allocator->get_alloc_size(ptr);
  memcpy(p, ptr, old_size);
  allocator->deallocate(ptr, old_size);
  return p;
}

void free(void *p) { allocator->deallocate(p); }
}