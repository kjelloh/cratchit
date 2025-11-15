#include "maybe.hpp"
#include "logger/log.hpp"

namespace persistent {
  namespace in {

    MaybeIStream to_maybe_istream(std::filesystem::path sie_file_path) {
      return MaybeIStream{std::make_unique<std::ifstream>(sie_file_path)};
    }

    MaybeIStream from_string(std::string const& s) {
      return MaybeIStream{std::make_unique<std::istringstream>(s)};
    }

    // MDMaybeIFStream to_md_maybe_istream(std::filesystem::path sie_file_path) {
    //   return MDMaybeIFStream {
    //      .meta = {sie_file_path}
    //     ,.defacto = to_maybe_istream(sie_file_path)
    //   };
    // }

  }
} // persistent