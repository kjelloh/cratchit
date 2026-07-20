// Followed example in Youtube video https://youtu.be/el7p-HC77g8
// Code from https://github.com/jonkero9/universe_generation_demo/blob/main/src/main.cpp

#ifndef JUI_UI
#define JUI_UI

#include "second/vendored/imgui/imgui.h"
#include "second/color.hpp"
#include "second/vendored/raylib/raylib.h"

namespace JUI {

  class Jui {
  public:
    void draw();
  private:
    int sec_size = 32;
  }; // Jui

  Color to_ray_color(Jcolor::JColor col);  
  ImVec4 to_imvec4(const Jcolor::JColor c);
} // namespace JUI

#endif
