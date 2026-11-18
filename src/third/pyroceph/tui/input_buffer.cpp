#include "pyroceph/tui/input_buffer.hpp"

namespace pyroceph::tui {

bool is_control(CodePoint code_point) {
  return code_point < 0x20 || (code_point >= 0x7F && code_point <= 0x9F);
}

bool InputBuffer::push(CodePoint code_point) {
  if (is_control(code_point)) return false;
  text_.push_back(code_point);
  return true;
}

bool InputBuffer::erase() {
  if (text_.empty()) return false;
  text_.pop_back();
  return true;
}

} // namespace pyroceph::tui
