#include "log.hpp"
#include <print>

namespace detail {
  void log_to_file(std::string_view sv) {
    std::print("\nSOME_TIME_STAMP:{}",sv);
  }
} // detail

void log_business(std::string_view sv) {
  detail::log_to_file(std::format("BUSINESS:'{}'",sv));
}
void log_development_trace(std::string_view sv) {
  detail::log_to_file(std::format("DEVELOPMENT_TRACE:'{}'",sv));
}
void log_design_insufficiency(std::string_view sv) {
  detail::log_to_file(std::format("DESIGN_INSUFFICIENCY:'{}'",sv));
}