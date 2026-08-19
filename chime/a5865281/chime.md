# Consider some bare-bone but expandable C++ parser combinator for name-value paired encoded text files?

## 20260819

So it is time to make the parsers be able to return structured types like pairs or lists or what have you?

* Current design returning a variant of a selection of types up-front can do this.
* That is, a member of the variant can not itself aggregate types of itself.
* And we cant extend the list of mvariants with say std::pair, or std::vector as these must then contain some other type than 'Value' that we have designed to cover all value types we can parse.

So what happens if each parser returns its own parsed type?

* How can we still compose them?
* I mean, the client that composes parsers must be able to hanlde different types returned by each parser it applies for parsing!

Let's shrink down our parser count for now so we can explore what happens when each parser returns its own parsed type without getting compilation errors all over the place.

Interesting! This was surprisingly straight forward and so far painless?

* I started by making Success be parameterised on the parsed value type

```cpp
  template <typename value_type>
  using Success = std::tuple<value_type,Input>;
```
* And the propagated this trhough all code that now needs to also be paramerised for an explicit value_type.

```cpp

template <typename value_type>
using Result = std::expected<Success<value_type>,ParseError>;

template <typename value_type>
using Parser = std::function<parsing::Result<value_type>(parsing::Input)>;

Parser<Text> literal(std::string_view expected) {
  return [expected](Input input) -> Result<Text> {
    auto text = input.view();

    if (!text.starts_with(expected)) {
      return std::unexpected(ParseError{});
    }

    return Success<Text>{
      Text{std::string(expected)},
      input.consumed(expected.size())
    };
  };
} // literal

template <typename value_type>
Result<value_type> parse(Parser<value_type> parser,Input input) {
  return parser(input);
} // parse

```

* But this does not affect the client code AT ALL?!!

```cpp
TEST(ParserTest,parse_literal) {

  auto result = parse(
       parsing::literal("magic_value")
      ,"magic_value=123");

  ASSERT_TRUE(result.has_value());
  auto const& [value,remaining] = result.value();
  EXPECT_EQ(remaining.view().size(),4);

}
```

It seems the compiler can inferr value_type from the passed ``` Parser<value_type> ``` to the parameterised parse() function?

When I present this reasoning to chatGPT I seem to be able to pick out some valuable information.

* Template argument deduction for parse() determines:

  * ``` value_type = Text ```
  * and the instantiated function is effectively:

    ```cpp
    Result<Text> parse(Parser<Text> parser, Input input);
    ```

  * So yes: the client doesn't need to know or spell out Text at all.

## 20260818

I am a little baffled about how hard I have to get my head around parsers and parser combinators.

* There is just something about how they are explained that don't make it click for me?
* I get the core idea of a parser consuming some input and returing the parsed 'thing' and the unconsumed input.
* But then I look at existing designs and always find something that are odd or does not fit?
* Like the Haskell exampel parser combinators?
  * Why is the Parser constrcucted from a parsing function?
  * Is it a Haskell thing that just confuses me?
  * I mean, the Parses seems to itself be a parsing function?
  * So why the extra indirection?
* And when looking at C++ parser combinators I just get overwhelmed by all the syntactig noise?
  * It seems one aspect is wether the parser is combined (assembled) at compile time or at runtime?
  * Maybe I should start with runtime assembled parsers?
  * Then I can just inherit from a base class Parser?
  * And have all parsers expose the same API?
  * Or does that not work bebcause we want the parser to be specialised for the 'thing' it parses?

I have now decided to start off with a basic runtime parser combinator approach!

* Then I avoid having to solve the template-magic to get combinator logic work at compile time.
* I also postpone implementing template-magic until I know what primitive parsers I actually need?
* Because I now seem to understand that what primitives i need comes down to the grammar of the 'language' I design.
  * And maybe I end up needing to tweak the grammar to make parsing easier?
  * I mean, I suppose it is easy to miss if I accidentally design a grammar that does not lend itself to easy parsing?

So what I thing I know about the grammar for now is that it is a name-value pair grammar.

