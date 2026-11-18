#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

// Documents (underlag) as PDF files in a folder, read with walk_pdf (session
// 9fe36d28). cratchit only reads them.
namespace cratchit {

// True for a file named *.pdf, in any case
bool is_pdf(std::filesystem::path const& file);

// The pdf files in folder (not hidden ones, not in subfolders), by name; none
// when it cannot be read
std::vector<std::filesystem::path> pdf_files_in(std::filesystem::path const& folder);

// What walk_pdf finds in a PDF, for a status line
struct PdfStatus {
  enum class Kind {
    text,          // text on some page
    no_text,       // pages, but no text on them (e.g. a scanned image)
    no_pages,      // no pages found: walk_pdf finds no page tree
    password,      // encrypted, and it needs a password
    unsupported,   // encrypted in a way walk_pdf cannot decrypt (AES)
    not_permitted, // its owner has marked its text not to be copied
    not_a_pdf,     // no "%PDF-" at its start
    cannot_read,   // the file could not be read
  };
  Kind kind = Kind::cannot_read;
  std::size_t pages = 0;
  std::size_t unmapped = 0; // characters that could not be read (text only)
};

// A PDF read once: what walk_pdf finds in it and its text
struct PdfDocument {
  std::filesystem::path file;
  PdfStatus status;
  std::vector<std::vector<std::string>> pages; // each page's lines (UTF-8); none unless status is text
};

// Reads file with walk_pdf::extract_text, once: its status and its text. Does not log.
PdfDocument read_pdf(std::filesystem::path const& file);

// The status of file (see read_pdf). Does not log.
PdfStatus pdf_status(std::filesystem::path const& file);

// The status for the user, e.g. "text, 2 sidor", "text, 1 sida, 4 tecken oläsliga",
// "ingen text, 1 sida (inskannad?)", "inga sidor hittade", "krypterad med lösenord"
std::string status_text(PdfStatus const& status);

} // namespace cratchit
