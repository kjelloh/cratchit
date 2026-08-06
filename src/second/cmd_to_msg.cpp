#include "cmd_to_msg.hpp"

tea::Msg cmd_to_msg(TestCmdDescriptor const&,TestCmdDescriptor::payload_type const& payload) {
  return tea::TestCmdResultMsg{payload};
}