* The separator is '='.
* The first part is in fact a 'path' into othe archive tree of values.
* The second part is the 'value'
* So do I want to support different types of values?
* Yes, that seems conveniant.
  * So I imagine I want a Value to be represented by an std::variant?
  * And I imagine initially at least Value = variant(Number,Text);
  * And later possibly also Date,CurrencyAmount,Account,...
  * Where Currency itself is 'typed' (e,g., pair (currency_descriptor,cents_amount))?

You know F-it! Why do I not just go with an std::function type erased Parser and returns a list of Success?

* Parser is std::function: Input -> pair(Range,Input).
* Yes, we can just return the matched Range?
* And referr further parsing to our Archive domian?

So maybe the first question is what values I have in a persistent Archive file?

* Yes, that seems to be the correct approach?
* The cratchit Archive defines a set of value-types.
* And parsing the file as a persistent archive means it should be able to parse those value types.
* Then the left hand side 'path' is also defined by the Acrhive domain.

Great, we are getting somewhere!

* So we skip the Haskell-inspired Parser for now.

```cpp
#include <concepts>
#include <expected>

struct String{}; // placeholder type for input to parse
struct ParseError{}; // placeholder type for parse error

template <typename a>
using Success = std::pair<a,String>;

template <typename a>
using ExpectedParse = std::expected<Success<a>,ParseError>;

template <typename a, template<typename> typename F>
concept ParserFunction =
    requires (F<a> f, String input) {
        { f(input) } ->
            std::same_as<ExpectedParse<a>>;
};

template <typename a, template<typename> typename F>
requires ParserFunction<a,F>
class Parser {
public:
  Parser(F<a> f) : m_f{f} {}
private:
  F<a> m_f;
}; 
```

* And just seed a runtime parser combinator instead.

```cpp
namespace parser {
  struct Input {}; // place-holder Input type

  struct Number{};
  struct Text{};

  using Value = std::variant<Number,Text>;

  using Success = std::tuple<Value,Input>;

  using Result = std::vector<Success>;

} // parser

using Parser = std::function<parser::Result(parser::Input)>;
```

This may work?

Ok, so I still struggle to get all dimensions of this parser design in its correct 'place'.

* I went with a variant Value but also tried to separate a base parsing framework from the Archive specific one.

```cpp
/**
 * A runtime composable Parser
 * tailored for persistent cratchit Archive parsing
 */
#pragma once

#include <functional> // std::function,
#include <variant>

// Archive values
namespace archive {

  struct Input {}; // place-holder Input type

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

  using Result = std::vector<Success>;

  using Parser = std::function<Result(Input)>;

  using Parser = std::function<parsing::Result(parsing::Input)>;
  Result parse(Parser parser,parsing::Input input);

} // parsing

namespace archive {

  // Archive parsing sepcifics goes here

}; // archive

using Input = parsing::Input;
using Parser = parsing::Parser;
using Result = parsing::Result;

Result parse(Parser parser,Input input);
```

* I am still actually confused if and why I need the extra indirection by the parse() function?
* Maybe that gets clearer when I start compose parsers and try to parse a name-value pair?

I provided this reasoning to chatGPT to get it to riff back some info to me.

* I get inspired by the proposed combinators.

```sh
choice(...)
many(...)
optional(...)
satisfy(...)
```

* I also think I should go back to have the parser result be an expected?

  * For one it is what I know I need for nof (success or some specific error)
  * This also makes the parsing to monadically compse with (to) parse_archive() result?

