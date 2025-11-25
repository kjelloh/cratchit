#pragma once

#include <filesystem>
#include <fstream>
#include <vector>
#include <optional>
#include <string>
#include <format>
#include <system_error>
#include "functional/maybe.hpp"

namespace persistent {
  namespace in {

    // Byte buffer type for raw file content
    using ByteBuffer = std::vector<std::byte>;

    inline AnnotatedMaybe<std::unique_ptr<std::istream>> string_to_istream_ptr(std::string s);
    AnnotatedMaybe<std::unique_ptr<std::istream>> path_to_istream_ptr(std::filesystem::path const& file_path);
    AnnotatedMaybe<ByteBuffer> istream_ptr_to_byte_buffer(std::unique_ptr<std::istream>  istream_ptr);
    AnnotatedMaybe<ByteBuffer> read_file_to_buffer(std::filesystem::path const& file_path);

    // Implementation

    // Helper std::string -> istream_ptr
    inline AnnotatedMaybe<std::unique_ptr<std::istream>> string_to_istream_ptr(std::string s) {
      AnnotatedMaybe<std::unique_ptr<std::istream>> result{};
      result.m_value = std::move(std::make_unique<std::istringstream>(s));
      if (!result) {
        result.push_message("Failed to create istringstream");
      }
      return result;
    }

    inline AnnotatedMaybe<std::unique_ptr<std::istream>> path_to_istream_ptr(std::filesystem::path const& file_path) {
      AnnotatedMaybe<std::unique_ptr<std::istream>> result{};

      // Check if file exists
      std::error_code ec;
      if (!std::filesystem::exists(file_path, ec)) {
        result.push_message(
          std::format("File does not exist: {}", file_path.string())
        );
        return result;
      }

      if (ec) {
        result.push_message(
          std::format("Error checking file existence: {} ({})",
                    file_path.string(), ec.message())
        );
        return result;
      }

      // Try to open file
      auto stream = std::make_unique<std::ifstream>(file_path, std::ios::binary);

      if (!stream || !stream->is_open() || !stream->good()) {
        result.push_message(
          std::format("Failed to open file for reading: {}", file_path.string())
        );
        return result;
      }

      // Success - move stream into result
      result.m_value = std::move(stream);
      result.push_message(
        std::format("Successfully opened file: {}", file_path.string())
      );

      return result;
    }

    inline AnnotatedMaybe<ByteBuffer> istream_ptr_to_byte_buffer(std::unique_ptr<std::istream> istream_ptr) {
      AnnotatedMaybe<ByteBuffer> result{};

      if (!istream_ptr->good()) {
        result.push_message("Stream is not in a good state for reading");
        return result;
      }

      // Get stream size if possible
      istream_ptr->seekg(0, std::ios::end);
      auto size_pos = istream_ptr->tellg();
      istream_ptr->seekg(0, std::ios::beg);

      if (size_pos < 0) {
        result.push_message("Failed to determine stream size");
        return result;
      }

      // Convert to size_t for buffer allocation and formatting
      std::size_t size = static_cast<std::size_t>(size_pos);

      // Allocate buffer
      ByteBuffer buffer;
      buffer.resize(size);

      // Read data
      istream_ptr->read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(size));

      if (!istream_ptr) {
        auto bytes_read = static_cast<std::size_t>(istream_ptr->gcount());
        result.push_message(
          std::format("Failed to read complete stream (read {} of {} bytes)",
                    bytes_read, size)
        );
        return result;
      }

      // Success
      result.m_value = std::move(buffer);
      result.push_message(
        std::format("Successfully read {} bytes from stream", size)
      );

      return result;
    }

    inline AnnotatedMaybe<ByteBuffer> read_file_to_buffer(std::filesystem::path const& file_path) {
      // Monadic composition: file_path → istream_ptr → buffer
      return path_to_istream_ptr(file_path)
        .and_then(istream_ptr_to_byte_buffer);
    }

  } // in
} // persistent
