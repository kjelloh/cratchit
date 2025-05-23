#include "tokenize.hpp"
#include <iostream>
#include <ranges>
#include <iomanip> // std::quoted,
#include <sstream> // std::istringstream,
#include <regex>
#include <unicode/regex.h> // icu::RegexPattern, U_ZERO_ERROR


namespace tokenize {

  std::string trim(std::string const &s) {
    return std::ranges::to<std::string>(
        s | std::views::drop_while(::isspace) | std::views::reverse |
        std::views::drop_while(::isspace) | std::views::reverse);
  }

  bool contains(std::string const &key, std::string const &s) {
    return (s.find(key) != std::string::npos);
  }

  bool starts_with(std::string const &key, std::string const &s) {
    return s.starts_with(key);
  }

  std::string without_front_word(std::string_view sv) {
    sv.remove_prefix(std::min(sv.find_first_not_of(" "), sv.size()));
    sv.remove_prefix(std::min(sv.find_first_of(" "), sv.size()));
    sv.remove_prefix(std::min(sv.find_first_not_of(" "), sv.size()));
    return std::string{sv};
  }

  // returns s split into first,second on provided delimiter delim.
  // split fail returns first = "" and second = s
  std::pair<std::string, std::string> split(std::string s, char delim) {
    auto pos = s.find(delim);
    if (pos < s.size())
      return {s.substr(0, pos), s.substr(pos + 1)};
    else
      return {"", s}; // split failed (return right = the unsplit string)
  }

  std::vector<std::string>
  splits(std::string s, char delim,
         eAllowEmptyTokens allow_empty_tokens) {
    std::vector<std::string> result;
    try {
      // TODO: Refactor code so default is allowing for split into empty tokens
      // and make skip whitespace a special case
      if (allow_empty_tokens == eAllowEmptyTokens::no) {
        auto head_tail = split(s, delim);
        // std::cout << "\nhead" << head_tail.first << " tail:" <<
        // head_tail.second;
        while (head_tail.first.size() > 0) {
          auto const &[head, tail] = head_tail;
          result.push_back(head);
          head_tail = split(tail, delim);
          // std::cout << "\nhead" << head_tail.first << " tail:" <<
          // head_tail.second;
        }
        if (head_tail.second.size() > 0) {
          result.push_back(head_tail.second);
          // std::cout << "\ntail:" << head_tail.second;
        }
      } else {
        size_t first{}, delim_pos{};
        do {
          delim_pos = s.find(delim, first);
          if (delim_pos != std::string::npos) {
            result.push_back(s.substr(first, delim_pos - first));
            first = delim_pos + 1;
          } else {
            auto tail = s.substr(first);
            if (tail.size() > 0)
              result.push_back(tail); // add non empty tail
          }
        } while (delim_pos < s.size());
      }
    } catch (std::exception const &e) {
      std::cout << "\nDESIGN INSUFFICIENCY: splits(s,delim,allow_empty_tokens) "
                   "failed for s="
                << std::quoted(s) << ". Expception=" << std::quoted(e.what());
    }
    if (result.size() == 1 and result[0].size() == 0) {
      // Quick Fix that tokenize::splits will return one element of zero length for an empty string :\
        // 240623 - Log this path to detect where in Cratchit this may occur and potentially cause unwanted behaviour
      result.clear();
      std::cout << "\ntokensize::splits(" << std::quoted(s)
                << ") PATCHED to empty path result";
    }

    return result;
  }

  std::vector<std::string> splits(std::string const &s) {
    std::vector<std::string> result{};
    try {
      std::istringstream is{s};
      std::string token{};
      while (is >> std::quoted(token)) {
        result.push_back(token);
        token.clear();
      }
    } catch (std::exception const &e) {
      std::cout << "\nDESIGN INSUFFICIENCY: splits(s) failed for s="
                << std::quoted(s) << ". Expception=" << std::quoted(e.what());
    }

    // for (auto const& token : result) std::cout << "\n\tsplits token: " <<
    // std::quoted(token);

    return result;
  }

  std::ostream &operator<<(std::ostream &os, TokenID const &id) {
    switch (id) {
    case TokenID::Undefined:
      os << "Undefined";
      break;
    case TokenID::Caption:
      os << "Caption";
      break;
    case TokenID::Amount:
      os << "Amount";
      break;
    case TokenID::Date:
      os << "Date";
      break;
    case TokenID::Unknown:
      os << "Unknown";
      break;
    }
    return os;
  }

