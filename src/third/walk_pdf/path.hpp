#pragma once

#include <compare>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace walk_pdf {

// The kinds of element the walker knows about
enum class Kind {
  document,  // the whole file (the root, path "/")
  revision,  // the original file or one appended update, up to and including its %%EOF
  header,    // %PDF-x.y at the start of the file
  comment,   // % ... up to end of line
  object,    // n g obj ... endobj
  xref,      // xref ... (classic cross-reference table)
  trailer,   // trailer << ... >>
  startxref, // startxref n
  eof,       // %%EOF
  unknown,   // bytes the walker could not make sense of

  // Inside objects (PDF 32000-1:2008, 7.3 Objects)
  null,       // null
  boolean,    // true or false
  integer,    // 12, -3
  real,       // 3.14, -.5
  name,       // /Type
  string,     // (text) or <hex>
  array,      // [ ... ]
  dictionary, // << /Key value ... >>
  reference,  // 12 0 R, a reference to object 12
  stream,     // the data of a stream object, between "stream" and "endstream"
  decoded,    // a stream's data decrypted and with its filters applied (not in the file)

  // Inside a content stream (decoded page drawing instructions)
  text_block, // BT ... ET, a text object
  operation,  // operands and an operator, e.g. "<0001> Tj"

  // Display order (not in the file's layout): the pages as shown
  pages, // all pages, in page tree order
  page,  // one page; its children are its drawing operations in order

  // Only in path segments: how a value is reached from its parent
  key,   // a dictionary entry, by its key
  index, // an array item, by its index
};

inline constexpr std::size_t kind_count = static_cast<std::size_t>(Kind::index) + 1;

std::string_view to_string(Kind kind);

// One step in a path: which child of its parent an element is.
// An object is named by its object number, the header and a stream's data by
// their kind alone, a dictionary entry by its key, an array item by its index,
// and anything else by its ordinal among siblings of the same kind. An object
// number occurs at most once per revision, so the generation is not needed.
struct Segment {
  Kind kind = Kind::document;
  std::uint64_t number = 0; // object number, array index, or ordinal
  std::string key;          // dictionary key, as decoded bytes (Kind::key only)

  static Segment revision(std::uint64_t ordinal) { return {Kind::revision, ordinal, {}}; }
  static Segment header() { return {Kind::header, 0, {}}; }
  static Segment object(std::uint64_t number) { return {Kind::object, number, {}}; }
  static Segment ordinal(Kind kind, std::uint64_t ordinal) { return {kind, ordinal, {}}; }
  static Segment entry(std::string key) { return {Kind::key, 0, std::move(key)}; }
  static Segment item(std::uint64_t index) { return {Kind::index, index, {}}; }
  static Segment stream() { return {Kind::stream, 0, {}}; }
  static Segment decoded() { return {Kind::decoded, 0, {}}; }
  static Segment pages() { return {Kind::pages, 0, {}}; }

  // "rev[1]", "header", "obj[12]", "xref[0]", "Type" (a key), "[3]" (an index), "stream", "decoded".
  // A key is written as is, except that bytes which would clash with the path
  // syntax are written #xx, as PDF names do: "A B" is "A#20B". A key that is
  // a reserved word ("header", "stream", "decoded", "pages") gets its first letter escaped.
  std::string to_string() const;

  auto operator<=>(Segment const&) const = default;
};

// Where an element is in the PDF, as a pure value: the segments from the root down.
// It holds no pointers or offsets; the Document looks up what a path refers to.
// The root (the whole document) is the empty path, "/".
class Path {
public:
  Path() = default;
  explicit Path(std::vector<Segment> segments) : segments_(std::move(segments)) {}

  std::vector<Segment> const& segments() const { return segments_; }
  bool is_root() const { return segments_.empty(); }
  Path child(Segment segment) const;
  Path parent() const;

  // "/rev[0]/obj[12]/Resources/Font"; the root is "/". Path strings contain no blanks.
  std::string to_string() const;

  // Reads the string form back; parse(p.to_string()) == p for every path p
  static std::expected<Path, std::string> parse(std::string_view text);

  auto operator<=>(Path const&) const = default;

private:
  std::vector<Segment> segments_;
};

} // namespace walk_pdf
