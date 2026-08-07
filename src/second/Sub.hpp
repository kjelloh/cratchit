#pragma once

#include "subscribables.hpp"

#include <vector>

namespace tea {

  // #TEA::events: Event descriptor alias
  using Sub = Subscribable;

  // #TEA::events: List of descriptors of events to listen to
  using Subs = std::vector<Sub>;

} // tea