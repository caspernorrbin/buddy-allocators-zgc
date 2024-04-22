#include "buddy_allocator.hpp"
#include "buddy_config.hpp"
#include "buddy_helper.hpp"
#include "buddy_instantiations.hpp"

#include "bbuddy.hpp"
#include "bbuddy_instantiations.hpp"

#include "btbuddy.hpp"
#include "btbuddy_instantiations.hpp"

#include "ibuddy.hpp"
#include "ibuddy_instantiations.hpp"

#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

const int n_cycles = 1;
const int total_runs = 1;
const int n_threads = 1;
const int runs_per_thread = total_runs / n_threads;
const size_t max_allocs = -1;
const bool should_free = false;

const bool const_allocs = true;
// const size_t total_alloc_size = 1073741824; // 1GiB
// const size_t total_alloc_size = 67108864; // 64MiB
const size_t total_alloc_size = ; // 2MiB
const size_t const_alloc_size = 16;
const int allocs_per_thread = total_alloc_size / const_alloc_size / n_threads;

const std::string filename = "runs/dh2.txt";
// const std::string filename = "runs/ls.txt";

std::array<std::vector<void *>, n_threads> allocations;

void allocate_run(BuddyAllocator<MallocConfig> *allocator,
                  std::vector<size_t> &allocation_sizes, int i) {
  size_t num_allocations = 0;
  for (int j = 0; j < runs_per_thread; j++) {
    if (const_allocs) {
      for (int k = 0; k < allocs_per_thread; k++) {
        void *p = allocator->allocate(const_alloc_size);
        assert(p != nullptr);
        if (should_free) {
          allocations[i].push_back(p);
        }
      }

    } else {
      for (const auto &size : allocation_sizes) {
        if (num_allocations++ >= max_allocs) {
          break;
        }

        void *p = allocator->allocate(size);
        assert(p != nullptr);

        if (should_free) {
          allocations[i].push_back(p);
        }
      }
    }
  }
}

void process_file(const std::string &filename,
                  std::vector<size_t> &allocation_sizes) {
  std::ifstream file(filename);

  if (!file.is_open()) {
    std::cerr << "Failed to open the file: " << filename << std::endl;
    exit(1);
  }

  std::string line;
  while (std::getline(file, line)) {
    try {
      const int number = std::stoi(line);
      allocation_sizes.push_back(number);
    } catch (const std::invalid_argument &ia) {
      std::cerr << "Invalid argument: " << ia.what() << std::endl;
    } catch (const std::out_of_range &oor) {
      std::cerr << "Out of range: " << oor.what() << std::endl;
    }
  }

  file.close();
}

int main() {
  //   size_t pool_size = 2000 * 1024;
  //   uint8_t *pool = mmap_allocate(pool_size);
  //   ZPageOptimizedTLSF zalloc(pool, pool_size, size_mapping, false);

  BuddyAllocator<MallocConfig> *allocator =
      // IBuddyAllocator<MallocConfig>::create(nullptr, nullptr, 0, false);
      // BinaryBuddyAllocator<MallocConfig>::create(nullptr, nullptr, 0, false);
      BTBuddyAllocator<MallocConfig>::create(nullptr, nullptr, 0, false);
  // allocator->print_free_list();

  std::vector<std::thread> threads(n_threads);

  std::vector<size_t> allocation_sizes;

  process_file(filename, allocation_sizes);

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < n_cycles; i++) {

    for (int j = 0; j < n_threads; j++) {
      threads[j] =
          std::thread(allocate_run, allocator, std::ref(allocation_sizes), j);
    }

    for (auto &t : threads) {
      t.join();
    }

    if (should_free) {
      for (auto &vec : allocations) {
        for (const auto &p : vec) {
          allocator->deallocate(p, const_alloc_size);
        }
        vec.clear();
      }
    }
  }

  // assert(allocator->allocate(1) == nullptr);

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = end_time - start_time;
  const double seconds = std::chrono::duration<double>(duration).count();
  std::cout << "Concurrent took: " << seconds << " seconds" << std::endl;
  // allocator->empty_lazy_list();
  // allocator->print_free_list();
}