It seems I should now aim to test to parse say ```magic_value=23´to get a flavour of how parser combinators 'behave'?

* So we need a parser for persistent archive entry?

So I am still stuck! Maybe I should start by designing a google test case and boot strap from there?

* So what is a good name for the soruce code file to test Parser?

  * Perhaps tests/parser_tests.cpp ``` (<some_type_domain>_tests.cpp) ```?
    * Works with tests aimed at types, functions etc.?
  * Or tests/parsing_tests.cpp ``` (<funcionality>_tests.cpp) ```?
    * Works for anything but may require 'functionality name' not seen in tested code?
  * Or tests/Parser_tests.cpp ``` (<type>_tests.cpp) ```?
    * Works only for types?
  * Or tests/TestParser.cpp? ``` (Test<type>).cpp ```?
  * Or tests/test_Parser.cpp? ``` (test_<type>.cpp) ```?

* There is something in me that want to use identifiers from tested code in the test source name?

  * So if we use the naming scheme ``` (<identifier>_tests.cpp) ```
  * We get e.g., tests/parse_archive_tests.cpp (To test parse_arhive() function)
  * And e.g., tests/Parser_tests.cpp (To test Parser type)
  * And e can imagine tests/namespace_archive_tests (namespace acrhive tests)
  * But I am not sure this will scale well and/or leave out making test suites

* Another naming scheme could be to name test-suites?

  * Then I am free to chose suite names in testing domain.
  * And use source code tests/parsing_suite_tests.cpp
  * But then 'suite' kind-of eludes to aggregating existing tests?
  * ANd then those existing tests still needs a naming scheme to live as source code?

Tricky!

Decision: I make folder tests/parsing and file Parser_tests.cpp

* Then the folder structure is kind-of 'domain' and file name kind-of ``` <identifier>_tests.cpp```?

So I went ahead and wrote a test.

```cpp
TEST(ParserTest,parse_name_pair_ok) {

  auto result = parse(
       archive::persistent_acrhive_entry
      ,"magic_value=123");

  EXPECT_EQ(1,result.size());

}
```

* And kept the Parser and parse.

```cpp
using Input = parsing::Input;
using Parser = parsing::Parser;
using Result = parsing::Result;

Result parse(Parser parser,Input input) {
  return parser(input);
} // parse
```

* Also kept Success and Result

```cpp
  using Success = std::tuple<Value,Input>;
  using Result = std::vector<Success>;
```

* I made a quick-and-dirty Input that works only on string_view (char const*)

```cpp
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
```

So there are still many moving parts in this design?

* I imagine to aim an Input being a lazy consumer from std::istream?
* And also consume 'code points' of a defined character set.
* I am also still not sure if this design leads to a good syntax for parser combinators?

But I think this is a good-enough ratchet point for now!

DARN! I keep going off the rails!

I did not like that I could not use the type Parser for anything? 

* I had made Parser an alias for an std::function: Input -> Result
* Fair enough, but that ment I could not use the alias Parser to define new ones.
  * I have to define them as concrete or lamda functions with the required signature.
* So I thought, OK, maybe I make Parser into a class with a call-operator?
  * But then it seems like a taughtology to have it aggregate an std::function?
  * So I ended up with a pure abstract base class.

```cpp
  class Parser {
  public:
    virtual Result operator()(Input const& input) = 0;
  private:
  }; // Parser
```
  * But this is not passable by value!
  * The type-erased parse() function does no longer compile.

```cpp
Result parse(Parser parser,Input input) {
  return parser(input);
} // parse
```

So while Parser as std::function seemed to work, an abstract base class Parser does NOT!

* Now when I thibnk about it, why did this work with std::function?
* Is std::function perhaps move-constructable?
* Or had I not yet defined any parsers with local state?
* Or maybe std::function with local state IS copyable (clone-able)?
* But from a functional immutable aim this seems liek something we do NOT want?

So what the heck? Why is this so hard?

I have now consulted chatGPT and on my prompt it now actually got my inteded runtime parser combinator goal. So I picked out some cimbinators, implemented them and defined tests for them.

* I actually now understands what they do and how combinators look for my choice of Parser.

  * So we can write the combinator 'sequence'.

```cpp
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

