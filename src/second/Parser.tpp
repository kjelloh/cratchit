/**
 * A runtime composable Parser
 * tailored for persistent cratchit Archive parsing
 */
#pragma once

#include <functional> // std::function,
#include <variant>
#include <string_view>
#include <expected>

// Archive values
namespace archive {

  struct Input {
    Input(char const* sz) : m_in(sz) {}
    size_t pos() const {return m_pos;}
    Input advanced(size_t dix) const {
      auto result = *this;
      result.m_pos += dix;
      return result;
    }
    std::string_view m_in;
    size_t m_pos;
  }; // test::Input

  struct Number{};
  struct Text{};

  using Value = std::variant<
     Number
    ,Text
  >; // Value

} // archive

// Parser combinator
namespace parsing {

  // Hard coded for acrhive for now
  using Value = archive::Value;
  using Input = archive::Input;

  using Success = std::tuple<Value,Input>;
  struct ParseError {}; // ParseError type place holder

  using Result = std::expected<Success,ParseError>;

  using Parser = std::function<Result(Input)>;

  using Parser = std::function<parsing::Result(parsing::Input)>;

} // parsing

namespace archive {
  // Archive parsing specifics goes here

  parsing::Result persistent_acrhive_entry(parsing::Input) {
    return std::unexpected(parsing::ParseError{});
  }

}; // archive

using Input = parsing::Input;
using Parser = parsing::Parser;
using Result = parsing::Result;

Result parse(Parser parser,Input input) {
  return parser(input);
} // parse
