#include "log.hpp"
#include <print>
#include <fstream>
#include <exception>

const std::string log_file_name("cratchit.log");

std::ofstream& log_file_instance()
{
    // create static instance on first call
    static std::ofstream file("cratchit.log"); // Open fresh empty log file
    return file;
}

bool log_file_ok() {
  return static_cast<bool>(log_file_instance());
}

namespace detail {
  void log_to_file(std::string_view sv) {
    if (log_file_ok()) {
      auto log_entry = std::format("SOME_TIME_STAMP:{}",sv);
      log_file_instance() << "\n" << log_entry;
    }
    else throw std::runtime_error(std::format(
      "FAILED to use not-ok log file '{}'"
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