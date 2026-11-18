#include "pyroceph/tui/input_buffer.hpp"

#include <gtest/gtest.h>

using pyroceph::tui::InputBuffer;
using pyroceph::tui::is_control;

TEST(InputBuffer, StartsEmpty) {
  InputBuffer input;
  EXPECT_TRUE(input.empty());
  EXPECT_FALSE(input.erase());
}

TEST(InputBuffer, PushAndErase) {
  InputBuffer input;
  EXPECT_TRUE(input.push(U'2'));
  EXPECT_TRUE(input.push(U'å'));
  EXPECT_EQ(input.text(), U"2å");
  EXPECT_TRUE(input.erase());
  EXPECT_EQ(input.text(), U"2");
  EXPECT_TRUE(input.erase());
  EXPECT_TRUE(input.empty());
}

TEST(InputBuffer, ControlCharactersAreIgnored) {
  InputBuffer input;
  for (char32_t c : {U'\x1B', U'\n', U'\x7F', U'\x85'}) EXPECT_FALSE(input.push(c));
  EXPECT_TRUE(input.empty());
}

TEST(InputBuffer, IsControl) {
  EXPECT_TRUE(is_control(0x00));
  EXPECT_TRUE(is_control(0x1F));
  EXPECT_FALSE(is_control(U' '));
  EXPECT_FALSE(is_control(U'~'));
  EXPECT_TRUE(is_control(0x7F));
  EXPECT_TRUE(is_control(0x9F));
  EXPECT_FALSE(is_control(0xA0));
}
