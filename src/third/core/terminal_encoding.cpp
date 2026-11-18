#include "core/terminal_encoding.hpp"

#include <array>
#include <format>
#include <utility>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif !defined(__APPLE__) && !defined(__ANDROID__)
#include <clocale>
#include <langinfo.h>
#endif

namespace cratchit {

namespace {

// Codeset names without case, '-', '_' or spaces, as they come from different libcs
constexpr std::array<std::pair<std::string_view, Charset>, 22> names{{
    {"UTF8", Charset::Utf8},
    {"ANSIX3.41968", Charset::Ascii}, // glibc's name for ASCII, e.g. in the C locale
    {"USASCII", Charset::Ascii},
    {"ASCII", Charset::Ascii},
    {"646", Charset::Ascii},
    {"CP437", Charset::Cp437},
    {"IBM437", Charset::Cp437},
    {"CP850", Charset::Cp850},
    {"IBM850", Charset::Cp850},
    {"CP1252", Charset::Cp1252},
    {"WINDOWS1252", Charset::Cp1252},
    {"ISO88591", Charset::Iso8859_1},
    {"LATIN1", Charset::Iso8859_1},
    {"ISO885915", Charset::Iso8859_15},
    {"GBK", Charset::Gbk},
    {"GB2312", Charset::Gbk}, // GBK is a superset
    {"GB18030", Charset::Gb18030},
    {"BIG5", Charset::Big5},
    {"SHIFTJIS", Charset::ShiftJis},
    {"SJIS", Charset::ShiftJis},
    {"EUCJP", Charset::EucJp},
    {"EUCKR", Charset::EucKr},
}};

constexpr std::array<std::pair<unsigned, Charset>, 13> code_pages{{
    {65001, Charset::Utf8},
    {20127, Charset::Ascii},
    {437, Charset::Cp437},
    {850, Charset::Cp850},
    {1252, Charset::Cp1252},
    {28591, Charset::Iso8859_1},
    {28605, Charset::Iso8859_15},
    {936, Charset::Gbk},
    {54936, Charset::Gb18030},
    {950, Charset::Big5},
    {932, Charset::ShiftJis},
    {51932, Charset::EucJp},
    {949, Charset::EucKr}, // Windows' extension of EUC-KR
}};

std::string squeezed(std::string_view name) {
  std::string result;
  for (char c : name) {
    if (c == '-' || c == '_' || c == ' ') {
      continue;
    }
    result.push_back(c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c);
  }
  return result;
}

// The charset to use in one direction, adding a warning when it falls back to UTF-8
Charset chosen(Reported const& reported, bool usable, std::string_view direction, std::vector<std::string>& warnings) {
  if (usable) {
    return reported.charset;
  }
  if (reported.charset == Charset::Unknown) {
    warnings.push_back(std::format("Varning: cratchit känner inte igen terminalens teckenkodning för {}, {}. cratchit använder UTF-8.", direction,
                                   reported.as_reported));
  } else {
    warnings.push_back(std::format("Varning: cratchit stöder inte terminalens teckenkodning för {}, {} ({}), än. cratchit använder UTF-8.",
                                   direction, reported.as_reported, charset_name(reported.charset)));
  }
  return Charset::Utf8;
}

#ifdef _WIN32
Reported from_code_page(unsigned code_page) {
  return {std::format("code page {}", code_page), charset_from_code_page(code_page)};
}
#endif

} // namespace

std::string_view charset_name(Charset charset) {
  switch (charset) {
  case Charset::Unknown: return "unknown";
  case Charset::Ascii: return "ASCII";
  case Charset::Utf8: return "UTF-8";
  case Charset::Cp437: return "CP437";
  case Charset::Cp850: return "CP850";
  case Charset::Cp1252: return "Windows-1252";
  case Charset::Iso8859_1: return "ISO-8859-1";
  case Charset::Iso8859_15: return "ISO-8859-15";
  case Charset::Gbk: return "GBK";
  case Charset::Gb18030: return "GB18030";
  case Charset::Big5: return "Big5";
  case Charset::ShiftJis: return "Shift_JIS";
  case Charset::EucJp: return "EUC-JP";
  case Charset::EucKr: return "EUC-KR";
  }
  return "unknown";
}

Charset charset_from_name(std::string_view name) {
  auto const key = squeezed(name);
  for (auto const& [known, charset] : names) {
    if (key == known) {
      return charset;
    }
  }
  return Charset::Unknown;
}

Charset charset_from_code_page(unsigned code_page) {
  for (auto const& [known, charset] : code_pages) {
    if (code_page == known) {
      return charset;
    }
  }
  return Charset::Unknown;
}

bool can_read(Charset charset) {
  return charset == Charset::Utf8 || charset == Charset::Cp437;
}

bool can_write(Charset charset) {
  return charset == Charset::Utf8;
}

TerminalEncoding choose_terminal_encoding(ReportedTerminal const& reported) {
  TerminalEncoding result;
  result.in = chosen(reported.in, can_read(reported.in.charset), "inmatning", result.warnings);
  result.out = chosen(reported.out, can_write(reported.out.charset), "utmatning", result.warnings);
  return result;
}

ReportedTerminal report_terminal_encoding() {
#if defined(_WIN32)
  // A process without a console gets 0; its bytes then follow the ANSI code page
  auto const in = GetConsoleCP();
  auto const out = GetConsoleOutputCP();
  return {from_code_page(in != 0 ? in : GetACP()), from_code_page(out != 0 ? out : GetACP())};
#elif defined(__APPLE__) || defined(__ANDROID__)
  // Terminals here are UTF-8, also when no locale is set (which would say ASCII)
  Reported const utf8{"UTF-8", Charset::Utf8};
  return {utf8, utf8};
#else
  // Ask for the user's locale, then put the program's back as it was
  std::string const saved = std::setlocale(LC_CTYPE, nullptr);
  std::setlocale(LC_CTYPE, "");
  std::string const codeset = nl_langinfo(CODESET);
  std::setlocale(LC_CTYPE, saved.c_str());
  Reported const reported{codeset, charset_from_name(codeset)};
  return {reported, reported};
#endif
}

} // namespace cratchit
