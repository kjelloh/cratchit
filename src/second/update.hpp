#pragma once

#include "Model.hpp"
#include "Msg.hpp"
#include "Cmd.hpp"
#include <tuple>

namespace tea {
  std::tuple<Model,Cmd> update(Model const& model,Msg const& msg);
} // tea
