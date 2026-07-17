#include "RawTerminal.hpp"
#include <iostream>
#include <unistd.h>

// public:
RawTerminal::RawTerminal() {
    tcgetattr(STDIN_FILENO, &original_);

    termios raw = original_;

    // Disable canonical mode and echo
    raw.c_lflag &= ~(ICANON | ECHO);

    // Return after every character
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

char RawTerminal::wait_for_char() {

    char c;
    while (true) {
      if (read(STDIN_FILENO, &c, 1) == 1) return c;
    }
}

RawTerminal::~RawTerminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_);
}
