#pragma once

#include "ViewState.hpp"

#include <string>

std::string view_to_string(ViewState const& view);

std::string concrete_view_to_string(RuntimeView const& concrete_view);
