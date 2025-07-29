#include "log.hpp"
#include <string_view>

logger::std_out_proxy::std_out_proxy() {
}
logger::std_out_proxy::~std_out_proxy() {
  this->flush();
}

logger::std_out_proxy& logger::std_out_proxy::operator<<(std::ostream& (*manip)(std::ostream&)) {
  // Check if it's std::flush specifically
  if (manip == static_cast<std::ostream& (*)(std::ostream&)>(std::flush)) {
    flush();
  } else {
    oss << manip;
    try_flush_to_spdlog();
  }
  return *this;
}    

// New line

void logger::std_out_proxy::try_flush_to_spdlog() {
    // spdlog::info("logger::std_out_proxy::try_flush_to_spdlog: Entry");
    std::string buffer = this->oss.str();  // Get hold of copy (oss.str() is not a cont&)

    std::string_view view(buffer);
    size_t start = 0;

    while (true) {
        size_t pos = view.find('\n', start);
        if (pos == std::string_view::npos) break;

        // spdlog::info("logger::std_out_proxy::try_flush_to_spdlog: Complete line {}..{}",start,pos - start);

        // Create a view of the line without copying
        std::string_view line = view.substr(start, pos - start);

        if (line.size() > 0) {
          spdlog::info(std::string(line));  // spdlog usually wants a string or c-string
          // spdlog::info("logger::std_out_proxy::try_flush_to_spdlog: line: {}",line);
        }

        start = pos + 1;
    }

    // Save leftover partial line
    std::string_view leftover_view = view.substr(start);

    // spdlog::info("logger::std_out_proxy::try_flush_to_spdlog: leftover_view: {}",leftover_view);

    // Put back incomplete line
    oss.str("");
    oss << leftover_view;
}

logger::std_out_proxy& logger::std_out_proxy::flush() {
  std::string content = oss.str();
  if (!content.empty()) {
    spdlog::info(content);
    oss.str("");
    oss.clear();
  }
  return *this;
}
