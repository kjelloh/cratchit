#include "open_file.hpp"
#include "log.hpp"

#include "make_owning_istream.tpp"

#include <fstream>
#include <format>

ExpectedOpenFile open_file(std::filesystem::path path) {
  log_development_trace(
    "open_file(path:'{}')"
    ,path.string()
  );
  auto istream_ptr = make_owning_istream<std::ifstream>(path);
  if (*istream_ptr) {
    return StreamSource{
      Url{std::format(
         "file://{}"
        ,path.string())
      }
      ,std::move(istream_ptr)};
  }
  return std::unexpected(OpenFileError::NoFile);
}
