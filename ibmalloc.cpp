#include "ibuddy.hpp"

#include <cstdlib>
#include <cstring>
#include <unistd.h>

using Config = IBuddyConfig<4, 25>;

// uint8_t mempool[1 << MAX_SIZE_LOG2];
// uint8_t allocatorpool[sizeof(IBuddyAllocator<Config>)];
static IBuddyAllocator<Config> *allocator = nullptr;

extern "C" {
void init_buddy() {
  // allocator = reinterpret_cast<BinaryBuddyAllocator *>(allocatorpool);
  allocator =
      IBuddyAllocator<Config>::create(nullptr, nullptr, 1 << 25, 31);
      // IBuddyAllocator<Config>::create(allocatorpool, mempool, 1 << MAX_SIZE_LOG2, MIN_SIZE_LOG2, MAX_SIZE_LOG2);
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