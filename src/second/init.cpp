#include "init.hpp"
#include "ViewState.hpp"
#include "log.hpp"

namespace app {

  std::pair<Model,tea::Cmd> init() {
    auto root_view = RootView{};
    auto model = Model{}
      .with_pushed_view_state(root_view);

    return std::make_pair(
       model
      ,tea::Cmd{}
    );
  } // init

} // tea
