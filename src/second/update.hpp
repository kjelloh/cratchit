/**
 * This is The Elm Architecture (TEA) client provided update() function
 * This name is used to honor Elm tutorial on the Elm architecture
 * See https://guide.elm-lang.org/architecture/
 */

#pragma once

#include "Model.hpp"
#include "Msg.hpp"
#include "Cmd.hpp"
#include <tuple>

namespace app {
  std::tuple<app::Model,tea::Cmd> update(app::Model const& model,Msg const& msg);
} // tea
