#include "init.hpp"
#include "ViewState.hpp"

namespace tea {

  std::pair<Model,Cmd> init() {    
    return std::make_pair(
       Model{}.with_pushed_state(RootView{})
      ,Cmd{}
    );
  } // init

} // tea
