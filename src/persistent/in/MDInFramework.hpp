#pragma once

// Meta Defacto In Freamwork for 'in from presistent storage'

#include "MetaDefacto.hpp"
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

    struct MaybeIStream {
      std::unique_ptr<std::istream> m_istream_ptr{};
      operator bool() const {return m_istream_ptr != nullptr;}
      std::istream& value() {
        if (*this) {
          return *m_istream_ptr;
        }
        throw std::runtime_error("Access to MaybeInStream with null stream");
      }
    };

    MaybeIStream to_maybe_istream(std::filesystem::path sie_file_path);
    MaybeIStream from_string(std::string const& s);

  }
} // persistent

