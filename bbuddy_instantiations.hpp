#ifndef BBUDDY_INSTANTIATIONS_HPP_
#define BBUDDY_INSTANTIATIONS_HPP_

#include "bbuddy.hpp"
#include "buddy_config.hpp"

template class BinaryBuddyAllocator<ZConfig>;
template class BinaryBuddyAllocator<SmallSingleConfig>;
template class BinaryBuddyAllocator<SmallDoubleConfig>;
template class BinaryBuddyAllocator<LargeQuadConfig>;
template class BinaryBuddyAllocator<MallocConfig>;

#endif // BBUDDY_INSTANTIATIONS_HPP_