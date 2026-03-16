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

      using ByteBuffer = ::text::encoding::ByteBuffer;
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
        std::optional<std::unique_ptr<std::istream>> injected_string_to_istream_ptr_step(std::string s);
        std::optional<std::unique_ptr<std::istream>> path_to_istream_ptr_step(std::filesystem::path const& file_path);
        std::optional<ByteBuffer> istream_ptr_to_byte_buffer_step(std::unique_ptr<std::istream>&& istream_ptr);
      } // maybe

      namespace monadic {
        AnnotatedMaybe<std::unique_ptr<std::istream>> injected_string_to_istream_ptr_step(std::string s);
        AnnotatedMaybe<std::unique_ptr<std::istream>> path_to_istream_ptr_step(std::filesystem::path const& file_path);
        AnnotatedMaybe<ByteBuffer> istream_ptr_to_byte_buffer_step(std::unique_ptr<std::istream>&& istream_ptr);
      } // monadic

    } // text

  } // in
} // persistent
