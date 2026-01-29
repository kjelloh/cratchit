#pragma once

#include "text/encoding.hpp" // BOM
#include "functional/maybe.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <optional>
#include <string>
#include <format>
#include <system_error>

namespace persistent {
  namespace in {

    namespace text {

      // Byte buffer type for raw file content
      using ByteBuffer = std::vector<std::byte>;
      using BOM = ::text::encoding::BOM;

      class bom_istream {
      public:
          std::istream& raw_in;
          std::optional<BOM> bom{};
          bom_istream(std::istream& in);
          operator bool();

        // TODO: Move base class members from derived 8859_1 and UTF-8 istream classes to here
      private:
      };


      namespace maybe {
        std::optional<std::unique_ptr<std::istream>> injected_string_to_istream_ptr(std::string s);
        std::optional<std::unique_ptr<std::istream>> path_to_istream_ptr_step(std::filesystem::path const& file_path);
        std::optional<ByteBuffer> istream_ptr_to_byte_buffer_step(std::unique_ptr<std::istream>&& istream_ptr);
      } // maybe

      namespace monadic {
        AnnotatedMaybe<std::unique_ptr<std::istream>> injected_string_to_istream_ptr(std::string s);
        AnnotatedMaybe<std::unique_ptr<std::istream>> path_to_istream_ptr_step(std::filesystem::path const& file_path);
        AnnotatedMaybe<ByteBuffer> istream_ptr_to_byte_buffer_step(std::unique_ptr<std::istream>&& istream_ptr);
      } // monadic

      // Monadic AnnotatedMaybe #1 + #2
      AnnotatedMaybe<ByteBuffer> path_to_byte_buffer_shortcut(std::filesystem::path const& file_path);

      inline AnnotatedMaybe<ByteBuffer> path_to_byte_buffer_shortcut(std::filesystem::path const& file_path) {
        // Monadic composition: file_path → istream_ptr → buffer
        return monadic::path_to_istream_ptr_step(file_path)
          .and_then(monadic::istream_ptr_to_byte_buffer_step);
      }
    } // text

  } // in
} // persistent
