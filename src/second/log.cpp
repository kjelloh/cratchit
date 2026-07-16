#include "log.hpp"
#include <print>
#include <fstream>
#include <exception>

const std::string log_file_name("cratchit.log");
static std::ofstream log_file(log_file_name);

bool log_file_ok() {
  return static_cast<bool>(log_file);
}

namespace detail {
  void log_to_file(std::string_view sv) {
    if (log_file_ok()) {
      auto log_entry = std::format("SOME_TIME_STAMP:{}",sv);
      log_file << "\n" << log_entry;
    }
    else throw std::runtime_error(std::format(
      "FAILED to create log file '{}'"
      ,log_file_name));
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