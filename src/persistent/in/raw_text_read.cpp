#include "raw_text_read.hpp"
#include "logger/log.hpp"
#include <span>

namespace persistent {
  namespace in {


    namespace text {

      bom_istream::bom_istream(std::istream& in) : raw_in{in} {
        // Check for BOM in fresh input stream
        BOM candidate{};

        if (raw_in >> candidate) {
          logger::cout_proxy << "\nConsumed BOM:" << candidate;
          this->bom = candidate;
        }
        else {
          // std::cout << "\nNo BOM detected in stream";
          raw_in.clear(); // clear the signalled failure to allow the stream to be read for non-BOM content
        }
        // std::cout << "\nbom_istream{} raw_in is at position:" << raw_in.tellg();
      }

      bom_istream::operator bool() {
        return static_cast<bool>(raw_in);
      }

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
            ,std::format("{} bytes -> stream",s.size())
          );
          return f(s);
        } // injected_string_to_istream_ptr

        AnnotatedMaybe<std::unique_ptr<std::istream>> path_to_istream_ptr_step(std::filesystem::path const& file_path) {

          auto f = cratchit::functional::to_annotated_maybe_f(
             persistent::in::text::maybe::path_to_istream_ptr_step
            ,std::format("{} -> stream",file_path.filename().string())
          );

          return f(file_path);

        } // path_to_istream_ptr_step

        AnnotatedMaybe<ByteBuffer> istream_ptr_to_byte_buffer_step(std::unique_ptr<std::istream>&& istream_ptr) {
          AnnotatedMaybe<ByteBuffer> result{};

          auto to_msg = [](ByteBuffer const& buffer){
            return std::format("{} bytes",buffer.size());
          };

          auto f = cratchit::functional::to_annotated_maybe_f(
            persistent::in::text::maybe::istream_ptr_to_byte_buffer_step
            ,"byte buffer"
            ,to_msg
          );

          result = f(std::move(istream_ptr));

          return result;
        } // istream_ptr_to_byte_buffer_step


      } // monadic

      AnnotatedMaybe<ByteBuffer> path_to_byte_buffer_shortcut(std::filesystem::path const& file_path) {
        // Monadic composition: file_path → istream_ptr → buffer
        return monadic::path_to_istream_ptr_step(file_path)
          .and_then(monadic::istream_ptr_to_byte_buffer_step);
      }


    } // text

  } // in
} // persistent
