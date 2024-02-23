#ifndef BUDDY_INSTANTIATIONS_HPP_
#define BUDDY_INSTANTIATIONS_HPP_

#include "buddy_config.hpp"
#include "buddy_allocator.hpp"

// Should reduce to fewer instantiations in the future
// Most are not used in neither the test or program(s)

template class BuddyAllocator<ZConfig>;
template class BuddyAllocator<SmallSingleConfig>;
template class BuddyAllocator<SmallDoubleConfig>;
template class BuddyAllocator<LargeQuadConfig>;
template class BuddyAllocator<MallocConfig>;

#endif // BUDDY_INSTANTIATIONS_HPP_
