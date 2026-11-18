#pragma once

#include "pyroceph/tui/state.hpp"

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace pyroceph::tui {

// What to show for a folder in a path the user types: the folder and a
// separator after it, e.g. U"/Users/me/Documents/"
std::u32string path_input_of(std::filesystem::path const& folder);

// The path the user typed, as the platform reads it
std::filesystem::path path_of(std::u32string_view typed);

// Completes the last name in typed to the existing folders (and, with files,
// files) whose names start with it, like Tab in a shell (from take 3):
// * One match: the rest of its name, and a separator after a folder.
// * Several: as far as they agree, or, when they agree on nothing more than
//   typed, nothing added and their names listed, sorted.
// * Hidden names (.name) count only when the typed name starts with a dot.
// * Nothing for a typed text without a separator, or a folder that cannot be read.
// Names compare as typed on macOS and Linux, and ignoring case (A-Z) on Windows.
// With accept_file, only the files it accepts count (folders always count).
// With describe_file, a file gets lines that tell what it holds: listed
// indented under its name among the candidates, and as notes when the file is
// the one match (also when its whole name is typed already).
// With list, the client lays out the candidates itself (e.g. grouped, in
// columns): it gets the matching files and folders, each sorted by name, and
// gives the lines to list.
using DescribeFile = std::function<std::vector<std::string>(std::filesystem::path const&)>;
using ListCandidates = std::function<std::vector<std::string>(std::vector<std::filesystem::path> const& files,
                                                              std::vector<std::filesystem::path> const& folders)>;
Completion complete_path(std::u32string_view typed, bool files = false,
                         std::function<bool(std::filesystem::path const&)> const& accept_file = {},
                         DescribeFile const& describe_file = {}, ListCandidates const& list = {});

} // namespace pyroceph::tui
