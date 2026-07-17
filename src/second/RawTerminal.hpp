#pragma once
#include <termios.h>

class RawTerminal {
public:
    RawTerminal();
    ~RawTerminal();

    char wait_for_char();

private:
    termios original_;
};