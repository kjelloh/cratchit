#include "view.hpp"
#include "utf8.hpp"
#include <format>

namespace tea {

  Ux view(Model const& model) {

    if (model.state_stack().size() > 0) {
      auto ux = std::visit(
        [](auto const& s){
          return s.view();
        }
        ,model.state_stack().back()
      );
      return ux;
    }

    return Ux{
      {"??null state stack??"}
      ,{"??null_state stack??"}
      ,{"??null_state stack??"}
    };

  } // view
} // tea

