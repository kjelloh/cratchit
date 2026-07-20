// Followed example in Youtube video https://youtu.be/el7p-HC77g8
// Code from https://github.com/jonkero9/universe_generation_demo/blob/main/src/main.cpp

#ifndef JCOLOR
#define JCOLOR

#include <cstdint>

namespace Jcolor {

  struct JColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
  };

  // KANA

  const JColor Crust = JColor{22, 22, 29, 255};
  const JColor Base = JColor{30, 31, 40, 255};
  const JColor Midblue = JColor{43, 42, 56, 255};
  const JColor Midblue2 = JColor{53, 54, 71, 255};
  const JColor Midblue3 = JColor{84, 84, 109, 255};
  const JColor Cornsilk = JColor{221, 215, 186, 255};
  const JColor Slaytegrass = JColor{114, 113, 104, 255};
  const JColor Navy = JColor{35, 50, 73, 255};
  const JColor Teal = JColor{44, 79, 103, 255};
  const JColor Purpgrey = JColor{148, 138, 169, 255};
  const JColor Purp = JColor{149, 127, 184, 255};
  const JColor Lightblue = JColor{126, 156, 215, 255};
  const JColor Teal_2 = JColor{122, 168, 160, 255};
  const JColor Rosypink = JColor{210, 126, 153, 255};
  const JColor Red = JColor{232, 35, 35, 255};
  const JColor Skyblue = JColor{127, 180, 203, 255};
  const JColor Greenyell = JColor{151, 187, 108, 255};
  const JColor Violetred = JColor{227, 104, 118, 255};
  const JColor Orange = JColor{255, 160, 102, 255};
  const JColor Cadetgreen = JColor{107, 149, 137, 255};
  const JColor Burlywood = JColor{230, 195, 132, 255};
  const JColor Tan = JColor{192, 163, 110, 255};
  const JColor Salmon = JColor{255, 93, 98, 255};
  const JColor Lightsteel = JColor{156, 170, 201, 255};
  const JColor Slateblue = JColor{101, 132, 147, 255};

} // namespace Jcolor

#endif
