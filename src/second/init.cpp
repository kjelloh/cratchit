#include "init.hpp"
#include "ViewState.hpp"

namespace app {

  std::pair<Model,tea::Cmd> init() {    
    return std::make_pair(
       Model{}.with_view_state(ViewState{RootView{}})
      ,tea::Cmd{}
    );
  } // init

} // tea
