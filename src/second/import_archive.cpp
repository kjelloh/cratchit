#include "import_archive.hpp"

namespace detail {
  ImportArchiveError to_import_archive_error(OpenFileError) {
    return ImportArchiveError::NotYetImplemented;
  }
  ImportArchiveError to_import_archive_error(ParseArchiveError) {
    return ImportArchiveError::NotYetImplemented;
  }

  // Helper std::expected<T,E> -> std::expected<T,ImportArchiveError>
  template <typename T, typename E>
  auto with_import_archive_error(std::expected<T, E> result) -> std::expected<T, ImportArchiveError> {
    if (result) {
      return std::move(*result);
    }

    return std::unexpected(
      to_import_archive_error(std::move(result.error()))
    );
  }

} // detail

template <typename F>
auto in_this_error_domain(F f) {
    return [f = std::move(f)](auto&&... args) {
        return detail::with_import_archive_error(
            std::invoke(f, std::forward<decltype(args)>(args)...)
        );
    };
} // in_this_error_domain


ExpectedImportedArchive import_archive(std::filesystem::path path) {

  auto import_result = 
     in_this_error_domain(open_file)(path)
    .and_then(in_this_error_domain(parse_archive)); 

  return import_result;

} // import_archive
