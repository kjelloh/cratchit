#include "import_archive.hpp"

std::string concrete_error_to_string(ImportArchiveError error) {

  switch (error) {
    case ImportArchiveError::Undefined: return "ImportArchiveError::Undefined";
    case ImportArchiveError::NotYetImplemented: return "ImportArchiveError::NotYetImplemented";
    case ImportArchiveError::UnsupportedArchiveSource: return "ImportArchiveError::UnsupportedArchiveSource";
    case ImportArchiveError::NoFile: return "ImportArchiveError::NoFile";
    case ImportArchiveError::UnsupportedOpenFileError: return "ImportArchiveError::UnsupportedOpenFileError";
    case ImportArchiveError::MalformedArchiveSource: return "ImportArchiveError::MalformedArchiveSource";
    case ImportArchiveError::ParseErrorUnsupported: return "ImportArchiveError::ParseErrorUnsupported";
    case ImportArchiveError::Unknown: return "ImportArchiveError::Unknown";
  }

  return std::format(
    "??ImportArchiveError??:{}"
    ,static_cast<int>(error)
  );

}

namespace detail {
  ImportArchiveError to_import_archive_error(OpenFileError error) {
    switch (error) {
      case OpenFileError::NoFile: return ImportArchiveError::NoFile;
      default: ;
    } // switch
    return ImportArchiveError::UnsupportedOpenFileError;
  }
  ImportArchiveError to_import_archive_error(ParseArchiveError error) {
    switch (error) {
      case ParseArchiveError::Undefined: return ImportArchiveError::Undefined;
      case ParseArchiveError::UnsupportedStreamSource: return ImportArchiveError::UnsupportedArchiveSource;
      case ParseArchiveError::NoFile: return ImportArchiveError::NoFile;
      case ParseArchiveError::NoMagicValue: return ImportArchiveError::MalformedArchiveSource;
      case ParseArchiveError::Unknown: return ImportArchiveError::Unknown;
      default: ;
    } // switch
    return ImportArchiveError::ParseErrorUnsupported;
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
