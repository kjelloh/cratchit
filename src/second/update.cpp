#include "update.hpp"

namespace tea {

  // helper type for the visitor
  template<class... Ts>
  struct overloaded : Ts... { using Ts::operator()...; };

  Model update(Model const& model,Msg const& msg) {

    return std::visit(overloaded{
        [&model](NoMsg arg) {
          return model;
        }
        ,[&model](UnicodeMsg msg) {
          return model.with_pushed_unicode(msg.code_point);
        }
      }
      ,msg
    );
  } // update
} // tea