```

  * And the test case for first or second parser should succeed.

```cpp
TEST(ParserTest,parse_sequence) {

  auto result = parse(
      parsing::sequence(
        parsing::literal("magic_value")
        ,parsing::literal("=")
      )
      ,"magic_value=123");

  ASSERT_TRUE(result.has_value());
  auto const& [value,remaining] = result.value();
  EXPECT_EQ(remaining.view().size(),3);

}
```

Now 'sequence' reveals a flaw in my current design.

* Parsers like 'sequence' should return a pair of parsed values.
  * Each parser success to a parsed value.
  * So when both pass we have two parsed values in-sequence
  * We should create a pair, a tuple or a list of these values.
  * This is a structured value!

But I currently have a type erased Value (and std::variant)

* And std::variant cant contain recrsive values of itself!

Ok, Good! Now I understand this interface of the parser!

## 20260817

So I am still intrigued to go for some parser combinator implementation for cratchit.

* Initially for parsing a name-value pair text file for persistent runtime data.

* I think I want to be inspired by computerphile [Functional Parsing - Computerphile](https://youtu.be/dDtZLm7HIJs)
  * See Ref: Professor Hutton's Functional Parsing Library
* And also take a look at [Petter Holmberg: Functional parsing in C++20](https://youtu.be/5iXKLwoqbyw)

So what seed can I plant to get going?

* Consider the Haskell function type for a parser?

```sh
newtype Parser a = P (String -> [(a,String)])
```

  * The confusing part is 'P'?
  * According to chatGPT this reads as 'P aggregates a function?
  * As in C++ a type that holds a function.

```cpp
template <typename A>
class Parser {
    std::function<std::vector<std::pair<A, std::string>>(std::string)> function;

public:
    Parser(std::function<std::vector<std::pair<A, std::string>>(std::string)> f)
        : function(f) {}
};
```

  * AHA! I think I got it now?
  * In Haskell everyting is free functions.
  * So the 'newtype' defines bot the type AND the constructor for new values of that type.
  * So chatGPT suggests some terms to define the Haskell syntax and form for a new type.

```sh
newtype TypeName typeParameter = ConstructorName (FieldType typeParameter)
``` 

  * So for C++ this means if we model 'TypeName' as a class then 'ConstructorName' becomes the class constructor?

```cpp
// C++ representation of Haskell 'newtype TypeName typeParameter = ConstructorName (FieldType typeParameter)'
template <typename typeParameter, template<typename> typename FieldType>
class TypeName {
public:
  // Haskell ConstructorName with type parameter F
  TypeName(FieldType<typeParameter> arg) : m_p{f} {}
private:
  FieldType<typeParameter> m_arg;
}
```

  * Now chatGPT did not fully agree on this?
  * It did not think FieldType was in turn paremeterised.
  * But is seems it thought FieldType could be parametersied on some othe type than TypeName?
  * But if we mean that left hand side means 'some typeParameter'.
  * And then mean that right hand side means 'same typeParameter'.
  * Tnhen I think I may have goptten the gist of it?

Anyhow, I think I can read the Haskell code now?

* 'Parser' is parameterised on some type 'a'.
* 'Parser' is constructed by a function 'P'
* The function 'P' takes a function from a 'String' to a list of pairs of a String and an a.
* Where 'a' is some type but the same type on both sides.
* So in C++ I may now write this as:

```cpp
// C++ representation of Haskell 'newtype TypeName typeParameter = ConstructorName (FieldType typeParameter)'
template <typename a, template<typename> typename F>
class TypeName {
public:
  // Haskell ConstructorName with type parameter F
  TypeName(F<a> f) : m_f{f} {}
private:
  F<a> m_f;
}
```

* Well, OK, Maybe not precise enough?

  * I like chatGPT explaing the Haskell syntax.

  ```text
  For a given type a, Parser a is a type whose value is constructed by P, and P takes a function of type String -> [(a,String)]
  ```

  * So to express this in C++ code we would need a way to restrict F to be a function 'String -> [a,String]'
  * I got this proposal.

```cpp
struct String{}; // placeholder type for input to parse
struct ParseError{}; // placeholder type for parse error

template <typename a>
using Success = std::pair<a,String>;

template <typename a>
using ExpectedParse = std::expected<Success<a>,ParseError>;

