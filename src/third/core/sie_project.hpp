#pragma once

#include "core/books/import.hpp"
#include "core/dot_folder.hpp"
#include "core/project.hpp"

#include <cstddef>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

// What creating a project from an SIE file will do. Read from the file; nothing written yet.
struct SieProjectPlan {
  std::filesystem::path sie_file; // the SIE file, in full
  std::filesystem::path folder;   // the project folder, where only .cratchit is made: the SIE file's own folder, or here
  std::string name;               // #FNAMN
  std::string orgnr;              // #ORGNR, may be empty
  std::string alert;              // when the file was not read as CP437 (see sie::lex_file), else empty
  std::string program;            // #PROGRAM name and version, e.g. "Fortnox 3.62.17", or empty
  std::optional<Date> generated;  // #GEN
  books::ImportPlan import;       // the books after importing the whole file into empty books
};

// Plans a new project from the SIE file, in the folder the file lies in, or,
// with here, in that folder (the working folder, "Ny bokföring här"; the file
// may lie anywhere). Only a .cratchit folder is made (see beslut.md). It reads and parses the file and
// plans importing it into empty books (see books::plan_import, which imports the
// file's fiscal year 0 with its accounts, dimensions, objects and vouchers).
// Error text when the file cannot be read, is no SIE file (#SIETYP), has no
// #FNAMN or no #RAR 0, has errors (the first ones are given), or when cratchit
// makes no project in the folder (see check_project_folder and check_project_here:
// a folder with .cratchit, inside a project, home or the workspace folder). Writes nothing.
std::expected<SieProjectPlan, std::string> plan_project_from_sie(
    std::filesystem::path const& sie_file, std::optional<std::filesystem::path> const& workspace_folder = std::nullopt,
    std::optional<std::filesystem::path> const& home = std::nullopt,
    std::optional<std::filesystem::path> const& here = std::nullopt);

// Creates the planned project in its folder: .cratchit/project.txt (`created`,
// `uuid` (new), `name`, `orgnr`), the books (see books::write_books: books.txt,
// accounts, dimensions, objects and the year's opening balances, vouchers and
// postings), and records the SIE file in the inventory of its fiscal year
// (see record_sie_use). The SIE file is never moved or changed. Error text when any
// step fails; what was created before stays.
std::expected<Project, std::string> create_project_from_sie(SieProjectPlan const& plan, std::string_view today);

// True when name can be a folder name on every platform: not empty, not `.`
// or `..`, no control characters, no / \ : * ? " < > |, and no space or dot at the end
bool is_folder_name(std::string_view name);

// A new random version 4 uuid, lowercase 8-4-4-4-12 (from take 3)
std::string new_uuid();

// True for text in the 8-4-4-4-12 form of hex digits
bool is_uuid_like(std::string_view text);

// True for an SIE file name: ending in .se, .si or .sie, in any case
bool is_sie_file_name(std::filesystem::path const& file);

} // namespace cratchit
