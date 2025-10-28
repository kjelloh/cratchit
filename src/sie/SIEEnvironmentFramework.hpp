#pragma once

#include "SIEEnvironment.hpp"
#include <istream>
#include <filesystem>

BAS::MetaEntry to_entry(SIE::Ver const& ver);
OptionalSIEEnvironment sie_from_stream(std::istream& is);
OptionalSIEEnvironment sie_from_sie_file(std::filesystem::path const& sie_file_path);