#include "text/functional.hpp"

namespace text {

  namespace functional {

    std::string_view trim(std::string_view s) {
      auto is_space = [](unsigned char c) { return std::isspace(c); };

      // Find first non-space character
      auto start = std::ranges::find_if_not(s, is_space);
      if (start == s.end()) return {};

      // Find last non-space character (search from end)
      auto rstart = s.rbegin();
      auto rend = s.rend();
      auto r_last_non_space = std::find_if(rstart, rend, [&](unsigned char c) {
        return !is_space(c);
      });

      // Convert reverse iterator to forward iterator
      auto end = r_last_non_space.base();

      return std::string_view(start, end);
    }

    size_t count_digits(std::string_view s) {
      return std::ranges::count_if(s, [](unsigned char c) {
        return std::isdigit(c);
      });
    }

    bool is_all_digits(std::string_view s) {
      return !s.empty() && std::ranges::all_of(s, [](unsigned char c) {
        return std::isdigit(c);
      });
    }

    bool is_all_zeros(std::string_view s) {
      return !s.empty() && std::ranges::all_of(s, [](char c) {
        return c == '0';
      });
    }

    std::optional<size_t> find_dash_position(std::string_view s) {
      auto it = std::ranges::find(s, '-');
      if (it == s.end()) {
        return std::nullopt;
      }
      return std::distance(s.begin(), it);
    }

    size_t count_dashes(std::string_view s) {
      return std::ranges::count(s, '-');
    }

    bool contains_any_keyword(std::string_view text, std::initializer_list<std::string_view> keywords) {
      std::string lower_text;
      lower_text.reserve(text.size());
      std::ranges::transform(text, std::back_inserter(lower_text),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

      return std::ranges::any_of(keywords, [&lower_text](std::string_view keyword) {
        return lower_text.find(keyword) != std::string::npos;
      });
    }

  } // functional
} // text
