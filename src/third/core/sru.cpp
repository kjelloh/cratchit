#include "core/sru.hpp"

#include <format>
#include <fstream>
#include <optional>
#include <sstream>

namespace cratchit {

namespace {

// The words of a line, separated by spaces or tabs
std::vector<std::string_view> words_of(std::string_view line) {
  std::vector<std::string_view> words;
  std::size_t position = 0;
  while (position < line.size()) {
    auto const start = line.find_first_not_of(" \t", position);
    if (start == std::string_view::npos) {
      break;
    }
    auto end = line.find_first_of(" \t", start);
    if (end == std::string_view::npos) {
      end = line.size();
    }
    words.push_back(line.substr(start, end - start));
    position = end;
  }
  return words;
}

} // namespace

std::string_view SruForm::kind() const {
  return std::string_view(name).substr(0, name.find('-'));
}

std::expected<std::vector<SruForm>, std::string> parse_sru(std::string_view text) {
  std::vector<SruForm> forms;
  std::optional<SruForm> form;
  std::size_t number = 0;
  while (!text.empty()) {
    ++number;
    auto const end = text.find('\n');
    auto line = text.substr(0, end);
    text = end == std::string_view::npos ? std::string_view{} : text.substr(end + 1);
    if (line.ends_with('\r')) {
      line.remove_suffix(1);
    }
    auto const words = words_of(line);
    if (words.empty()) {
      continue;
    }
    auto const error = [number](std::string_view what) {
      return std::unexpected(std::format("SRU line {}: {}", number, what));
    };
    auto const label = words[0];
    if (label == "#BLANKETT") {
      if (form) {
        return error("#BLANKETT before #BLANKETTSLUT");
      }
      if (words.size() < 2) {
        return error("#BLANKETT without a name");
      }
      form = SruForm{std::string(words[1]), {}, {}};
    } else if (label == "#BLANKETTSLUT") {
      if (!form) {
        return error("#BLANKETTSLUT outside a form");
      }
      forms.push_back(std::move(*form));
      form.reset();
    } else if (label == "#IDENTITET" && form) {
      for (std::size_t index = 1; index < words.size(); ++index) {
        form->identity += (index == 1 ? "" : " ") + std::string(words[index]);
      }
    } else if (label == "#UPPGIFT") {
      if (!form) {
        return error("#UPPGIFT outside a form");
      }
      if (words.size() < 3) {
        return error("#UPPGIFT without a code and a value");
      }
      if (!form->fields.emplace(words[1], words[2]).second) {
        return error(std::format("field {} given twice", words[1]));
      }
    }
  }
  if (form) {
    return std::unexpected(std::format("SRU: the form {} is not ended by #BLANKETTSLUT", form->name));
  }
  return forms;
}

std::expected<std::vector<SruForm>, std::string> read_sru(std::filesystem::path const& file) {
  std::ifstream in(file, std::ios::binary);
  if (!in) {
    return std::unexpected(std::format("Cannot read {}", file.string()));
  }
  std::ostringstream content;
  content << in.rdbuf();
  auto forms = parse_sru(content.str());
  if (!forms) {
    return std::unexpected(std::format("{}: {}", file.string(), forms.error()));
  }
  return forms;
}

} // namespace cratchit
