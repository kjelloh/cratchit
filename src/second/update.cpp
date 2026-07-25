#include "update.hpp"

namespace tea {

  // helper type for the visitor
  template<class... Ts>
  struct overloaded : Ts... { using Ts::operator()...; };

  Model update(Model const& model,Msg const& msg) {

    return std::visit(overloaded{
        [&model](NoMsg no_key_msg) {
          return model;
        }
        ,[&model](UnicodeKeyMsg unicode_key_msg) {
          return model.with_pushed_unicode(unicode_key_msg.code_point);
        }
        ,[&model](BackspaceKeyMsg const& backspace_msg) {
          return model.with_popped_unicode();
        }
      }
      ,msg
    );
  } // update
} // tea
