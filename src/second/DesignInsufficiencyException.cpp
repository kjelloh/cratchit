#include "DesignInsufficiencyException.hpp"

#include <format>

DesignInsufficiencyException::DesignInsufficiencyException(std::string what)
  : runtime_error(std::format("DESIGN_INSUFFICIENCY: {}",what)) {}
