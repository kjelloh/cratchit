/**
 * TEA client provided bindings: descriptor -> result_type -> Msg
 */
#pragma once

#include "executables.hpp"
#include "Msg.hpp"

namespace app {

  Msg cmd_to_msg(tea::TestCmdDescriptor const&,tea::TestCmdDescriptor::payload_type const&);

} // tea