#include "walk_pdf.hpp"

#include "document.hpp"

#include <fstream>
#include <iterator>
#include <string_view>

namespace walk_pdf {

namespace {

std::size_t count_of(std::string_view text, std::string_view what) {
  std::size_t count = 0;
  for (auto at = text.find(what); at != std::string_view::npos; at = text.find(what, at + what.size())) {
    ++count;
  }
  return count;
}

} // namespace

std::expected<ExtractedText, TextError> extract_text(std::filesystem::path const& file) {
  std::ifstream stream{file, std::ios::binary};
  if (!stream) {
    return std::unexpected(TextError{TextError::Kind::cannot_read, "the file could not be read"});
  }
  return extract_text(std::string{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}});
}

std::expected<ExtractedText, TextError> extract_text(std::string pdf_bytes) {
  // PDF 32000-1:2008, 7.5.2: the header, which readers look for in the first 1024 bytes
  if (std::string_view{pdf_bytes}.substr(0, 1024).find("%PDF-") == std::string_view::npos) {
    return std::unexpected(TextError{TextError::Kind::not_a_pdf, "the file is not a PDF (no %PDF- header)"});
  }
  Document const document{std::move(pdf_bytes)};

  TextReport report;
  if (auto const& security = document.security()) {
    report.encrypted = true;
    if (!security->key) {
      auto const kind = security->status.ends_with("needs a password") ? TextError::Kind::needs_password
                                                                        : TextError::Kind::unsupported_encryption;
      return std::unexpected(TextError{kind, "the file is encrypted: " + security->status});
    }
    if (!security->allowed(5)) {
      return std::unexpected(TextError{TextError::Kind::not_permitted,
                                       "the file's permissions do not allow copying or extracting its text"});
    }
  }

  auto text = plain_text(document);
  if (!text) {
    return std::unexpected(TextError{TextError::Kind::not_permitted, text.error()}); // checked above; a guard
  }
  report.pages = text->pages.size();
  for (auto const& page : text->pages) {
    for (auto const& line : page) {
      report.unmapped_glyphs += count_of(line, "\xEF\xBF\xBD"); // U+FFFD in UTF-8
    }
  }
  return ExtractedText{std::move(*text), report};
}

} // namespace walk_pdf
