#include "cmd_to_msg.hpp"

app::Msg cmd_to_msg(TestCmdDescriptor const&,TestCmdDescriptor::payload_type const& payload) {
  return app::TestCmdResultMsg{payload};
}