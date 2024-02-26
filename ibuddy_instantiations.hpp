#ifndef IBUDDY_INSTANTIATIONS_HPP_
#define IBUDDY_INSTANTIATIONS_HPP_

#include "buddy_config.hpp"
#include "ibuddy.hpp"

template class IBuddyAllocator<ZConfig>;
template class IBuddyAllocator<SmallSingleConfig>;
template class IBuddyAllocator<SmallDoubleConfig>;
template class IBuddyAllocator<LargeQuadConfig>;
template class IBuddyAllocator<MallocConfig>;

#endif // IBUDDY_INSTANTIATIONS_HPP_