#include "init.hpp"
#include "ViewState.hpp"

namespace app {

  std::pair<Model,tea::Cmd> init() {

    auto root_view = RootView{};
    return std::make_pair(
       Model{}.with_view_state(root_view)
      ,tea::Cmd{}
    );
  } // init

} // tea
