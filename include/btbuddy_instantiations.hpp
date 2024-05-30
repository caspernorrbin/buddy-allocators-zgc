#ifndef BTBUDDY_INSTANTIATIONS_HPP_
#define BTBUDDY_INSTANTIATIONS_HPP_

#include "btbuddy.hpp"
#include "buddy_config.hpp"

template class BTBuddyAllocator<ZConfig>;
template class BTBuddyAllocator<SmallSingleConfig>;
template class BTBuddyAllocator<SmallDoubleConfig>;
template class BTBuddyAllocator<LargeQuadConfig>;
template class BTBuddyAllocator<MallocConfig>;

#endif // BTBUDDY_INSTANTIATIONS_HPP_