#include "log.hpp"
#include <print>

void log_business(std::string_view sv) {
}
void log_development_trace(std::string_view sv) {
  std::print("DEVELOPMENT_TRACE:{}",sv);
}
void log_design_insufficiency(std::string_view sv) {
}