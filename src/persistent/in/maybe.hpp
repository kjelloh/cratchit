#pragma once

// Meta Defacto In Freamwork for 'in from presistent storage'

#include "MetaDefacto.hpp"
#include "functional/memory.hpp" // OwningMaybeRef,...
#include <memory> // std::unique_ptr
#include <exception>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace persistent {

  struct FileStreamMeta {
    std::filesystem::path file_path;
  };

  namespace in {

    using MaybeIStream = cratchit::functional::memory::OwningMaybeRef<std::istream>;

    MaybeIStream to_maybe_istream(std::filesystem::path sie_file_path);
    MaybeIStream from_string(std::string const& s);

    std::optional<std::string> to_raw_bytes(std::istream& is);

  }
} // persistent

