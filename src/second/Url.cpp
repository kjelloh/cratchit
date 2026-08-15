#include "Url.hpp"
#include "log.hpp"

#include <ranges>

Url::Url(std::string s) : m_s{s} {}

std::string Url::string() const {return m_s;}

bool is_local_file_url(Url const& url) {

  bool result{false};

  std::string const FILE_URL_MARKER{"file:"};

  std::string const& s = url.string();

  log_development_trace(
     "is_local_file_url(url:{})"
    ,url.string()
  );

  if (url.string().size() < FILE_URL_MARKER.size()) return false;

  // RFC 8089: file URI scheme is case-insensitive.
  auto is_case_insensitive_equal = [](char lhs,char rhs) {
    return 
          std::tolower(static_cast<unsigned char>(lhs)) 
      ==  std::tolower(static_cast<unsigned char>(rhs));
  }; // is_case_insensitive_equal

  result = std::ranges::equal(
     s | std::views::take(FILE_URL_MARKER.size())
    ,FILE_URL_MARKER
    ,is_case_insensitive_equal
  );

  log_development_trace(
     "is_local_file_url -> {}"
    ,result
  );

  return result;

} // is_local_file_url

