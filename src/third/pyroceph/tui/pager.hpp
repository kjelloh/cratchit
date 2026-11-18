#pragma once

#include "pyroceph/tui/state.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace pyroceph::tui {

// A page of a list of lines too long to show at once, turned a whole page at a
// time, as more does. The page shows at most size lines, with a line "…" above
// it when lines come before the page and below it when lines come after.
class Pager {
public:
  explicit Pager(std::size_t size) : size_(size == 0 ? 1 : size) {}

  // The lines to page through. The page stays where it was, moved back when
  // the list became shorter than it.
  void set(std::vector<std::string> lines);

  // The page, with "…" above and below as told above; nothing for no lines
  View view() const;

  bool can_back() const { return top_ > 0; }
  bool can_forward() const { return top_ + size_ < lines_.size(); }

  // Turn a page back or forward. True when the page changed.
  bool back();
  bool forward();

  // The first line shown (0-based), how many are shown, and how many there are
  std::size_t first() const { return top_; }
  std::size_t shown() const;
  std::size_t count() const { return lines_.size(); }

  // The line that tells that more lines come before or after the page
  static constexpr char const* more_line = "  …";

private:
  std::size_t size_;
  std::vector<std::string> lines_;
  std::size_t top_ = 0;
};

} // namespace pyroceph::tui
