#include "log.hpp"
#include <string_view>

log::std_out_proxy& log::std_out_proxy::operator<<(std::ostream& (*manip)(std::ostream&)) {
  oss << manip;
  try_flush_to_spdlog();
  return *this;
}    

// New line

void log::std_out_proxy::try_flush_to_spdlog() {
    std::string buffer = this->oss.str();  // Get hold of copy (oss.str() is not a cont&)

    std::string_view view(buffer);
    size_t start = 0;

    while (true) {
        size_t pos = view.find('\n', start);
        if (pos == std::string_view::npos) break;

        // Create a view of the line without copying
        std::string_view line = view.substr(start, pos - start);

        spdlog::info(std::string(line));  // spdlog usually wants a string or c-string

        start = pos + 1;
    }

    // Save leftover partial line
    std::string_view leftover_view = view.substr(start);

    oss.clear();
    oss.str(leftover_view.data()); // copy back
}

