#include "ibuddy.hpp"

#include <cstdint>
#include <cstdlib>
#include <cstring>

using Config = IBuddyConfig<4, 27, 1, 0>;

// uint8_t mempool[1 << MAX_SIZE_LOG2];
// uint8_t allocatorpool[sizeof(IBuddyAllocator<Config>)];
static IBuddyAllocator<Config> *allocator = nullptr;

extern "C" {
void init_buddy() {
  allocator = IBuddyAllocator<Config>::create(nullptr, nullptr, 31, false);
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

  void *p = allocator->allocate(size);
  if (p == nullptr) {
    return nullptr;
  }

  if (ptr == nullptr) {
    return p;
  }

  const size_t old_size =
      allocator->get_alloc_size(reinterpret_cast<uintptr_t>(ptr));
  memcpy(p, ptr, old_size);
  allocator->deallocate(ptr, old_size);
  return p;
}

void free(void *p) { allocator->deallocate(p); }
}