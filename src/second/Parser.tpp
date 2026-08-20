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

  // No Archive domain values yet

} // archive

// Parser combinator
namespace parsing {

  // Immutable Input
  class Input {
  public:
    using code_point_type = char;
    Input(char const* sz) 
      : m_in(sz)
        ,m_absolute_pos{0} {}

    Input(Input const& input) = default;
    
    Input consumed(size_t consumed_count) const {
      auto result = *this;
      result.m_in = result.m_in.substr(consumed_count);
      result.m_absolute_pos += consumed_count;
      return result;
    }
    std::string_view view() const {
      return m_in;
    }

    size_t pos() const {return m_absolute_pos;}

  private:
    std::string_view m_in;
    size_t m_absolute_pos;
  }; // test::Input

  struct NaturalNumber{
    size_t value;
  };

  struct Text{
    bool operator==(Text const&) const = default;
    std::string value;
  };

  template <typename LHS,typename RHS>
  struct Both {
    LHS lhs;
    RHS rhs;
  }; // Both

  template <typename LHS,typename RHS>
  struct Either {
    std::optional<LHS> lhs;
    std::optional<RHS> rhs;
  }; // Either

  template <typename value_type>
  using Success = std::tuple<value_type,Input>;

  struct ParseError {
    std::string caption;
    size_t pos;
  }; // ParseError

  ParseError make_error(std::string caption,Input const& input) {
    return ParseError{
       std::format(
         "{}:at[{}:'{}...']"
        ,caption
        ,input.pos()
        ,input.view().substr(0,4)
       )
      ,input.pos()
    };
  } // make_error

  ParseError make_composed_error(std::string caption,ParseError error) {
    auto composed_caption = std::format(
      "{}.{}"
      ,caption
      ,error.caption
    );
    return ParseError{
       composed_caption
      ,error.pos
    };
  } // make_composed_error

  std::string parse_error_to_string(ParseError const& error) {
    return std::format(
      "{} failed at:{}"
      ,error.caption
      ,error.pos
    );
  } // parse_error_to_string

  template <typename value_type>
  using Result = std::expected<Success<value_type>,ParseError>;

  template <typename value_type>
  using Parser = std::function<parsing::Result<value_type>(parsing::Input)>;

  // Parser natural_number() {
  //   using uint8_t_vector = std::vector<uint8_t>;
  //   auto is_ascii_digit = [](uint8_t cp) -> bool {return cp>='0' and cp<='9';};
  //   auto ascii_digits_to_size_t = [](uint8_t_vector const& v) {
  //     size_t result{};
  //     for (auto n : v) result = result*10 + n;
  //     return result;
  //   };
  //   return [is_ascii_digit,ascii_digits_to_size_t](Input input) -> Result {
  //     auto text = input.view();
  //     std::vector<uint8_t> ascii_digits{};
  //     for (size_t ix=0;ix<text.size() and is_ascii_digit(text[ix]);++ix) {
  //       ascii_digits.push_back(static_cast<uint8_t>(text[ix]));
  //     }
  //     if (ascii_digits.size()==0) return std::unexpected(ParseError{});
      
  //     return Success{
  //        NaturalNumber{ascii_digits_to_size_t(ascii_digits)}
  //       ,input.consumed(ascii_digits.size())
  //     };
  //   }; // lambda
  // } // natural_number


  Parser<Text> literal(std::string_view expected) {
    return [expected](Input input) -> Result<Text> {
      auto text = input.view();

      if (!text.starts_with(expected)) {
        return std::unexpected(make_error(
           std::format("literal:'{}'",expected)
          ,input)
        );
      }

      return Success<Text>{
        Text{std::string(expected)},
        input.consumed(expected.size())
      };
    };
  } // literal

  template <typename LHS,typename RHS>
  Parser<Both<LHS,RHS>> both(Parser<LHS> first, Parser<RHS> second) {
    return [first, second](Input input) -> Result<Both<LHS,RHS>> {
        auto r1 = first(input);

        if (!r1) {
          return std::unexpected(
            make_composed_error("both:lhs",r1.error())
          );
        }

        auto [value1, remaining] = r1.value();

        auto r2 = second(remaining);

        if (!r2) {
          return std::unexpected(
            make_composed_error("both:rhs",r2.error())
          );
        }

        auto [value2, remaining2] = *r2;

        // Until structured values, return the second one
        return Success<Both<LHS,RHS>>{
            Both{value1,value2}
            ,remaining2
        };
    }; // lambda
  } // sequence

  // template<class F>
  // Parser map(Parser parser, F transform) {
  //   return [parser, transform](Input input) -> Result {
  //     auto result = parser(input);

  //     if (!result) {
  //       return std::unexpected(result.error());
  //     }

  //     auto [value, remaining] = *result;

  //     return Success{
  //        transform(value)
  //       ,remaining
  //     };
  //   };
  // } // map

  // Parser flat_map(
  //   Parser parser,
  //   std::function<Parser(Value)> next) {

  //     return [parser, next](Input input) -> Result {
  //       auto r1 = parser(input);

  //       if (!r1) {
  //         return std::unexpected(r1.error());
  //       }

  //       auto [value, remaining] = *r1;

  //       auto next_parser = next(value);

  //       return next_parser(remaining);
  //     }; // lambda
  // } // flat_map

  template <typename LHS,typename RHS>
  Parser<Either<LHS,RHS>> either(Parser<LHS> first, Parser<RHS> second) {
    return [first, second](Input input) -> Result<Either<LHS,RHS>> {
      auto r1 = first(input);

      if (r1) {
        auto const& [parsed_value,remaining] = r1.value();
        return Success<Either<LHS,RHS>> {
          Either<LHS,RHS>{parsed_value,std::nullopt}
          ,remaining
        };
      }

      auto r2 = second(input);
      if (r2) {
        auto const& [parsed_value,remaining] = r2.value();
        return Success<Either<LHS,RHS>> {
           Either<LHS,RHS>{std::nullopt,parsed_value}
          ,remaining
        };
      }

      return std::unexpected(
        make_composed_error("either:rhs",r2.error())
      );

    }; // lambda
  } // either

} // parsing

namespace archive {
  // Archive parsing specifics goes here

  // // runtime composable Parser (produces variant Value)
  // parsing::Result persistent_acrhive_entry(parsing::Input input) {
  //   using namespace parsing;
  //   return sequence(
  //      literal("magic_value")
  //     ,sequence(
  //        literal("=")
  //       ,natural_number()
  //     )
  //   )(input);    
  // };

}; // archive

using Input = parsing::Input;

template<typename value_type>
using Parser = parsing::Parser<value_type>;


template<typename value_type>
using Result = parsing::Result<value_type>;

template <typename value_type>
Result<value_type> parse(Parser<value_type> parser,Input input) {
  return parser(input);
} // parse
