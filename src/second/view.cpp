#include "view.hpp"
#include "utf8.hpp"
#include <format>

namespace app {

  tea::Ux view(Model const& model) {

    if (model.view_state_stack().size() > 0) {
      auto ux = std::visit(
        [](auto const& s) {
          return s.view();
        }
        ,model.view_state_stack().back()
      );
      return ux;
    }

    // if (model.app_state_stack().size() > 0) {
    //   auto ux = model.app_state_stack().back().view();
    //   return ux;
    // }

    return tea::Ux{
      {"??app state stack??"}
      ,{"??app state stack??"}
      ,{"??app state stack??"}
    };

  } // view
} // tea

