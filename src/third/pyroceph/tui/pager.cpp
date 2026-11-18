#include "pyroceph/tui/pager.hpp"

#include <algorithm>
#include <utility>

namespace pyroceph::tui {

void Pager::set(std::vector<std::string> lines) {
  lines_ = std::move(lines);
  // Keep the page where it was, but on a whole page within the list
  if (top_ >= lines_.size()) top_ = lines_.empty() ? 0 : (lines_.size() - 1) / size_ * size_;
}

std::size_t Pager::shown() const { return std::min(size_, lines_.size() - std::min(top_, lines_.size())); }

View Pager::view() const {
  View page;
  if (lines_.empty()) return page;
  if (can_back()) page.push_back(more_line);
  for (std::size_t i = top_; i < top_ + shown(); ++i) page.push_back(lines_[i]);
  if (can_forward()) page.push_back(more_line);
  return page;
}

bool Pager::back() {
  if (!can_back()) return false;
  top_ = top_ >= size_ ? top_ - size_ : 0;
  return true;
}

bool Pager::forward() {
  if (!can_forward()) return false;
  top_ += size_;
  return true;
}

} // namespace pyroceph::tui
