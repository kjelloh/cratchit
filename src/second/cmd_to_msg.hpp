/**
 * TEA client provided bindings: descriptor -> result_type -> Msg
 */
#pragma once

#include "executables.hpp"
#include "Msg.hpp"

tea::Msg cmd_to_msg(TestCmdDescriptor const&,TestCmdDescriptor::result_type const&);