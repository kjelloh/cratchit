/**
 * This is The Elm Architecture (TEA) client provided view() function
 * This name is used to honor Elm tutorial on the Elm architecture
 * See https://guide.elm-lang.org/architecture/
 */

#pragma once

#include "Model.hpp"
#include "Ux.hpp"

namespace tea {
  Ux view(Model const& model);
} // tea

