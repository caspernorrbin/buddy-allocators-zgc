
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

struct Alloc {
  void *addr;
  size_t size;
  size_t true_size;
};

static const size_t BLOCK_HEADER_SIZE = 0;

static const size_t PAGE_SIZE = 2097152;
// static const size_t PAGE_SIZE = 10 * 1000 * 1024;

std::map<std::string, Alloc> heap;
std::map<void *, size_t> allocs;
std::vector<Operation> operations;

std::vector<bool> collect_heap(void *heap_start) {
  uintptr_t hstart = (uintptr_t)heap_start;
  std::vector<bool> partitions(PAGE_SIZE / 16, false);

  // Fill heap table.
  for (const auto &obj : heap) {
    const Alloc &a = obj.second;
    size_t ostart = (((uintptr_t)a.addr) - hstart) / 16;
    for (int i = 0; i < (BLOCK_HEADER_SIZE / 16); i++) {
      partitions[ostart - 1 - i] = true;
    }

    for (int i = 0; i < (a.size / 16); i++) {
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

    if (i % 256 == 0)
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

void apply_distribution(BuddyAllocator<ZConfig> *allocator, void *heap_start,
                        size_t step) {
  size_t counter = 0;
  size_t heap_usage = 0;
  size_t requested_size = 0;

  bool prev_alloc = true;
  for (const Operation &op : operations) {
    counter++;

  retry:
    if (counter % step == 0) {
      // allocator.aggregate();
      // dump_heap_state(heap_start);
    }

    // if (op.type == 'a') {
    if (op.size > 262144) {
      std::cout << "Skipping too large allocate request of: " << op.size
                << std::endl;
      continue;
    }
    void *a = allocator->allocate(op.size);
    if (a != nullptr) {
      size_t blk_size = BuddyHelper::round_up_pow2(op.size);
      // std::cout << "a " << op.id << " " << op.size << std::endl;
      // heap[op.id] = {a.addr, a.blk_size};
      // heap[op.id] = {a, op.size};
      heap[op.id] = {a, blk_size, op.size};
      // allocs[a.addr] = a.blk_size;
      requested_size += op.size;
      heap_usage += blk_size;
      prev_alloc = true;
    } else {
      // Failed to allocate, start freeing objects.
      if (prev_alloc) {
        std::cout << "FAILED " << op.size << std::endl;
        std::cout << "FRAGMENTATION "
                  << (float)(heap_usage - requested_size) / (float)heap_usage
                  << std::endl;
        dump_heap_state(heap_start);
      }
      size_t cutoff = heap_usage / 2;
      // std::cout << "cutoff: " << cutoff << std::endl;

      while (heap_usage > cutoff) {
        auto it = heap.begin();
        std::advance(it, rand() % heap.size());
        std::string id = it->first;
        size_t blk_size = it->second.size;
        requested_size -= it->second.true_size;
        heap_usage -= blk_size;
        // std::cout << it->second.size << " " << heap[id].size << std::endl;
        allocator->deallocate(heap[id].addr, heap[id].size);
        heap.erase(id);
        // std::cout << "f " << id << " 0" << std::endl;
      }
      prev_alloc = false;
      goto retry;
    }
    // } else if (op.type == 'f') {
    //   if (op.id == "(nil)") {
    //     allocator->deallocate(nullptr, 0);
    //     // allocator.free(nullptr, 0);
    //   } else {
    //     if (heap.find(op.id) != heap.end()) {
    //       // allocator->free(heap[op.id].addr);
    //       allocator->deallocate(heap[op.id].addr, heap[op.id].size);
    //       // allocator.free(heap[op.id].addr, heap[op.id].blk_size);
    //       heap.erase(op.id);
    //       // allocs.erase(heap[op.id].addr);
    //     }
    //   }
    // }
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
  size_t step = 1000;

  if (argc == 3) {
    step = std::stoi(argv[2]);
  }

  void *pool = mmap_allocate(PAGE_SIZE);
  BuddyAllocator<ZConfig> *allocator =
      // BinaryBuddyAllocator<ZConfig>::create(nullptr, pool, 0, false);
      BTBuddyAllocator<ZConfig>::create(nullptr, pool, 0, false);
  // IBuddyAllocator<ZConfig>::create(nullptr, pool, 0, false);

  process_file(filename);

  // auto start_time = std::chrono::high_resolution_clock::now();
  apply_distribution(allocator, pool, step);
  // print_heap(pool);
  // print_free_blocks(pool);

  // auto end_time = std::chrono::high_resolution_clock::now();
  // auto duration = end_time - start_time;
  // const double seconds = std::chrono::duration<double>(duration).count();
  // std::cout << seconds << std::endl;

  return 0;
}
