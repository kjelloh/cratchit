#pragma once

#include "path.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace walk_pdf {

// A value inside an object (PDF 32000-1:2008, 7.3 Objects), as a byte range in the file.
// Its kind is null, boolean, integer, real, name, string, array, dictionary,
// reference or unknown (a token that is none of these), or stream for a
// stream's data.
struct Value {
  Kind kind;
  std::size_t begin; // offset of its first byte
  std::size_t end;   // offset just after its last byte
};

// The value that starts at pos, after any whitespace and comments, and ends
// before end. nullopt if only whitespace and comments are left, or the next
// byte ends a container ("]" or ">>").
std::optional<Value> value_at(std::string_view bytes, std::size_t pos, std::size_t end);

// One child of a dictionary or array: an entry (key and value) or an item (value)
struct Child {
  std::string key; // a dictionary entry's key, decoded (empty for array items)
  Value value;
};

// Where a dictionary's or array's children start
std::size_t first_child_at(Value const& container);

// The container's next child at or after pos, or nullopt after the last one.
// The next child starts at the returned value's end.
std::optional<Child> next_child(std::string_view bytes, Value const& container, std::size_t pos);

// The data of a stream whose dictionary ends at pos: the bytes between the
// EOL after "stream" and the EOL before "endstream". nullopt if no "stream"
// keyword follows the dictionary.
std::optional<Value> stream_data(std::string_view bytes, std::size_t pos, std::size_t end);

// One operation in a content stream (PDF 32000-1:2008, 7.8.2): its operands,
// then its operator, e.g. "/F8 16 Tf" or "<0001> Tj". An inline image
// ("BI ... ID data EI") is one operation, named "BI", with no operands.
struct Operation {
  std::size_t begin;          // its first operand, or its operator if it has none
  std::size_t operator_begin; // its operator
  std::size_t end;            // just after its operator ("EI" for an inline image)
  std::string_view name;      // the operator, e.g. "Tj"
};

// The next operation at or after pos, before end, or nullopt if none is left
std::optional<Operation> next_operation(std::string_view bytes, std::size_t pos, std::size_t end);

// A name's bytes, without its '/', with #xx escapes decoded: "/A#20B" is "A B"
std::string decode_name(std::string_view name);

} // namespace walk_pdf
