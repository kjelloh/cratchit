#include "MDInFramework.hpp"
#include "logger/log.hpp"

namespace persistent {
  namespace in {

    MaybeIStream to_maybe_istream(std::filesystem::path sie_file_path) {
      return MaybeIStream{std::make_unique<std::ifstream>(sie_file_path)};
    }

  }
} // persistent