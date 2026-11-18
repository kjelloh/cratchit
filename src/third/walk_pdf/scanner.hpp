#pragma once

#include "path.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace walk_pdf {

// A top-level element found in the bytes of a revision
struct Found {
  Kind kind;
  std::size_t begin;            // offset of its first byte
  std::size_t end;              // offset just after its last byte
  std::uint64_t number = 0;     // objects: object number
  std::uint32_t generation = 0; // objects: generation
};

// The next top-level element at or after pos, before end, or nullopt if only
// whitespace is left. A %PDF- line counts as the header only when the element
// starts at header_at (the start of the file); elsewhere it is a comment.
// Bytes that are not recognised are gathered into one unknown element up to
// the next element that is.
std::optional<Found> scan_element(std::string_view bytes, std::size_t pos, std::size_t end,
                                  std::size_t header_at);

} // namespace walk_pdf
