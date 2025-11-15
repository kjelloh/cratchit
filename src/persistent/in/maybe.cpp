#include "maybe.hpp"
#include "logger/log.hpp"

namespace persistent {
  namespace in {

    MaybeIStream to_maybe_istream(std::filesystem::path sie_file_path) {
        // Open in binary mode to disable any potential platform specific text-file processing.
        // That is, on windows the story is that the platform may convert \n\r to \n (hide unix eol)
        // Also on windows, text files may use strcl-z as EOF.
        // TODO: Consider to remember to test app behaviour on Linix and Windows.
        //       For now develpment is on macOS / 20251115 
        auto stream = std::make_unique<std::ifstream>(sie_file_path, std::ios::binary);
        if (!stream->is_open()) return {}; // failed to open
        return MaybeIStream{std::move(stream)};
    }


    MaybeIStream from_string(std::string const& s) {
      return MaybeIStream{std::make_unique<std::istringstream>(s)};
    }

    std::optional<std::string> to_raw_bytes(std::istream& is) {
      if (!is) return std::nullopt;
      std::string buffer((std::istreambuf_iterator<char>(is)),
                        std::istreambuf_iterator<char>());
      return buffer;
    }

  }
} // persistent