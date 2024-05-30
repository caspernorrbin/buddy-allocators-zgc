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

  const int page_size = 2097152;

  for (const auto &size : sizes) {
    const int num_allocs = page_size / size;
    for (int i = 0; i < N; i++) {
      // Create buddy instance
      BuddyAllocator<ZConfig> *btbuddy =
          //   BTBuddyAllocator<ZConfig>::create(allocator_mem, mem, 10, false);
        //   BinaryBuddyAllocator<ZConfig>::create(allocator_mem, mem, 10, false);
      IBuddyAllocator<ZConfig>::create(allocator_mem, mem, 10, false);

      // Start the timer
      timespec start, end;
      clock_gettime(CLOCK_MONOTONIC_RAW, &start);

      // Fill the page with allocations
      for (int j = 0; j < num_allocs; j++) {
        void *ptr = btbuddy->allocate(size);
        asm volatile("" : : "r,m"(ptr) : "memory");
        if (ptr == nullptr) {
          std::cerr << "Error: " << ptr << " is null" << std::endl;
          return 1;
        }
      }

      // Stop the timer
      clock_gettime(CLOCK_MONOTONIC_RAW, &end);

      //   const double elapsedTime = end.tv_nsec - start.tv_nsec;
      // Calculate the elapsed time in microseconds
      const double elapsedTime = (end.tv_sec - start.tv_sec) * 1000000.0 +
                                 (end.tv_nsec - start.tv_nsec) / 1000.0;

      std::cout << elapsedTime << " " << size << std::endl;
    }
  }

  return 0;
}
