#pragma once

#include "Msg.hpp"
#include <variant>

struct RootState;

using State = std::variant<RootState>;

struct RootState {
  State operator()(tea::UnicodeKeyMsg const& unicode_msg) const;
};
