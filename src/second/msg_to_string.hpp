#pragma once

#include "Msg.hpp"
#include <string>

std::string msg_to_string(tea::Msg const& msg);

std::string msg_to_string(tea::UnicodeKeyMsg const& m);
