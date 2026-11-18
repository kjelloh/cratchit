#pragma once

#include "charset/unicode.hpp"
#include "charset/utf8.hpp"

#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace cratchit::sie {

// The items between `{` and `}` in a record, e.g. `{1 "100" 6 "P1"}`.
// Items are kept as text; pairing dimensions with objects is up to the parser.
struct ObjectList {
  std::vector<std::string> items;

  bool operator==(ObjectList const&) const = default;
};

// One field of a record: text (quotes removed, `\"` made `"`) or an object list.
// Text is UTF-8.
using Field = std::variant<std::string, ObjectList>;

// One meaningful line of an SIE file
struct Line {
  enum class Kind {
    record,      // `#LABEL field field ...`
    block_begin, // a line with only `{` (starts the transactions of a #VER)
    block_end,   // a line with only `}`
  };

  Kind kind = Kind::record;
  std::string label; // without `#`, as written; empty for block lines
  std::vector<Field> fields;
  std::size_t number = 0; // line number in the file, from 1
};

// A line, or error text naming the line number
using Lexed = std::expected<Line, std::string>;

// Takes the Unicode code points of an SIE file one at a time and gives back
// a line once its end is reached. Lines end at LF; CR is ignored. Blank lines
// give nothing. Fields are separated by spaces or tabs, also in object lists.
// A field that starts with `"` is quoted: it runs to the next `"` that is not
// `\"`, and may hold spaces, `{` and `}`. A `\` before anything but `"` is kept.
// A `"` inside an unquoted field is kept as is.
// Errors: a line that is not a record, `{` or `}`; a `#` without a label;
// an unclosed quote or object list; `{` inside an object list; `}` outside one.
class Lexer {
public:
  // A line once the code points pushed so far end one, else nothing
  std::optional<Lexed> push(charset::unicode::CodePoint code_point);

  // Ends the input. A last line without a line feed is given here.
  std::optional<Lexed> finish();

private:
  enum class State {
    line_start,  // before the first non-blank of a line
    label,       // in `#LABEL`
    between,     // between fields
    unquoted,    // in an unquoted field
    quoted,      // in a quoted field
    escape,      // after `\` in a quoted field
    after_quote, // right after a closing quote
    block,       // after a lone `{` or `}`
    failed,      // the line has an error; skip to its end
  };

  std::optional<Lexed> end_line();
  void fail(std::string what);
  void append(charset::unicode::CodePoint code_point);
  void end_field();

  State state_ = State::line_start;
  Line line_;
  std::string text_;      // the field being read, UTF-8
  bool in_list_ = false;  // reading items of an object list
  ObjectList list_;
  std::string error_;
  std::size_t number_ = 1;
  charset::utf8::Encoder to_utf8_;
};

} // namespace cratchit::sie
