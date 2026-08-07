#include "cmd_to_msg.hpp"

namespace app {

  Msg cmd_to_msg(tea::TestCmdDescriptor const&,tea::TestCmdDescriptor::payload_type const& payload) {
    return app::TestCmdResultMsg{payload};
  }

} // tea