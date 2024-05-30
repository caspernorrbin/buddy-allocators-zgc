#ifndef BTBUDDY_HPP_
#define BTBUDDY_HPP_

#include "buddy_allocator.hpp"
#include "buddy_helper.hpp"
#include "buddy_instantiations.hpp"
#include <cstddef>
#include <cstdint>

template <typename Config>
class BTBuddyAllocator : public BuddyAllocator<Config> {
public:
  BTBuddyAllocator(void *start, int lazyThreshold, bool startFull);
  ~BTBuddyAllocator() = default;
  BTBuddyAllocator(const BTBuddyAllocator &) = delete;
  BTBuddyAllocator &operator=(const BTBuddyAllocator &) = delete;

  static BTBuddyAllocator *create(void *addr, void *start,
                                      int lazyThreshold, bool startFull);

  // void deallocate_range(void *ptr, size_t size) override;

  void print_free_list() override;

protected:
  void *allocate_internal(size_t size) override;
  void deallocate_internal(void *ptr, size_t size) override;
  void init_bitmaps(bool startFull) override;

private:
  void init_free_lists();
  uint8_t tree_height(size_t size);
  void set_tree(uint8_t region, unsigned int index, unsigned char value);
  unsigned char get_tree(uint8_t region, unsigned int index);
  
  unsigned char _btTree[Config::numRegions][1U << Config::numLevels];
  unsigned char _btBits[Config::numLevels] = {8};
  unsigned int _levelOffsets[Config::numLevels] = {0};
};

#endif // BTBUDDY_HPP