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


} // archive

// Parser combinator
namespace parsing {

  // Immutable Input
  class Input {
  public:
    using code_point_type = char;
    Input(char const* sz) : m_in(sz) {}
    Input consumed(size_t consumed_count) const {
      auto result = *this;
      result.m_in = result.m_in.substr(consumed_count);
      return result;
    }
    std::string_view view() const {
      return m_in;
    }
  private:
    std::string_view m_in;
  }; // test::Input

  struct NaturalNumber{
    size_t value;
  };
  struct Text{
    std::string value;
  };

  using Value = std::variant<
     NaturalNumber
    ,Text
  >; // Value

  using Success = std::tuple<Value,Input>;
  struct ParseError {}; // ParseError type place holder

  using Result = std::expected<Success,ParseError>;

  // runtime composable Parser (produces variant Value)
  using Parser = std::function<parsing::Result(parsing::Input)>;

  Parser natural_number() {
    using uint8_t_vector = std::vector<uint8_t>;
    auto is_ascii_digit = [](uint8_t cp) -> bool {return cp>='0' and cp<='9';};
    auto ascii_digits_to_size_t = [](uint8_t_vector const& v) {
      size_t result{};
      for (auto n : v) result = result*10 + n;
      return result;
    };
    return [is_ascii_digit,ascii_digits_to_size_t](Input input) -> Result {
      auto text = input.view();
      std::vector<uint8_t> ascii_digits{};
      for (size_t ix=0;ix<text.size() and is_ascii_digit(text[ix]);++ix) {
        ascii_digits.push_back(static_cast<uint8_t>(text[ix]));
      }
      if (ascii_digits.size()==0) return std::unexpected(ParseError{});
      
      return Success{
         NaturalNumber{ascii_digits_to_size_t(ascii_digits)}
        ,input.consumed(ascii_digits.size())
      };
    }; // lambda
  } // number


  Parser literal(std::string_view expected) {
    return [expected](Input input) -> Result {
      auto text = input.view();

      if (!text.starts_with(expected)) {
        return std::unexpected(ParseError{});
      }

      return Success{
        Text{std::string(expected)},
        input.consumed(expected.size())
      };
    };
  } // literal

  Parser sequence(Parser first, Parser second) {
    return [first, second](Input input) -> Result {
        auto r1 = first(input);

        if (!r1) {
            return std::unexpected(r1.error());
        }

        auto [value1, remaining] = r1.value();

        auto r2 = second(remaining);

        if (!r2) {
            return std::unexpected(r2.error());
        }

        auto [value2, remaining2] = *r2;

        // Until structured values, return the second one
        return Success{
            value2,
            remaining2
        };
    }; // lambda
  } // sequence

  template<class F>
  Parser map(Parser parser, F transform) {
    return [parser, transform](Input input) -> Result {
      auto result = parser(input);

      if (!result) {
        return std::unexpected(result.error());
      }

      auto [value, remaining] = *result;

      return Success{
         transform(value)
        ,remaining
      };
    };
  } // map

  Parser flat_map(
    Parser parser,
    std::function<Parser(Value)> next) {

      return [parser, next](Input input) -> Result {
        auto r1 = parser(input);

        if (!r1) {
          return std::unexpected(r1.error());
        }

        auto [value, remaining] = *r1;

        auto next_parser = next(value);

        return next_parser(remaining);
      }; // lambda
  } // flat_map

  Parser choice(Parser first, Parser second) {
    return [first, second](Input input) -> Result {
      auto result = first(input);

      if (result) {
        return result;
      }

      return second(input);
    }; // lambda
  } // choice

} // parsing

namespace archive {
  // Archive parsing specifics goes here

  // runtime composable Parser (produces variant Value)
  parsing::Result persistent_acrhive_entry(parsing::Input input) {
    using namespace parsing;
    return sequence(
       literal("magic_value")
      ,sequence(
         literal("=")
        ,natural_number()
      )
    )(input);    
  };

}; // archive

using Input = parsing::Input;
using Parser = parsing::Parser;
using Result = parsing::Result;

Result parse(Parser parser,Input input) {
  return parser(input);
} // parse
