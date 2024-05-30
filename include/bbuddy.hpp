#ifndef BBUDDY_HPP_
#define BBUDDY_HPP_

#include "buddy_allocator.hpp"
#include "buddy_helper.hpp"
#include "buddy_instantiations.hpp"
#include <cstddef>
#include <cstdint>

template <typename Config>
class BinaryBuddyAllocator : public BuddyAllocator<Config> {
public:
  BinaryBuddyAllocator(void *start, int lazyThreshold, bool startFull);
  ~BinaryBuddyAllocator() = default;
  BinaryBuddyAllocator(const BinaryBuddyAllocator &) = delete;
  BinaryBuddyAllocator &operator=(const BinaryBuddyAllocator &) = delete;

  static BinaryBuddyAllocator *create(void *addr, void *start,
                                      int lazyThreshold, bool startFull);

protected:
  void *allocate_internal(size_t size) override;
  void deallocate_internal(void *ptr, size_t size) override;
  void init_bitmaps(bool startFull) override;

private:
  void init_free_lists();
  unsigned int map_index(unsigned int index);
  uint8_t level_alignment(uintptr_t ptr, uint8_t region, uint8_t start_level);
};

#endif // BBUDDY_HPP