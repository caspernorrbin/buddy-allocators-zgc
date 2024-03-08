#ifndef IBUDDY_HPP_
#define IBUDDY_HPP_

#include "buddy_allocator.hpp"
#include "buddy_helper.hpp"
#include "buddy_instantiations.hpp"
#include <cstddef>
#include <cstdint>

template <typename Config>
class IBuddyAllocator : public BuddyAllocator<Config> {
public:
  IBuddyAllocator(void *start, int lazyThreshold, bool startFull);
  ~IBuddyAllocator() = default;
  IBuddyAllocator(const IBuddyAllocator &) = delete;
  IBuddyAllocator &operator=(const IBuddyAllocator &) = delete;

  static IBuddyAllocator *create(void *addr, void *start, int lazyThreshold,
                                 bool startFull);

  void deallocate_range(void *ptr, size_t size) override;

protected:
  void *allocate_internal(size_t size) override;
  void deallocate_internal(void *ptr, size_t size) override;
  void init_bitmaps(bool startFull) override;

private:
  void init_free_lists();
  void deallocate_single(uintptr_t ptr);
};

#endif // IBUDDY_HPP