#pragma once

#include "Model.hpp"
#include "Cmd.hpp"
#include <utility> // std::pair

namespace tea {
  std::pair<Model,Cmd> init();
} // tea

