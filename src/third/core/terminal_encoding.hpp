#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

// A character set cratchit can name. Only some of them can be transcoded yet;
// the rest are known so that the platform's choice can be reported by name.
enum class Charset {
  Unknown,
  Ascii,
  Utf8,
  Cp437,
  Cp850,
  Cp1252,
  Iso8859_1,
  Iso8859_15,
  Gbk,
  Gb18030,
  Big5,
  ShiftJis,
  EucJp,
  EucKr,
};

// The usual name of a charset, e.g. "UTF-8" or "CP437"
std::string_view charset_name(Charset charset);

// The charset of a POSIX codeset name as given by nl_langinfo(CODESET), e.g.
// "UTF-8" or "ISO-8859-1". Case, '-', '_' and spaces do not matter.
Charset charset_from_name(std::string_view name);

// The charset of a Windows code page number, e.g. 65001 (UTF-8) or 850
Charset charset_from_code_page(unsigned code_page);

// One encoding as the platform reported it, e.g. "code page 850" or "ANSI_X3.4-1968"
struct Reported {
  std::string as_reported;
  Charset charset = Charset::Unknown;
};

// What the platform says the terminal reads and writes
struct ReportedTerminal {
  Reported in;
  Reported out;
};

// The encodings cratchit uses towards the terminal. Inside, cratchit works in
// Unicode code points; these are only for the bytes read from and written to
// the terminal.
struct TerminalEncoding {
  Charset in = Charset::Utf8;
  Charset out = Charset::Utf8;
  std::vector<std::string> warnings; // one per encoding that fell back to UTF-8
};

// True when cratchit can transcode terminal input in this charset to Unicode.
// UTF-8 always counts: it is the fallback.
bool can_read(Charset charset);

// True when cratchit can transcode Unicode to terminal output in this charset
bool can_write(Charset charset);

// The reported encodings when cratchit can transcode them, else UTF-8 with a warning
TerminalEncoding choose_terminal_encoding(ReportedTerminal const& reported);

// Asks the platform which encodings the terminal uses.
// macOS and Android: UTF-8, known when built. Windows: the console input and
// output code pages, or the ANSI code page without a console. Linux and other
// POSIX: the codeset of the user's locale (LC_ALL, LC_CTYPE, LANG).
ReportedTerminal report_terminal_encoding();

} // namespace cratchit
