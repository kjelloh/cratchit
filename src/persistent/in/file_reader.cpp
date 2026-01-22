#include "file_reader.hpp"
#include <span>

namespace persistent {
  namespace in {
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

      std::optional<ByteBuffer> istream_ptr_to_byte_buffer_step(std::unique_ptr<std::istream>&& istream_ptr) {

        ByteBuffer buffer;
        const size_t chunk_size = 4096;

        if (true) {
          // C++23 variant of reading bytes from char stream
          // NOTE: reinterpret_cast char <-> std::byte is specifically defined and OK in the standard
          std::array<std::byte, chunk_size> temp;
          auto writable = std::as_writable_bytes(std::span(temp));

          while (istream_ptr->read(reinterpret_cast<char*>(writable.data()), chunk_size)) {
              buffer.insert(buffer.end(), temp.begin(), temp.begin() + istream_ptr->gcount());
          }
          if (istream_ptr->gcount() > 0) {
              buffer.insert(buffer.end(), temp.begin(), temp.begin() + istream_ptr->gcount());
          }
        }


        if (false) {
          // C-style (ish) reading bytes from char stream
          // NOTE: reinterpret_cast char <-> std::byte is specifically defined and OK in the standard
          char chunk[chunk_size];

          while (istream_ptr->read(chunk, sizeof(chunk)) || istream_ptr->gcount() > 0) {
              auto count = istream_ptr->gcount();
              buffer.insert(
                  buffer.end(),
                  reinterpret_cast<std::byte*>(chunk),
                  reinterpret_cast<std::byte*>(chunk + count)
              );
          }
        }

        if (false) {
          // C++20 (ish) way of copying from char stream to bytes 
          // NOTE: reinterpret_cast char <-> std::byte is specifically defined and OK in the standard

          std::array<std::byte, chunk_size> temp;

          while (istream_ptr->read(reinterpret_cast<char*>(temp.data()), chunk_size)) {
              buffer.insert(buffer.end(), temp.begin(), temp.begin() + istream_ptr->gcount());
          }
          if (istream_ptr->gcount() > 0) {
              buffer.insert(buffer.end(), temp.begin(), temp.begin() + istream_ptr->gcount());
          }        
        }

        return buffer;

      }


    } // maybe

    namespace monadic {

      // Helper std::string -> maybe istream
      AnnotatedMaybe<std::unique_ptr<std::istream>> injected_string_to_istream_ptr(std::string s) {

        auto f = cratchit::functional::to_annotated_nullopt(
           persistent::in::maybe::injected_string_to_istream_ptr
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

        auto f = cratchit::functional::to_annotated_nullopt(
           persistent::in::maybe::path_to_istream_ptr_step
          ,std::format(
            "Failed to open file for reading: {}"
            ,error_string)
          ,std::format("File {} opened OK",file_path.string())
        );

        return f(file_path);

      } // path_to_istream_ptr_step

      AnnotatedMaybe<ByteBuffer> istream_ptr_to_byte_buffer_step(std::unique_ptr<std::istream>&& istream_ptr) {
        AnnotatedMaybe<ByteBuffer> result{};

        std::string error_string{};

        if (!istream_ptr->good()) {
          error_string += "Stream is not in a good state for reading";
          return result;
        }

        // Get stream size if possible
        istream_ptr->seekg(0, std::ios::end);
        auto size_pos = istream_ptr->tellg();
        istream_ptr->seekg(0, std::ios::beg);

        if (size_pos < 0) {
          error_string += "Failed to determine stream size";
        }

        auto f = cratchit::functional::to_annotated_nullopt(
           persistent::in::maybe::istream_ptr_to_byte_buffer_step
          ,std::format(
            "Failed to read. error:{}"
            ,error_string)
        );

        result = f(std::move(istream_ptr));

        if (result) {
          result.push_message(
            std::format("Successfully read {} bytes from stream", result.value().size())
          );
        }

        return result;
      } // istream_ptr_to_byte_buffer_step


    } // monadic
  } // in
} // persistent
