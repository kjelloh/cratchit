#pragma once

#include "Msg.hpp"
#include "Ux.hpp"

#include <immer/vector.hpp>
#include <variant>

class RootView;

using ViewState = std::variant<RootView>;

class RootView {
public:
  using This = RootView;

  // update
  This update(tea::UnicodeKeyMsg const& unicode_msg) const;
  This update(tea::BackspaceKeyMsg const& backspace_key_msg) const;

  tea::Ux view() const;
  using CodePointBuffer = immer::vector<char32_t>;

  // code point buffer handling
  This with_pushed_unicode(char32_t cp) const;
  This with_popped_unicode() const;

  private:

    CodePointBuffer m_code_point_buffer{};
};
