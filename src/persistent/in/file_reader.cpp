#include "file_reader.hpp"

namespace persistent {
  namespace in {
    namespace maybe {

      std::optional<std::unique_ptr<std::istream>> path_to_istream_ptr_step(std::filesystem::path const& file_path) {
        std::optional<std::unique_ptr<std::istream>> result{};

        auto is_ptr = std::make_unique<std::ifstream>(file_path, std::ios::binary);
        if (*is_ptr) {
          result = std::move(is_ptr);
        }

        return result;
      }

    } // maybe

    namespace monadic {

      // Helper std::string -> maybe istream
      AnnotatedMaybe<std::unique_ptr<std::istream>> injected_string_to_istream_ptr(std::string s) {
        AnnotatedMaybe<std::unique_ptr<std::istream>> result{};
        result.m_value = std::move(std::make_unique<std::istringstream>(s));
        if (!result) {
          result.push_message("Failed to create istringstream");
        }
        return result;
      } // injected_string_to_istream_ptr

      AnnotatedMaybe<std::unique_ptr<std::istream>> path_to_istream_ptr_step(std::filesystem::path const& file_path) {
        AnnotatedMaybe<std::unique_ptr<std::istream>> result{};

        auto  error_string = std::format("Failed for file {}",file_path.string());
        std::error_code ec;
        if (!std::filesystem::exists(file_path, ec)) {
          error_string += std::format(" - File does not exist");
        }

        if (ec) {
          error_string += std::format(
             " - Error checking file existence - message:{}"
            ,ec.message());
        }

        auto f = cratchit::functional::to_annotated_nullopt(
           persistent::in::maybe::path_to_istream_ptr_step
          ,std::format("Failed to open file for reading: {}"
          ,error_string)
        );

        return f(file_path);

      } // path_to_istream_ptr_step

      AnnotatedMaybe<ByteBuffer> istream_ptr_to_byte_buffer_step(std::unique_ptr<std::istream>&& istream_ptr) {
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
      } // istream_ptr_to_byte_buffer_step


    } // monadic
  } // in
} // persistent
