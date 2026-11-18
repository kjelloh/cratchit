#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

// Saving a document cratchit shows (a report, a voucher's Utskrift) to a text
// file, when the user asks for it (beslut 2026-11-18): cratchit makes a visible
// file only then, and never writes over one.
namespace cratchit {

// A file name (UTF-8, without extension) from title: each character that a
// file name may not have on Windows, macOS or Linux (\ / : * ? " < > | and
// control characters) becomes '-', and spaces and dots at the end are dropped
// (Windows drops them itself). "dokument" when nothing is left.
std::string file_name_of(std::string_view title);

// Writes lines to <folder>/<file_name_of(title)>.txt, each line ended by "\n",
// in UTF-8. When that file is there, <name> (2).txt, (3).txt, ... instead: an
// existing file is never written over. The file written, or error text.
std::expected<std::filesystem::path, std::string> save_document(std::filesystem::path const& folder, std::string_view title,
                                                                std::vector<std::string> const& lines);

// A path as UTF-8 text, for showing it (path.string() is not UTF-8 on Windows)
std::string utf8_of(std::filesystem::path const& path);

} // namespace cratchit
