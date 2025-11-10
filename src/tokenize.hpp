#pragma once

#include <string>
#include <vector>
#include <string_view>

namespace tokenize {

  std::string trim(std::string const& s);
  bool contains(std::string const& key, std::string const& s);
  bool starts_with(std::string const& key, std::string const& s);
  std::string without_front_word(std::string_view sv);

  // returns s split into first,second on provided delimiter delim.
  // split fail returns first = "" and second = s
  std::pair<std::string, std::string> split(std::string s, char delim);

  enum class eAllowEmptyTokens { unknown, no, YES, undefined };

  std::vector<std::string> splits(std::string s, char delim,
         eAllowEmptyTokens allow_empty_tokens = eAllowEmptyTokens::no);

  std::vector<std::string> splits(std::string const& s);

  enum class TokenID { Undefined, Caption, Amount, Date, Unknown };

  std::ostream &operator<<(std::ostream &os, TokenID const& id);

  TokenID token_id_of(std::string const& s);

  enum class SplitOn { Undefined, TextAndAmount, TextAmountAndDate, Unknown };

  std::vector<std::string> splits(std::string const& s, SplitOn split_on);
} // namespace tokenize
