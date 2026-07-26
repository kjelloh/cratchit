#include "init.hpp"
#include "State.hpp"

namespace tea {

  std::pair<Model,Cmd> init() {    
    return std::make_pair(
       Model{}.with_pushed_state(RootState{})
      ,Cmd{}
    );
  } // init

} // tea
