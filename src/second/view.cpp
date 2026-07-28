#include "view.hpp"
#include "utf8.hpp"
#include <format>

namespace tea {

  Ux view(Model const& model) {

    if (model.app_state_stack().size() > 0) {
      auto ux = model.app_state_stack().back().view();
      return ux;
    }

    return Ux{
      {"??app state stack??"}
      ,{"??app state stack??"}
      ,{"??app state stack??"}
    };

  } // view
} // tea

