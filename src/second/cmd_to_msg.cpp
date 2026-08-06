#include "cmd_to_msg.hpp"

tea::Msg cmd_to_msg(TestCmdDescriptor const&,TestCmdDescriptor::result_type const&) {
  return tea::TestCmdResultMsg{};
}