// Followed example in Youtube video https://youtu.be/el7p-HC77g8
// Code from https://github.com/jonkero9/universe_generation_demo/blob/main/src/main.cpp

#include "jui.hpp"


namespace JUI {

  void Jui::draw() {
    int num_x_sec = /* raylib */ GetScreenWidth() / this->sec_size;
    int num_y_sec = /* raylib */ GetScreenHeight() / this->sec_size;

    // x is 'column' left-to-right
    for (size_t x=0;x<num_x_sec;++x) {

      auto line_color = to_ray_color(Jcolor::Cornsilk);
      /* raylib */ DrawLine(
         x*sec_size                         // start x
        ,0                                  // start y
        ,x*sec_size                         // end x
        ,/* raylib */ GetScreenHeight()      // end y
        ,line_color                         // color
      );

      // y is 'row' up-to-down
      for (size_t y=0;y<num_y_sec;++y) {
        /* raylib */ DrawLine(
           0                                  // start x (leftmost column)
          ,y*sec_size                         // start y
          ,GetScreenWidth()                   // end x (rightmost column)
          ,y*sec_size                         // end y (same = vertical line)
          ,line_color                         // color
        );

      }
    }
  } // draw

  Color to_ray_color(Jcolor::JColor col) {
    return Color{col.r, col.g, col.b, col.a};
  };

  ImVec4 to_imvec4(const Jcolor::JColor c) {
    return ImVec4(c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 255.0);
  };

} // JUI

