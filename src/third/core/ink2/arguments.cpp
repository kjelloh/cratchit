#include "core/ink2/arguments.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <format>

namespace cratchit::ink2 {

namespace {

std::string lower_extension(std::filesystem::path const& path) {
  auto extension = path.extension().string();
  std::ranges::transform(extension, extension.begin(),
                         [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return extension;
}

} // namespace

std::expected<Arguments, std::string> parse_arguments(std::vector<std::string_view> const& arguments) {
  if (arguments.size() == 1 && (arguments.front() == "--help" || arguments.front() == "-h")) {
    return Arguments{true, false, {}, std::nullopt, std::nullopt};
  }
  Arguments parsed;
  bool has_sie = false;
  for (std::size_t index = 0; index < arguments.size(); ++index) {
    auto const argument = arguments[index];
    if (argument == "--trace") {
      if (parsed.trace) {
        return std::unexpected("--trace is given twice");
      }
      parsed.trace = true;
      continue;
    }
    if (argument == "--previous-deficit") {
      if (parsed.previous_deficit) {
        return std::unexpected("--previous-deficit is given twice");
      }
      if (index + 1 == arguments.size()) {
        return std::unexpected("--previous-deficit needs last year's underskott in kronor, e.g. --previous-deficit 0");
      }
      auto const text = arguments[++index];
      std::int64_t kronor = 0;
      auto const [end, error] = std::from_chars(text.data(), text.data() + text.size(), kronor);
      if (text.empty() || error != std::errc{} || end != text.data() + text.size() || kronor < 0) {
        return std::unexpected(
            std::format("--previous-deficit needs whole kronor of 0 or more, without spaces, not \"{}\"", text));
      }
      parsed.previous_deficit = kronor;
      continue;
    }
    if (argument.starts_with('-')) {
      return std::unexpected(std::format("Unknown option {}", argument));
    }
    std::filesystem::path const path(argument);
    auto const extension = lower_extension(path);
    std::error_code error;
    if (extension == ".se" || extension == ".si" || extension == ".sie") {
      if (has_sie) {
        return std::unexpected(std::format("Only one SIE file, not also {}", argument));
      }
      parsed.sie = path;
      has_sie = true;
    } else if (extension == ".sru" || std::filesystem::is_directory(path, error)) {
      if (parsed.previous) {
        return std::unexpected(std::format("Only one of last year's returns, not also {}", argument));
      }
      parsed.previous = path;
    } else {
      return std::unexpected(std::format(
          "{} is neither an SIE file (.se, .si, .sie) nor last year's return (.sru, or a folder of it)", argument));
    }
  }
  if (!has_sie) {
    return std::unexpected("No SIE file given");
  }
  if (parsed.previous && parsed.previous_deficit) {
    return std::unexpected("Give last year's return or --previous-deficit, not both");
  }
  if (!parsed.previous && !parsed.previous_deficit) {
    return std::unexpected(
        "Last year's return is needed for its underskott: its BLANKETTER.SRU (or folder), or --previous-deficit");
  }
  return parsed;
}

} // namespace cratchit::ink2
