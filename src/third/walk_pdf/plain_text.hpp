#pragma once

#include "document.hpp"

#include <cstddef>
#include <expected>
#include <string>
#include <vector>

namespace walk_pdf {

// A document's text as plain text: each page's lines, top to bottom
struct PlainText {
  std::vector<std::vector<std::string>> pages;

  // The pages' lines, each page after a "--- page n ---" line
  std::string to_string() const;
};

// A piece of text where it is drawn: from one Tj, ' or ", or one string of a TJ
struct TextRun {
  std::string text;  // UTF-8
  double x = 0;      // its start on the page, in default user space (1/72 inch)
  double y = 0;      //   (on the baseline)
  double end_x = 0;  // where the next glyph would start
  double end_y = 0;
  double size = 0;   // the font size on the page
};

// The text runs of a page (counted from 0), in drawing order: the page's
// operations and the forms it draws are interpreted, following the graphics
// state (q, Q, cm) and text state (BT, Tf, Tc, Tw, Tz, TL, Ts, Td, TD, Tm, T*).
// Marked content with /ActualText (BDC ... EMC) is one run of that text,
// in place of the text its glyphs map to.
std::vector<TextRun> text_runs(Document const& document, std::size_t page);

// A page's runs as lines: runs along the same baseline (in the same direction)
// form a line, ordered along it, with a space where the gap between two runs
// is wider than a fifth of the font size. Lines go top to bottom, with an
// empty line where the gap between two lines is wide.
std::vector<std::string> lines_of(std::vector<TextRun> const& runs);

// The document's text, page by page. Refused for an encrypted file whose
// permissions do not allow copying or extracting its text.
std::expected<PlainText, std::string> plain_text(Document const& document);

} // namespace walk_pdf
