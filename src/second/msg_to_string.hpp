#pragma once

#include "Msg.hpp"
#include <string>

// Msg (variant) to string conversion for logging and development trace
std::string msg_to_string(tea::Msg const& msg);

// Concrete msg to string conversion for logging and development trace
// Uses prefix 'concrete' to clarify 'template magic' code that dipatches both on:
// 1. variant msg -> concrete msg
// 2. concrete_msg_to_string(concrete msg) or fallback to concrete msg type info string if no concrete_msg_to_string
std::string concrete_msg_to_string(tea::UnicodeKeyMsg const& m);
