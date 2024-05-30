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
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <N> <size>" << std::endl;
    return 1;
  }

  int N = std::atoi(argv[1]);
  int size = std::atoi(argv[2]);

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

  double totalTime = 0.0;

  for (int i = 0; i < N; i++) {
    // Create btbuddy instance
    BuddyAllocator<ZConfig> *btbuddy =
        BTBuddyAllocator<ZConfig>::create(allocator_mem, mem, 10, false);
        // BinaryBuddyAllocator<ZConfig>::create(allocator_mem, mem, 10, false);

    // Place allocation in the lazy layer
    void *ptr1 = btbuddy->allocate(size);
    // std::cout << "Allocated address: " << ptr1 << std::endl;
    btbuddy->deallocate(ptr1, size);

    // Start the timer
    timespec start, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    // Allocate once
    void *ptr = btbuddy->allocate(size);
    asm volatile("" : : "r,m"(ptr) : "memory");

    // Stop the timer
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    if (ptr != mem) {
      std::cerr << "Error: " << ptr << " != " << mem << std::endl;
      return 1;
    }

    // Calculate the elapsed time in nanoseconds
    const double elapsedTime = end.tv_nsec - start.tv_nsec;

    // Print the allocated address and elapsed time
    // std::cout << "Allocated address: " << ptr << ", Elapsed time: " <<
    // elapsedTime << " ms" << std::endl;

    totalTime += elapsedTime;
  }

  // Calculate the average time
  double averageTime = totalTime / N;

  // Print the average time
  std::cout << "Average time: " << averageTime << " ns" << std::endl;

  return 0;
}
