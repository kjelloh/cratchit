#include "scanner.hpp"

#include "bytes.hpp"

#include <utility>

namespace walk_pdf {

namespace {

// "n g obj" at pos: the object number, generation and position after "obj"
struct ObjectStart {
  std::uint64_t number;
  std::uint32_t generation;
  std::size_t body;
};

std::optional<ObjectStart> object_start_at(Bytes const& bytes, std::size_t pos) {
  if (!is_digit(bytes.at(pos))) {
    return std::nullopt;
  }
  auto const number = bytes.integer_at<std::uint64_t>(pos);
  if (!number) {
    return std::nullopt;
  }
  auto const generation = bytes.integer_at<std::uint32_t>(bytes.skip_whitespace(number->second));
  if (!generation) {
    return std::nullopt;
  }
  auto const obj = bytes.skip_whitespace(generation->second);
  if (!bytes.keyword_at(obj, "obj")) {
    return std::nullopt;
  }
  return ObjectStart{number->first, generation->first, obj + 3};
}

// Position after "endobj" of the object whose body starts at pos.
// A stream's data may hold any bytes, so skip it via "endstream".
// Not yet handled: "endobj" or "stream" inside a string in the object.
// Only look for "stream" before the next "endobj", or most objects would search
// on through the rest of the file.
std::size_t end_of_object(Bytes const& bytes, std::size_t pos) {
  while (true) {
    auto const endobj = bytes.find_keyword(pos, "endobj");
    auto const stream = bytes.find_keyword(pos, "stream", endobj.value_or(bytes.size()));
    if (stream) {
      auto const endstream = bytes.find_keyword(*stream + 6, "endstream");
      if (!endstream) {
        return bytes.size(); // Damaged file: the object runs to the end
      }
      pos = *endstream + 9;
      continue;
    }
    return endobj ? *endobj + 6 : bytes.size();
  }
}

// Position after the ">>" that closes the dictionary starting at pos ("<<").
// Not yet handled: "<<" or ">>" inside a string in the dictionary.
std::size_t end_of_dictionary(Bytes const& bytes, std::size_t pos) {
  int depth = 0;
  while (pos + 1 < bytes.size()) {
    if (bytes.at(pos) == '<' && bytes.at(pos + 1) == '<') {
      ++depth;
      pos += 2;
    } else if (bytes.at(pos) == '>' && bytes.at(pos + 1) == '>') {
      --depth;
      pos += 2;
      if (depth == 0) {
        return pos;
      }
    } else {
      ++pos;
    }
  }
  return bytes.size();
}

// A recognised element starting exactly at from, or nullopt
std::optional<Found> element_at(Bytes const& bytes, std::size_t from, std::size_t header_at) {
  auto const c = bytes.at(from);
  if (c == '%') {
    auto const to = bytes.end_of_text(from);
    auto const text = bytes.text(from, to);
    if (from == header_at && text.starts_with("%PDF-")) {
      return Found{Kind::header, from, to};
    }
    if (text.starts_with("%%EOF")) {
      return Found{Kind::eof, from, to};
    }
    return Found{Kind::comment, from, to};
  }
  if (auto const object = object_start_at(bytes, from)) {
    return Found{Kind::object, from, end_of_object(bytes, object->body), object->number,
                 object->generation};
  }
  if (bytes.keyword_at(from, "xref")) {
    // The table runs until its trailer
    return Found{Kind::xref, from, bytes.find_keyword(from + 4, "trailer").value_or(bytes.size())};
  }
  if (bytes.keyword_at(from, "trailer")) {
    return Found{Kind::trailer, from, end_of_dictionary(bytes, bytes.skip_whitespace(from + 7))};
  }
  if (bytes.keyword_at(from, "startxref")) {
    auto to = from + 9;
    if (auto const offset = bytes.integer_at<std::uint64_t>(bytes.skip_whitespace(to))) {
      to = offset->second;
    }
    return Found{Kind::startxref, from, to};
  }
  return std::nullopt;
}

} // namespace

std::optional<Found> scan_element(std::string_view bytes, std::size_t pos, std::size_t end,
                                  std::size_t header_at) {
  Bytes const limited{bytes.substr(0, end)};
  auto const from = limited.skip_whitespace(pos);
  if (from >= limited.size()) {
    return std::nullopt;
  }
  if (auto const found = element_at(limited, from, header_at)) {
    return found;
  }
  // Gather unrecognised lines until a line starts with something recognised
  auto to = limited.end_of_line(from);
  while (true) {
    auto const next = limited.skip_whitespace(to);
    if (next >= limited.size() || element_at(limited, next, header_at)) {
      break;
    }
    to = limited.end_of_line(next);
  }
  return Found{Kind::unknown, from, to};
}

} // namespace walk_pdf
