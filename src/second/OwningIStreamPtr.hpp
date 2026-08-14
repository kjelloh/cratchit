#pragma once

#include <memory> // std::unique_ptr
#include <istream>

using OwningIStreamPtr = std::unique_ptr<std::istream>;
