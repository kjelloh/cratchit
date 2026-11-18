#pragma once

#include "core/date.hpp"

#include <cstddef>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

// The inventory (förteckning) of a project's folder: the files cratchit has
// used, known by the SHA-256 of their content, so that cratchit can tell where
// the books come from without moving, renaming or copying the user's files.
// For now the SIE files read into the books and the account statements read into drafts.
//
// Two levels, each a CSV file with a header row, one row per use of a file:
// * the project's: <project>/.cratchit/inventory.csv
// * a fiscal year's: <project>/.cratchit/books/<year start>/inventory.csv
// A fiscal year's entries override the project's for the same file (sha256),
// since the folder may be laid out differently over the years.
struct InventoryEntry {
  std::string sha256;       // of the file's content, 64 lowercase hex digits
  std::string kind;         // "sie" or "kontoutdrag"
  std::string name;         // the file's name when used
  std::string path;         // where it was, relative to the project folder (also when outside it, e.g. ../x.se)
  std::string size;         // in bytes
  std::string read;         // the day it was used, YYYY-MM-DD
  std::string use;          // "ny bokföring" or "import" (SIE), "utkast" (kontoutdrag)
  std::string generated;    // #GEN, YYYY-MM-DD, or empty
  std::string program;      // #PROGRAM name and version, or empty
  std::string year_start;   // #RAR 0
  std::string year_end;
  std::string status;       // the fiscal year's status then (see status_of), without warning
  std::string vouchers_added;
  std::string vouchers_changed;

  bool operator==(InventoryEntry const&) const = default;
};

// The path of file as the inventory stores it: relative to the project folder
// (also when outside it, e.g. ../x.se), with generic separators (/), so that
// the folder can move to another place and platform. UTF-8.
std::string stored_path(std::filesystem::path const& file, std::filesystem::path const& project_folder);

// <project>/.cratchit/inventory.csv
std::filesystem::path inventory_file(std::filesystem::path const& project_folder);

// <project>/.cratchit/books/<year start>/inventory.csv
std::filesystem::path year_inventory_file(std::filesystem::path const& project_folder, Date year_start);

// The entries of one inventory file, in order; none when it does not exist.
// Error text when it cannot be read or a row has another number of fields.
std::expected<std::vector<InventoryEntry>, std::string> read_inventory_file(std::filesystem::path const& file);

// Adds the entry last in the inventory file (made, with its folder, when missing). Error text when it cannot be written.
std::expected<void, std::string> add_to_inventory(std::filesystem::path const& file, InventoryEntry const& entry);

// The project's inventory, with the fiscal year's entries overriding the
// project's for the same file when a year is given. Error text as read_inventory_file.
std::expected<std::vector<InventoryEntry>, std::string> read_inventory(std::filesystem::path const& project_folder,
                                                                      std::optional<Date> year_start = std::nullopt);

// What the books got from an SIE file, for its entry
struct SieUse {
  std::string use; // "ny bokföring" or "import"
  std::size_t vouchers_added = 0;
  std::size_t vouchers_changed = 0;
};

// Records that the SIE file was read into the project's books on day today:
// an entry in the inventory of the file's fiscal year (#RAR 0), with its
// SHA-256, name, path relative to the project folder, size, #GEN, #PROGRAM,
// fiscal year and its status. Never touches the SIE file. Error text when the
// file cannot be read or the entry cannot be written.
std::expected<InventoryEntry, std::string> record_sie_use(std::filesystem::path const& project_folder,
                                                          std::filesystem::path const& sie_file, SieUse const& use,
                                                          Date today);

// Records that the account statement was read into drafts of the fiscal year
// year_start – year_end on day today: an entry in that year's inventory, kind
// "kontoutdrag", use "utkast", with its SHA-256, name, path relative to the
// project folder and size (no SIE fields). Never touches the file. Error text
// when it cannot be read or the entry cannot be written.
std::expected<InventoryEntry, std::string> record_statement_use(std::filesystem::path const& project_folder,
                                                                std::filesystem::path const& statement_file,
                                                                Date year_start, Date year_end, Date today);

} // namespace cratchit