  TokenID token_id_of(std::string const &s) {
    // NOTE: The order of matching below matters (matches from least permissive
    // (date) to most permissive (any text)) If you put the most permissive
    // first the less permissive tokens will never be matched.
    TokenID result{TokenID::Undefined};
    // YYYYMMDD, YYYY-MM-DD
    if (const std::regex date_regex(
            "([2-9]\\d{3})-?([0]\\d|[1][0-2])-?([0-2]\\d|[3][0-1])");
        std::regex_match(s, date_regex))
      result = TokenID::Date;
    // '+','-' or none followed by nnn... (any number of digits) followed by an
    // optional ',' or '.' followed by ate least and max two digits
    else if (const std::regex amount_regex("^[+-]?\\d+([.,]\\d\\d?)?$");
             std::regex_match(s, amount_regex))
      result = TokenID::Amount;
    // any string of characters in the set a-z,A-Z, and åäöÅÄÖ. NOTE: Requires
    // the runtime locale to be set to UTF-8 encoding and that this source file
    // is also UTF-8 encoded! else if (const std::regex
    // caption_regex("[a-zA-ZåäöÅÄÖ ]+"); std::regex_match(s,caption_regex))
    // result = TokenID::Caption; else if (const std::regex caption_regex(R"([
    // -~]+)"); std::regex_match(s,caption_regex)) result = TokenID::Caption;
    // else result = TokenID::Unknown;
    else {
      // Try the ICU library Unicode services

      UErrorCode status = U_ZERO_ERROR;
      // Match any printable Unicode text
      std::unique_ptr<icu::RegexPattern> caption_regex(
          icu::RegexPattern::compile(uR"([\p{Print}]+)", 0, status));
      if (U_FAILURE(status) || !caption_regex) {
        std::cerr << "Failed to compile regex\n";
        result = TokenID::Unknown;
      }

      // Assume UTF-8 encoding
      icu::UnicodeString unicode_s = icu::UnicodeString::fromUTF8(s);

      std::unique_ptr<icu::RegexMatcher> matcher(
          caption_regex->matcher(unicode_s, status));

      if (U_FAILURE(status) || !matcher) {
        std::cerr << "Failed to create matcher\n";
        result = TokenID::Unknown;
      }

      result = matcher->matches(status) ? TokenID::Caption
                                        : result = TokenID::Unknown;
    }

    // std::cout << "\n\ttoken_id_of " << std::quoted(s) << " = " << result;

    return result;
  }

  std::vector<std::string> splits(std::string const &s, SplitOn split_on) {
    // std::cout << "\nsplits(std::string const& s,SplitOn split_on)";
    std::vector<std::string> result{};
    auto spaced_tokens = splits(s);
    std::vector<TokenID> ids{};
    for (auto const &s : spaced_tokens) {
      ids.push_back(token_id_of(s));
    }

    for (int i = 0; i < spaced_tokens.size(); ++i) {
      std::cout << "\n"
                << spaced_tokens[i] << " id:" << static_cast<int>(ids[i])
                << std::flush;
    }

    switch (split_on) {
    case SplitOn::TextAmountAndDate: {
      std::vector<TokenID> expected_id{TokenID::Caption, TokenID::Amount,
                                       TokenID::Date};
      int state{0};
      std::string s{};
      for (int i = 0; i < ids.size();) {
        if (ids[i] == expected_id[state]) {
          if (s.size() > 0)
            s += " ";
          s += spaced_tokens[i++];
        } else {
          if (s.size() > 0)
            result.push_back(s);
          ++state;
          s.clear();
        }
        if (state + 1 > expected_id.size()) {
          std::cout << "\nDESIGN INSUFFICIENCY: Error, unable to process state:"
                    << state;
          break;
        }
      }
      if (s.size() > 0)
        result.push_back(s); // add any tail
    } break;
    default: {
      std::cout << "\nERROR - Unknown split_on value "
                << static_cast<int>(split_on);
    } break;
    }

    // for (auto const& token : result) std::cout << "\n\tsplits token: " <<
    // std::quoted(token);

    return result;
  }
} // namespace tokenize
