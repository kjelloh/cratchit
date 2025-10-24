#include "Key.hpp"
#include "tokenize.hpp"
#include <iostream> // std::cout,
#include <iomanip> // std::quoted,
#include <sstream> // std::ostringstream,
#include <cctype> // std::isprint,

namespace Key {
    Path_::Path_(std::vector<std::string> const &v) : m_path{v} {}
    Path_::Path_(std::string const &s_path, char delim)
        : m_delim{delim},
          m_path(tokenize::splits(s_path, delim,
                                  tokenize::eAllowEmptyTokens::YES)) {
      if (m_path.size() == 1 and m_path[0].size() == 0) {
        // Quick Fix that tokenize::splits will return one element of zero length for an empty string :\
        // 240623 - I do not dare to fix 'splits' itself when I do not know the effect it may have on other code...
        m_path.clear();
        std::cout << "\nKey::Path_(" << std::quoted(s_path)
                  << ") PATCHED to empty path";
      }
    };
    Path_ Path_::operator+(std::string const &key) const {
      Path_ result{*this};
      result.m_path.push_back(key);
      return result;
    }
    Path_::operator std::string() const {
      std::ostringstream os{};
      os << *this;
      return os.str();
    }
    Path_& Path_::operator+=(std::string const &key) {
      m_path.push_back(key);
      // std::cout << "\noperator+= :" << *this  << " size:" << this->size();
      return *this;
    }
    Path_& Path_::operator--() {
      m_path.pop_back();
      // std::cout << "\noperator-- :" << *this << " size:" << this->size();
      return *this;
    }
    Path_ Path_::parent() {
      Path_ result{*this};
      --result;
      // std::cout << "\nparent:" << result << " size:" << result.size();
      return result;
    }
    std::string Path_::back() const { return m_path.back(); }
    std::string Path_::operator[](std::size_t pos) const { return m_path[pos]; }
    std::string Path_::to_string() const {
      std::ostringstream os{};
      os << *this;
      return os.str();
    }


  std::ostream &operator<<(std::ostream &os, Key::Path_ const &key_path) {
    int key_count{0};
    for (auto const &key : key_path) {
      if (key_count++ > 0)
        os << key_path.m_delim;
      if (false) {
        // patch to filter out unprintable characters that results from
        // incorrect character decodings NOTE: This loop will break down
        // multi-character encodings like UTF-8 and turn them into two or more
        // '?'
        for (auto ch : key) {
          if (std::isprint(ch))
            os << ch; // Filter out non printable characters (AND characters in
                      // wrong encoding, e.g., charset::ISO_8859_1 from raw file
                      // input that erroneously end up here ...)
          else
            os << "?";
        }
      } else {
        os << key; // Assume key is in runtime character encoding (ok to output
                   // as-is)
      }
      // std::cout << "\n\t[" << key_count-1 << "]:" << std::quoted(key);
    }
    return os;
  }
  std::string to_string(Key::Path_ const &key_path) {
    std::ostringstream os{};
    os << key_path;
    return os.str();
  }
} // namespace Key
