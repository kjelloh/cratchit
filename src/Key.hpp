#pragma once

#include <vector>
#include <string>
#include <ostream>

// TODO: Consider to refactor naming to better convey a path of string 'keys'
namespace Key {
  class Path_ {
  public:
    auto begin() const { return m_path.begin(); }
    auto end() const { return m_path.end(); }
    Path_() = default;
    Path_(Path_ const &other) = default;
    Path_(std::vector<std::string> const &v);
    Path_(std::string const &s_path, char delim = '^');
    auto size() const { return m_path.size(); }
    Path_ operator+(std::string const &key) const;
    operator std::string() const;
    Path_ &operator+=(std::string const &key);
    Path_ &operator--();
    Path_ parent();
    std::string back() const;
    std::string operator[](std::size_t pos) const;
    friend std::ostream &operator<<(std::ostream &os, Path_ const &key_path);
    std::string to_string() const;

  private:
    std::vector<std::string> m_path{};
    char m_delim{'^'};
  }; // class Path_

  using Paths = std::vector<Path_>;

  std::ostream &operator<<(std::ostream &os, Key::Path_ const &key_path);
  std::string to_string(Key::Path_ const &key_path);

  using Sequence = Path_;

} // namespace Key
