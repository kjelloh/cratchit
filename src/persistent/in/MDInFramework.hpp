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
      operator bool() const & {return m_istream_ptr != nullptr;}
      std::istream& value() const & {
        if (*this) {
          return *m_istream_ptr;
        }
        throw std::runtime_error("Access to MaybeInStream with null stream");
      }

      template <class F>
      auto and_then(F&& f) const & {
          using result_type = std::invoke_result_t<F, std::istream&>;
          static_assert(
              std::is_default_constructible_v<result_type>,
              "f must return an optional-like type which is default-constructible"
          );

          if (*this)
              return std::invoke(std::forward<F>(f), value());
          else
              return result_type{}; // empty
      }

    };

    MaybeIStream to_maybe_istream(std::filesystem::path sie_file_path);
    MaybeIStream from_string(std::string const& s);
    using MDMaybeIFStream = MetaDefacto<FileStreamMeta,MaybeIStream>;
    MDMaybeIFStream to_md_maybe_istream(std::filesystem::path sie_file_path);

  }
} // persistent

