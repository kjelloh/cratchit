#pragma once

#include <filesystem>
#include <fstream>
#include <vector>
#include <optional>
#include <string>
#include <format>
#include <system_error>
#include "functional/maybe.hpp"

namespace cratchit {
namespace io {

  // Result types for file I/O operations with error context

  /// Byte buffer type for raw file content
  using ByteBuffer = std::vector<std::byte>;

  /// Result type for file operations with error messages
  template<typename T>
  // using IOResult = functional::AnnotatedOptional<T>;
  using IOResult = AnnotatedMaybe<T>;

  /// Error information for file operations
  struct FileIOError {
    std::filesystem::path path;
    std::string message;
    std::error_code error_code;

    std::string to_string() const {
      return std::format("FileIOError: {} (path: {}, error_code: {})",
                         message, path.string(), error_code.message());
    }
  };

  // Forward declarations
  class FileReader;

  /// Opens a file for reading
  /// @param file_path Path to the file
  /// @return Optional stream if file can be opened, empty otherwise
  IOResult<std::unique_ptr<std::istream>> open_file(std::filesystem::path const& file_path);

  /// Reads all bytes from an input stream into a buffer
  /// @param is Input stream to read from
  /// @return Optional buffer if read succeeds, empty otherwise
  IOResult<ByteBuffer> read_stream_to_buffer(std::istream& is);

  /// Reads all bytes from a file into a buffer (composed pipeline)
  /// @param file_path Path to the file
  /// @return Optional buffer if file can be read, empty otherwise
  IOResult<ByteBuffer> read_file_to_buffer(std::filesystem::path const& file_path);

  // Implementation

  inline IOResult<std::unique_ptr<std::istream>> open_file(std::filesystem::path const& file_path) {
    IOResult<std::unique_ptr<std::istream>> result{};

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

  inline IOResult<ByteBuffer> read_stream_to_buffer(std::istream& is) {
    IOResult<ByteBuffer> result{};

    if (!is.good()) {
      result.push_message("Stream is not in a good state for reading");
      return result;
    }

    // Get stream size if possible
    is.seekg(0, std::ios::end);
    auto size_pos = is.tellg();
    is.seekg(0, std::ios::beg);

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
    is.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(size));

    if (!is) {
      auto bytes_read = static_cast<std::size_t>(is.gcount());
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

  inline IOResult<ByteBuffer> read_file_to_buffer(std::filesystem::path const& file_path) {
    // Monadic composition: file_path → stream → buffer
    return open_file(file_path)
      .and_then([](auto& stream_ptr) -> IOResult<ByteBuffer> {
        return read_stream_to_buffer(*stream_ptr);
      });
  }

} // namespace io
} // namespace cratchit
