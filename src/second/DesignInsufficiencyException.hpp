#pragma once

#include <exception>
#include <string>

class DesignInsufficiencyException : public std::runtime_error {
public:
  DesignInsufficiencyException(std::string what);
}; // DesignInsufficiencyException