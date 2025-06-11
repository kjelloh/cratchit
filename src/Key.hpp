#pragma once

#include <vector>
#include <string>
#include <ostream>

namespace Key {
  class Path {
  public:
    auto begin() const { return m_path.begin(); }
    auto end() const { return m_path.end(); }
    Path() = default;
    Path(Path const &other) = default;
    Path(std::vector<std::string> const &v);
    Path(std::string const &s_path, char delim = '^');
    auto size() const { return m_path.size(); }
    Path operator+(std::string const &key) const;
    operator std::string() const;
    Path &operator+=(std::string const &key);
    Path &operator--();
    Path parent();
    std::string back() const;
    std::string operator[](std::size_t pos) const;
    friend std::ostream &operator<<(std::ostream &os, Path const &key_path);
    std::string to_string() const;

  private:
    std::vector<std::string> m_path{};
    char m_delim{'^'};
  }; // class Path

  using Paths = std::vector<Path>;

  std::ostream &operator<<(std::ostream &os, Key::Path const &key_path);
  std::string to_string(Key::Path const &key_path);
} // namespace Key
