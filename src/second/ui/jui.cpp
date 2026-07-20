// Followed example in Youtube video https://youtu.be/el7p-HC77g8
// Code from https://github.com/jonkero9/universe_generation_demo/blob/main/src/main.cpp

#include "jui.hpp"

Color JUI::to_ray_color(Jcolor::JColor col) {
  return Color{col.r, col.g, col.b, col.a};
};

ImVec4 JUI::to_imvec4(const Jcolor::JColor c) {
  return ImVec4(c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 255.0);
};
