/**
 * This is The Elm Architecture (TEA) client provided init() function
 * The name init() is used to honor Elm tutorial on the Elm architecture
 * See https://guide.elm-lang.org/architecture/
 */

#pragma once

#include "Model.hpp"
#include "Cmd.hpp"
#include <utility> // std::pair

namespace app {
  std::pair<Model,Cmd> init();
} // tea

