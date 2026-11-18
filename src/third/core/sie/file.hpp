#pragma once

#include "charset/unicode.hpp"
#include "core/sie/lexer.hpp"
#include "core/terminal_encoding.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace cratchit::sie {

// True for a character expected in the text of an SIE file: tab, CR, LF,
// printable ASCII and the Latin-1 letters and signs (U+00A0-U+00FF, e.g. å ä ö
// é ü ß ° µ). CP437's box drawing, block, Greek and math characters are not,
// nor are other control characters. Finding one when reading CP437 means the
// file is likely in another encoding (UTF-8 å is CP437 "├Ñ", Windows-1252 å is "σ").
bool is_expected_text(charset::unicode::CodePoint code_point);

// The lines of an SIE file and how its bytes were read
struct LexedFile {
  Charset charset = Charset::Cp437; // the encoding applied
  std::string alert;                // for the user when not read as CP437, else empty
  std::vector<Lexed> lines;         // every line, or its error
};

// Lexes the bytes of an SIE file. They are read as CP437 (PC8), as SIE
// requires. When that gives a character that is not expected text and the
// bytes are UTF-8 (see charset::detect::is_utf8), they are read as UTF-8 and
// the alert says why; a leading UTF-8 byte order mark is skipped. Error text
// when neither fits. Does not log.
std::expected<LexedFile, std::string> lex_bytes(std::span<std::uint8_t const> bytes);

// Reads and lexes an SIE file, see lex_bytes. Logs the encoding applied and
// any alert (business), or an unknown encoding (design insufficiency).
// The alert and error text name the file.
std::expected<LexedFile, std::string> lex_file(std::filesystem::path const& file);

} // namespace cratchit::sie
