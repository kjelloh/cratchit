#pragma once

// The walk_pdf API: a PDF's text as plain text, for programs that process it
// further (e.g. a bookkeeping application reading vouchers). See API.md.

#include "plain_text.hpp"

#include <cstddef>
#include <expected>
#include <filesystem>
#include <string>

namespace walk_pdf {

// Why there is no text
struct TextError {
  enum class Kind {
    cannot_read,            // the file could not be read
    not_a_pdf,              // no "%PDF-" header in the first 1024 bytes
    needs_password,         // encrypted, and the empty password does not open it
    unsupported_encryption, // encrypted in a way walk_pdf can not decrypt (e.g. AES)
    not_permitted,          // its permissions do not allow copying or extracting its text
  };
  Kind kind;
  std::string message; // for people, in English
};

// What to know before trusting the text
struct TextReport {
  std::size_t pages = 0;           // pages found; 0 when the file has no page tree walk_pdf can find
  std::size_t unmapped_glyphs = 0; // characters that could not be mapped, shown as U+FFFD
  bool encrypted = false;          // the file is encrypted (and walk_pdf decrypted it)
};

struct ExtractedText {
  PlainText text; // pages of lines, UTF-8
  TextReport report;
};

// The text of the PDF file, page by page and line by line
std::expected<ExtractedText, TextError> extract_text(std::filesystem::path const& file);

// The text of a PDF held in memory, e.g. an e-mail attachment
std::expected<ExtractedText, TextError> extract_text(std::string pdf_bytes);

} // namespace walk_pdf
