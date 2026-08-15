#include "init.hpp"
#include "ViewState.hpp"
#include "import_archive.hpp"
#include "log.hpp"
#include "expected_to_string.tpp"

#include <expected>

namespace app {

  std::pair<Model,tea::Cmd> init() {
    auto root_view = RootView{};
    auto model = Model{}
      .with_pushed_view_state(root_view);

    if (true) {
      // POC for persistent file import

      auto import_result = import_archive("./cratchit/runtime.cfg");
      if (import_result) {
        log_development_trace(
           "import_archive SUCCESS: {}"
          ,expected_to_string(import_result)
        );
      }
      else {
        log_development_trace(
          "import_archive failed: {}"
          ,expected_to_string(import_result)
        );
      }
    } // if POC

    return std::make_pair(
       model
      ,tea::Cmd{}
    );
  } // init

} // tea
