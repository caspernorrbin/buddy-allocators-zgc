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
    std::cerr << "Usage: " << argv[0] << " <size>" << std::endl;
    return 1;
  }

  int size = std::atoi(argv[1]);

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

  // Create btbuddy instance
  BuddyAllocator<ZConfig> *btbuddy =
      BTBuddyAllocator<ZConfig>::create(allocator_mem, mem, 10, false);
      // BinaryBuddyAllocator<ZConfig>::create(allocator_mem, mem, 10, false);
      // IBuddyAllocator<ZConfig>::create(allocator_mem, mem, 10, false);

  // Place allocation in the lazy layer
  void *ptr0 = btbuddy->allocate(size);
  btbuddy->deallocate(ptr0);

  // Start the timer
  timespec start, end;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);

  // Allocate once
  void *ptr = btbuddy->allocate(size);
  asm volatile("" : : "r,m"(ptr) : "memory");

  // Stop the timer
  clock_gettime(CLOCK_MONOTONIC_RAW, &end);

  // Calculate the elapsed time in nanoseconds
  const double elapsedTime = end.tv_nsec - start.tv_nsec;

  // Print the average time
  std::cout << elapsedTime << std::endl;

  return 0;
}
