
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

#include <sys/mman.h>

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

struct Operation {
  char type;
  std::string id;
  size_t size;
};

struct Allocation {
  void *addr;
  size_t size;
};

static const size_t BLOCK_HEADER_SIZE = 0;

static const size_t PAGE_SIZE = 2097152;
// static const size_t PAGE_SIZE = 10 * 1000 * 1024;

std::map<std::string, Allocation> heap;
std::vector<Operation> operations;

std::vector<bool> collect_heap(void *heap_start) {
  uintptr_t hstart = (uintptr_t)heap_start;
  std::vector<bool> partitions(PAGE_SIZE / 16, false);

  // Fill heap table.
  for (const auto &obj : heap) {
    const Allocation &a = obj.second;
    size_t ostart = (((uintptr_t)a.addr) - hstart) / 16;
    for (int i = 0; i < (BLOCK_HEADER_SIZE / 16); i++) {
      partitions[ostart - 1 - i] = true;
    }

    for (int i = 0; i < (BuddyHelper::round_up_pow2(a.size) / 16); i++) {
      // for (int i = 0; i < (a.size / 16); i++) {
      partitions[ostart + i] = true;
    }
  }

  return partitions;
}

std::vector<size_t> collect_heap_holes(void *heap_start) {
  std::vector<size_t> holes;
  std::vector<bool> partitions = collect_heap(heap_start);

  size_t counter = 0;
  for (size_t i = 0; i < partitions.size(); i++) {
    if (partitions[i] || i == (partitions.size() - 1)) {
      if (i != 0 && counter > 0) {
        holes.push_back(counter);
      }
      counter = 0;
    } else {
      counter += 16;
    }
  }

  return holes;
}

void dump_heap_state(void *heap_start) {
  std::map<size_t, size_t> counts;
  std::vector<size_t> holes = collect_heap_holes(heap_start);
  size_t last_free = 0;

  for (size_t i = 0; i < holes.size(); i++) {
    if (i == holes.size() - 1) {
      last_free = holes[i];
    } else {
      counts[holes[i]]++;
    }
  }

  for (const auto &pair : counts) {
    std::cout << std::setw(7) << pair.second << " " << pair.first << std::endl;
  }

  std::cout << "END " << last_free << std::endl;
  // std::cout << "END " << std::endl;
}

void print_free_blocks(void *heap_start) {
  std::vector<size_t> holes = collect_heap_holes(heap_start);

  for (size_t hole : holes) {
    std::cout << hole << std::endl;
  }
}

void print_heap(void *heap_start) {
  std::vector<bool> partitions = collect_heap(heap_start);

  for (size_t i = 0; i < partitions.size(); i++) {
    if (partitions[i])
      std::cout << '1';
    else
      std::cout << "\033[41m \033[0m";

    if (i % 256 == 255)
      std::cout << std::endl;
  }
}

uint8_t *mmap_allocate(size_t pool_size) {
  return static_cast<uint8_t *>(mmap(nullptr, pool_size, PROT_READ | PROT_WRITE,
                                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
}

void process_file(const std::string &filename) {
  std::ifstream file(filename);

  if (!file.is_open()) {
    std::cerr << "Failed to open the file: " << filename << std::endl;
    exit(1);
  }

  char type;
  std::string id;
  size_t size;
  std::string line;

  while (std::getline(file, line)) {
    std::istringstream iss(line);
    iss >> type >> id >> size;
    operations.push_back({type, id, size});
  }

  file.close();
}

void apply_distribution(BuddyAllocator<ZConfig> *allocator, void *heap_start) {
  size_t counter = 0;
  size_t allocated_size = 0;
  size_t used_size = 0;
  bool prev_alloc = false;
  for (const Operation &op : operations) {
    counter++;
    // if (!prev_alloc && op.type == 'a') {
    //   allocator->empty_lazy_list();
    // }
    if (prev_alloc && op.type == 'f') {
      dump_heap_state(heap_start);
    }
    prev_alloc = op.type == 'a';

    // if (counter % 1000 == 0) {
    //   dump_heap_state(heap_start);
    //   std::cout << "Allocated: " << allocated_size << " Used: " << used_size
    //             << " Waste: "
    //             << (float)(allocated_size - used_size) / (float)allocated_size
    //             << std::endl;
    // }

    if (op.type == 'a') {
      if (op.size > 262144) {
        std::cout << "Skipping too large allocate request of: " << op.size
                  << std::endl;
        continue;
      }
      void *addr = allocator->allocate(op.size);
      if (addr != nullptr) {
        // heap[op.id] = {addr, JSMallocUtil::align_up(op.size, 16)};
        heap[op.id] = {addr, op.size};
        // heap[op.id] = {addr, BuddyHelper::round_up_pow2(op.size)};
        allocated_size += BuddyHelper::round_up_pow2(op.size);
        used_size += op.size;
      } else {
        std::cout << "Failed to allocate: " << op.size << std::endl;
      }
    } else if (op.type == 'f') {
      if (op.id == "(nil)") {
        // allocator.free(nullptr);
        allocator->deallocate(nullptr, 0);
      } else {
        // allocator.free(heap[op.id].addr);
        allocator->deallocate(heap[op.id].addr, heap[op.id].size);
        heap.erase(op.id);
        allocated_size -= BuddyHelper::round_up_pow2(op.size);
        used_size -= op.size;
      }
    }
  }
}

int main(int argc, char **argv) {

  if (argc < 2) {
    std::cout << "Usage: ./" << argv[0] << " <filename>" << std::endl;
    std::cout << "The provided file should describe an allocation/free pattern "
                 "on the following form:"
              << std::endl;
    std::cout << "Allocation:\t 'a <id> <size>'" << std::endl;
    std::cout << "Free:\t\t 'f <id> <size>'" << std::endl;
    exit(1);
  }

  std::string filename = argv[1];

  void *pool = mmap_allocate(PAGE_SIZE);
  // JSMallocZ allocator(pool, PAGE_SIZE, false);
  BuddyAllocator<ZConfig> *allocator =
      // BinaryBuddyAllocator<ZConfig>::create(nullptr, pool, 0, false);
      BTBuddyAllocator<ZConfig>::create(nullptr, pool, 0, false);
      // IBuddyAllocator<ZConfig>::create(nullptr, pool, 1000, false);

  process_file(filename);

  // auto start_time = std::chrono::high_resolution_clock::now();
  apply_distribution(allocator, pool);
  // print_heap(pool);
  // print_free_blocks(pool);

  // auto end_time = std::chrono::high_resolution_clock::now();
  // auto duration = end_time - start_time;
  // const double seconds = std::chrono::duration<double>(duration).count();
  // std::cout << seconds << std::endl;

  return 0;
}