template <typename a, template<typename> typename F>
concept ParserFunction =
    requires (F<a> f, String input) {
        { f(input) } ->
            std::same_as<ExpectedParse<a>>;
};

template <typename a, template<typename> typename F>
requires ParserFunction<a,F>
class Parser {
public:
  Parser(F<a> f) : m_f{f} {}
private:
  F<a> m_f;
}; 
```

  * But now I wonder if we ar getting over our heads to mimic the HAskell code?
  * I Haskell the type Parser and the cinstructo P are separatly defined.
  * But in C++ we have the option of Parser aggregating a parsing function or being one? 
  * If we look at the Haskell 'parse' function.

```sh
parse :: Parser a -> String -> [(a,String)]
parse (P p) inp = p inp
```
  * We see that 'p' is the result of constructor 'P'
  * So 'p' IS a Parser.
  * Then 'parse' is just applying the parser to the input.
  * Now in C++ terms this means we can 'just' make Parser provide an operator()(String)?
  * And return the parse result.

## Ref: Professor Hutton's Functional Parsing Library

* I suppose it could prove valuable to get the gist of [Professor Hutton's Functional Parsing Library](http://bit.ly/C_FunctParsLib)?

```sh

-- Functional parsing library from chapter 13 of Programming in Haskell,
-- Graham Hutton, Cambridge University Press, 2016.

module Parsing (module Parsing, module Control.Applicative) where

import Control.Applicative
import Data.Char

-- Basic definitions

newtype Parser a = P (String -> [(a,String)])

parse :: Parser a -> String -> [(a,String)]
parse (P p) inp = p inp

item :: Parser Char
item = P (\inp -> case inp of
                     []     -> []
                     (x:xs) -> [(x,xs)])

-- Sequencing parsers

instance Functor Parser where
   -- fmap :: (a -> b) -> Parser a -> Parser b
   fmap g p = P (\inp -> case parse p inp of
                            []        -> []
                            [(v,out)] -> [(g v, out)])

instance Applicative Parser where
   -- pure :: a -> Parser a
   pure v = P (\inp -> [(v,inp)])

   -- <*> :: Parser (a -> b) -> Parser a -> Parser b
   pg <*> px = P (\inp -> case parse pg inp of
                             []        -> []
                             [(g,out)] -> parse (fmap g px) out)

instance Monad Parser where
   -- (>>=) :: Parser a -> (a -> Parser b) -> Parser b
   p >>= f = P (\inp -> case parse p inp of
                           []        -> []
                           [(v,out)] -> parse (f v) out)

-- Making choices

instance Alternative Parser where
   -- empty :: Parser a
   empty = P (\inp -> [])

   -- (<|>) :: Parser a -> Parser a -> Parser a
   p <|> q = P (\inp -> case parse p inp of
                           []        -> parse q inp
                           [(v,out)] -> [(v,out)])

-- Derived primitives

sat :: (Char -> Bool) -> Parser Char
sat p = do x <- item
           if p x then return x else empty

digit :: Parser Char
digit = sat isDigit

lower :: Parser Char
lower = sat isLower

upper :: Parser Char
upper = sat isUpper

letter :: Parser Char
letter = sat isAlpha

alphanum :: Parser Char
alphanum = sat isAlphaNum

char :: Char -> Parser Char
char x = sat (== x)

string :: String -> Parser String
string []     = return []
string (x:xs) = do char x
                   string xs
                   return (x:xs)

ident :: Parser String
ident = do x  <- lower
           xs <- many alphanum
           return (x:xs)

nat :: Parser Int
nat = do xs <- some digit
         return (read xs)

int :: Parser Int
int = do char '-'
         n <- nat
         return (-n)
       <|> nat

-- Handling spacing

space :: Parser ()
space = do many (sat isSpace)
           return ()

token :: Parser a -> Parser a
token p = do space
             v <- p
             space
             return v

identifier :: Parser String
identifier = token ident

natural :: Parser Int
natural = token nat

integer :: Parser Int
integer = token int

symbol :: String -> Parser String
symbol xs = token (string xs)

```

