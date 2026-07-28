#include "init.hpp"
#include "ViewState.hpp"

namespace tea {

  std::pair<Model,Cmd> init() {    
    return std::make_pair(
       Model{}.with_view_state(ViewState{RootView{}})
      ,Cmd{}
    );
  } // init

} // tea
