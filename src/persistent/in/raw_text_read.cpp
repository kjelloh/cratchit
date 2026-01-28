#include "raw_text_read.hpp"
#include <span>

namespace persistent {
  namespace in {

    namespace text {
      namespace maybe {

        std::optional<std::unique_ptr<std::istream>> injected_string_to_istream_ptr(std::string s) {
          std::optional<std::unique_ptr<std::istream>> result{};
          auto iss_ptr = std::make_unique<std::istringstream>(s);
          if (*iss_ptr) {
            result = std::move(iss_ptr);
          }
          return result;
        }


        std::optional<std::unique_ptr<std::istream>> path_to_istream_ptr_step(std::filesystem::path const& file_path) {
          std::optional<std::unique_ptr<std::istream>> result{};
          auto is_ptr = std::make_unique<std::ifstream>(file_path, std::ios::binary);
          if (*is_ptr) {
            result = std::move(is_ptr);
          }
          return result;
        }

        // TODO: Consider to return std::expected<ByteBuffer,some error code>
        //       For istream reading it may be to 'weak' to just return nullopt on failure.
        //       But good enoigh for now I asume?
        std::optional<ByteBuffer> istream_ptr_to_byte_buffer_step(std::unique_ptr<std::istream>&& istream_ptr) {

          ByteBuffer buffer;
          const size_t chunk_size = 4096;

          // NOTE 20260122 - I am unsatisfied with the code options to properly read bytes from std::istream.
          //                 I went with as_writable_bytes on span over std::arrray of bytes.
          //                 We still need to cast from char/char* to bytes though.

          std::array<std::byte, chunk_size> temp;
          auto writable = std::as_writable_bytes(std::span(temp));

          while (true) {
              istream_ptr->read(reinterpret_cast<char*>(writable.data()), writable.size());

              std::streamsize n = istream_ptr->gcount();

              if (n > 0) {
                  buffer.insert(buffer.end(), temp.begin(), temp.begin() + n);
              }

              // Check stream state BEFORE continuing
              if (istream_ptr->eof()) {
                  break; // Clean EOF after processing all bytes
              }

              if (istream_ptr->fail() || istream_ptr->bad()) {
                  // Could capture: rdstate(), strerror(errno), etc.
                  return {}; // Error during read
              }              

          }

          return buffer; 

        }


      } // maybe

      namespace monadic {

        // Helper std::string -> maybe istream
        AnnotatedMaybe<std::unique_ptr<std::istream>> injected_string_to_istream_ptr(std::string s) {

          auto f = cratchit::functional::to_annotated_maybe_f(
            persistent::in::text::maybe::injected_string_to_istream_ptr
            ,"Failed to create istringstream"
          );
          return f(s);
        } // injected_string_to_istream_ptr

        AnnotatedMaybe<std::unique_ptr<std::istream>> path_to_istream_ptr_step(std::filesystem::path const& file_path) {

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

          auto f = cratchit::functional::to_annotated_maybe_f(
            persistent::in::text::maybe::path_to_istream_ptr_step
            ,std::format(
              "Failed to open file for reading: {}"
              ,error_string)
            ,std::format("File {} opened OK",file_path.string())
          );

          return f(file_path);

        } // path_to_istream_ptr_step

        AnnotatedMaybe<ByteBuffer> istream_ptr_to_byte_buffer_step(std::unique_ptr<std::istream>&& istream_ptr) {
          AnnotatedMaybe<ByteBuffer> result{};

          auto f = cratchit::functional::to_annotated_maybe_f(
            persistent::in::text::maybe::istream_ptr_to_byte_buffer_step
            ,"Failed to read stream"
          );

          // NOTE: to_annotated_maybe_f takes hard coded string before the result is known
          //       So we add on a success message with resulting buffer size on success
          result = f(std::move(istream_ptr));
          if (result) {
            result.push_message(
              std::format("Successfully read {} bytes from stream", result.value().size())
            );
          }

          return result;
        } // istream_ptr_to_byte_buffer_step


      } // monadic

    } // text

  } // in
} // persistent
