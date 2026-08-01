#pragma once

#include "raylib.h" // See https://www.raylib.com/cheatsheet/cheatsheet.html, https://github.com/raysan5/raylib

// #tea
#include "Msg.hpp"
#include "view.hpp"

#include <vector>
#include <deque>

class CratchitRaylibApp {
public:
  int run(int, char**);
private:
  const int INITIAL_SCREEN_WIDTH = 1080;
  const int INITIAL_SCREEN_HEIGHT = 720;
  const int FONT_HEIGHT = 32;

  Font m_current_font{};

  size_t m_frames_counter{0};
  void render(tea::Ux const& ux);
  
}; // CratchitRaylibApp
