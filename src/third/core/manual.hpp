#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

// The name of the user manual in the framework folder (<program folder>/.cratchit/)
inline constexpr std::string_view manual_file_name = "anvandarmanual.md";

// The user manual (docs/anvandarmanual.md, Markdown in Swedish), read for its
// help texts. A help text starts after a line of its own
// `<!-- hjälp: <topic> -->` and ends at the next comment (usually
// `<!-- /hjälp -->`), the next heading or the end of the manual. Comments are
// invisible when the Markdown is shown. A help text may hold several
// paragraphs and lists; keep it within one screen.
class Manual {
public:
  explicit Manual(std::string text) : text_(std::move(text)) {}

  // The help text of topic as plain text lines for the terminal: no `**`,
  // `*` or backticks, a link shows only its text, and the ``` lines of a code
  // block are left out. Blank lines between paragraphs are kept as empty
  // lines. Empty when the manual has no help text for topic.
  std::vector<std::string> help(std::string_view topic) const;

  // The topics the manual has help texts for, in order
  std::vector<std::string> topics() const;

private:
  std::string text_;
};

// Reads the manual from file. Error text when it cannot be read.
std::expected<Manual, std::string> read_manual(std::filesystem::path const& file);

// The Markdown of one line as plain text for the terminal (see Manual::help).
// A list item `* x` becomes `- x`.
std::string plain_text_of(std::string_view markdown);

} // namespace cratchit
