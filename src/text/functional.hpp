#pragma once
#include "tokenize.hpp" // TODO: Consider to move here?
#include <string>
#include <algorithm> // std::copy_if,
#include <map>
#include <sstream> // std::ostringstream,...

namespace functional {
  namespace text {
    // TODO: Refactor away?
    inline std::string filtered(std::string const& s,auto filter) {
      std::string result{};;
      std::copy_if(s.begin(),s.end(),std::back_inserter(result),filter);
      return result;
    }
  }
}

namespace text {
  namespace functional {

      // Trim whitespace from both ends
      std::string_view trim(std::string_view s);

      // Count digits in a string
      size_t count_digits(std::string_view s);

      // Check if string contains only digits
      bool is_all_digits(std::string_view s);

      // Check if suffix is all zeros
      bool is_all_zeros(std::string_view s);

      // Find dash position in string
      std::optional<size_t> find_dash_position(std::string_view s);

      // Count occurrences of dash character
      size_t count_dashes(std::string_view s);

      bool contains_any_keyword(std::string_view text, std::initializer_list<std::string_view> keywords);

      inline std::string utf_ignore_to_upper(std::string const& s) {

        auto utf_ignore_to_upper_f = [](char ch) {
          if (ch <= 0x7F) return static_cast<char>(std::toupper(ch));
          return ch;
        };

        std::string result{};
        std::transform(s.begin(),s.end(),std::back_inserter(result),utf_ignore_to_upper_f);
        return result;
      }

      inline std::vector<std::string> utf_ignore_to_upper(std::vector<std::string> const& tokens) {
        std::vector<std::string> result{};
        auto f = [](std::string s) {return utf_ignore_to_upper(s);};
        std::transform(tokens.begin(),tokens.end(),std::back_inserter(result),f);
        return result;
      }


    inline bool strings_share_tokens(std::string const& s1,std::string const& s2) {
      bool result{false};
      auto s1_words = utf_ignore_to_upper(tokenize::splits(s1));
      auto s2_words = utf_ignore_to_upper(tokenize::splits(s2));
      for (int i=0; (i < s1_words.size()) and !result;++i) {
        for (int j=0; (j < s2_words.size()) and !result;++j) {
          result = (s1_words[i] == s2_words[j]);
        }
      }
      return result;
    }

    inline bool first_in_second_case_insensitive(std::string const& s1, std::string const& s2) {
      auto upper_s1 = utf_ignore_to_upper(s1);
      auto upper_s2 = utf_ignore_to_upper(s2);
      return (upper_s2.find(upper_s1) != std::string::npos);
    }

    namespace out {

      template <typename K,typename T>
      std::string to_string(std::map<K,T> const& c,std::string sep = " ") {
        std::ostringstream os{};
        bool not_first{false};
        std::for_each(c.begin(), c.end(), [&sep,&not_first, &os](auto const& entry) {
          if (not_first) {
            os << sep; // separator
          }
          os << entry.first;
          os << "=";
          os << entry.second;
          not_first = true;
        });
        return os.str();
      }

    }
  
  } // functional
} // text
