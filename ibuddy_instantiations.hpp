#include "ibuddy.hpp"

// Should reduce to fewer instantiations in the future
// Most are not used in neither the test or program(s)

using ZConfig = IBuddyConfig<4, 18, 8, true, 4>;

template class IBuddyAllocator<IBuddyConfig<4, 27, 2, true, 0>>;
template class IBuddyAllocator<IBuddyConfig<4, 27, 1, true, 0>>;
template class IBuddyAllocator<IBuddyConfig<4, 29, 1, true, 0>>;
template class IBuddyAllocator<IBuddyConfig<4, 25, 1, true, 0>>;
template class IBuddyAllocator<IBuddyConfig<4, 25, 2, true, 0>>;
template class IBuddyAllocator<IBuddyConfig<4, 25, 4, true, 0>>;
template class IBuddyAllocator<IBuddyConfig<4, 21, 1, true, 0>>;
template class IBuddyAllocator<IBuddyConfig<4, 21, 4, true, 0>>;
template class IBuddyAllocator<IBuddyConfig<4, 18, 1, true, 0>>;
template class IBuddyAllocator<IBuddyConfig<4, 18, 1, true, 4>>;
template class IBuddyAllocator<IBuddyConfig<4, 18, 1, true, 8>>;
template class IBuddyAllocator<IBuddyConfig<4, 18, 2, true, 0>>;
template class IBuddyAllocator<IBuddyConfig<4, 18, 2, true, 4>>;
template class IBuddyAllocator<IBuddyConfig<4, 18, 2, true, 8>>;
template class IBuddyAllocator<IBuddyConfig<4, 18, 4, true, 0>>;
template class IBuddyAllocator<IBuddyConfig<4, 18, 4, true, 4>>;
template class IBuddyAllocator<IBuddyConfig<4, 18, 4, true, 8>>;
template class IBuddyAllocator<IBuddyConfig<4, 18, 8, true, 0>>;
template class IBuddyAllocator<ZConfig>;
template class IBuddyAllocator<IBuddyConfig<4, 18, 8, true, 8>>;
template class IBuddyAllocator<IBuddyConfig<4, 8, 1, true, 0>>;
template class IBuddyAllocator<IBuddyConfig<4, 8, 1, true, 4>>;
template class IBuddyAllocator<IBuddyConfig<4, 8, 1, true, 8>>;
template class IBuddyAllocator<IBuddyConfig<4, 8, 2, true, 0>>;
template class IBuddyAllocator<IBuddyConfig<4, 8, 2, true, 4>>;
template class IBuddyAllocator<IBuddyConfig<4, 8, 2, true, 8>>;
template class IBuddyAllocator<IBuddyConfig<4, 8, 4, true, 0>>;
template class IBuddyAllocator<IBuddyConfig<4, 8, 4, true, 4>>;
template class IBuddyAllocator<IBuddyConfig<4, 8, 4, true, 8>>;