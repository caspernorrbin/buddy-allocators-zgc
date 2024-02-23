#include "ibuddy.hpp"
#include "buddy_config.hpp"

// Should reduce to fewer instantiations in the future
// Most are not used in neither the test or program(s)

template class IBuddyAllocator<ZConfig>;
template class IBuddyAllocator<SmallSingleConfig>;
template class IBuddyAllocator<SmallDoubleConfig>;
template class IBuddyAllocator<LargeQuadConfig>;
template class IBuddyAllocator<MallocConfig>;
