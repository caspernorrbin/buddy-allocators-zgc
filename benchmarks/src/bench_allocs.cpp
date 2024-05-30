#include "../../include/buddy_allocator.hpp"
#include "../../include/buddy_config.hpp"
#include "../../include/buddy_helper.hpp"
#include "../../include/buddy_instantiations.hpp"

#include "../../include/bbuddy.hpp"
#include "../../include/bbuddy_instantiations.hpp"

#include "../../include/btbuddy.hpp"
#include "../../include/btbuddy_instantiations.hpp"

#include "../../include/ibuddy.hpp"
#include "../../include/ibuddy_instantiations.hpp"

#include <time.h>

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sys/mman.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <N>" << std::endl;
    return 1;
  }

  const int N = std::atoi(argv[1]);
  const int sizes[] = {16,   32,   64,    128,   256,   512,    1024,  2048,
                       4096, 8192, 16384, 32768, 65536, 131072, 262144};

  // mmap 2MB
  void *mem = mmap(nullptr, 2 * 1024 * 1024, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  void *allocator_mem =
      mmap(nullptr, sizeof(BTBuddyAllocator<ZConfig>), PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mem == MAP_FAILED) {
    std::cerr << "Failed to mmap memory" << std::endl;
    return 1;
  }

  for (const auto &size : sizes) {
    for (int i = 0; i < N; i++) {
      // Create buddy instance
      BuddyAllocator<ZConfig> *btbuddy =
          // BTBuddyAllocator<ZConfig>::create(allocator_mem, mem, 0, false);
      BinaryBuddyAllocator<ZConfig>::create(allocator_mem, mem, 10, false);
      // IBuddyAllocator<ZConfig>::create(allocator_mem, mem, 0, false);

      // Place allocation in the lazy layer
      // void *ptr1 = btbuddy->allocate(size);
      // std::cout << "Allocated address: " << ptr1 << std::endl;
      // btbuddy->deallocate(ptr1, size);
      void *ptr = btbuddy->allocate(size);
      if (ptr != mem) {
        std::cerr << "Error: " << ptr << " != " << mem << std::endl;
        return 1;
      }

      // Start the timer
      timespec start, end;
      clock_gettime(CLOCK_MONOTONIC_RAW, &start);

      // Allocate once
      asm volatile("" : : "r,m"(ptr) : "memory");
      btbuddy->deallocate(ptr, size);

      // Stop the timer
      clock_gettime(CLOCK_MONOTONIC_RAW, &end);

      // Calculate the elapsed time in nanoseconds
      const double elapsedTime = end.tv_nsec - start.tv_nsec;

      std::cout << elapsedTime << " " << size << std::endl;
    }
  }

  return 0;
}
