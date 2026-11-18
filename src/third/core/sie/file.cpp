#include "core/sie/file.hpp"

#include "charset/cp437.hpp"
#include "charset/detect.hpp"
#include "charset/utf8.hpp"
#include "core/log.hpp"

#include <cstddef>
#include <format>
#include <fstream>
#include <iterator>
#include <optional>
#include <utility>
#include <vector>

namespace cratchit::sie {

namespace {

// The first character read as CP437 that is not expected text
struct Unexpected {
  charset::unicode::CodePoint code_point = 0;
  std::size_t line = 0;
};

std::optional<Unexpected> find_unexpected_cp437(std::span<std::uint8_t const> bytes) {
  charset::cp437::Decoder decoder;
  std::size_t line = 1;
  for (auto const byte : bytes) {
    auto const code_point = decoder.push(byte);
    if (!code_point) {
      continue;
    }
    if (!is_expected_text(*code_point)) {
      return Unexpected{*code_point, line};
    }
    if (*code_point == U'\n') {
      ++line;
    }
  }
  return std::nullopt;
}

// `'├' (U+251C) on line 3`, the character itself as UTF-8 unless a control character
std::string describe(Unexpected const& unexpected) {
  std::string shown;
  if (unexpected.code_point >= 0x20 && unexpected.code_point != 0x7F) {
    charset::utf8::Encoder to_utf8;
    shown = "'";
    for (auto const byte : to_utf8.push(unexpected.code_point)) {
      shown.push_back(static_cast<char>(byte));
    }
    shown += "' ";
  }
  return std::format("{}(U+{:04X}) on line {}", shown, static_cast<std::uint32_t>(unexpected.code_point),
                     unexpected.line);
}

// Every line from pushing the bytes through a decoder and the lexer
template <typename Decoder>
std::vector<Lexed> lex_with(Decoder decoder, std::span<std::uint8_t const> bytes) {
  Lexer lexer;
  std::vector<Lexed> lines;
  for (auto const byte : bytes) {
    if (auto const code_point = decoder.push(byte)) {
      if (auto line = lexer.push(*code_point)) {
        lines.push_back(std::move(*line));
      }
    }
  }
  if constexpr (requires { decoder.finish(); }) {
    if (auto const code_point = decoder.finish()) {
      if (auto line = lexer.push(*code_point)) {
        lines.push_back(std::move(*line));
      }
    }
  }
  if (auto line = lexer.finish()) {
    lines.push_back(std::move(*line));
  }
  return lines;
}

} // namespace

bool is_expected_text(charset::unicode::CodePoint code_point) {
  return code_point == U'\t' || code_point == U'\n' || code_point == U'\r' ||
         (code_point >= 0x20 && code_point <= 0x7E) || (code_point >= 0xA0 && code_point <= 0xFF);
}

std::expected<LexedFile, std::string> lex_bytes(std::span<std::uint8_t const> bytes) {
  auto const unexpected = find_unexpected_cp437(bytes);
  if (!unexpected) {
    return LexedFile{Charset::Cp437, {}, lex_with(charset::cp437::Decoder{}, bytes)};
  }
  if (charset::detect::is_utf8(bytes)) {
    // A byte order mark (U+FEFF) only marks the file as UTF-8; it is not text
    auto const text = bytes.size() >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF
                          ? bytes.subspan(3)
                          : bytes;
    return LexedFile{Charset::Utf8,
                     std::format("Not in PC8 (CP437) as SIE requires: read as CP437 it has {}. Read as UTF-8 instead.",
                                 describe(*unexpected)),
                     lex_with(charset::utf8::Decoder{}, text)};
  }
  return std::unexpected(
      std::format("Unknown encoding: not PC8 (CP437) as SIE requires (read as CP437 it has {}) and not UTF-8",
                  describe(*unexpected)));
}

std::expected<LexedFile, std::string> lex_file(std::filesystem::path const& file) {
  std::ifstream in(file, std::ios::binary);
  if (!in) {
    return std::unexpected(std::format("Cannot read {}", file.string()));
  }
  std::vector<std::uint8_t> const bytes{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
  auto result = lex_bytes(bytes);
  if (!result) {
    LOG_DESIGN_INSUFFICIENCY("SIE file {}: {}", file.string(), result.error());
    return std::unexpected(std::format("{}: {}", file.string(), result.error()));
  }
  LOG_BUSINESS("Read SIE file {} as {}", file.string(), charset_name(result->charset));
  if (!result->alert.empty()) {
    result->alert = std::format("{}: {}", file.string(), result->alert);
    LOG_BUSINESS("{}", result->alert);
  }
  return result;
}

} // namespace cratchit::sie
