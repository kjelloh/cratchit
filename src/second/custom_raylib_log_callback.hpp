#pragma once

#include "raylib.h" // See https://www.raylib.com/cheatsheet/cheatsheet.html, https://github.com/raysan5/raylib

// Custom logging function to register with raylib
void custom_raylib_log_callback(int msgType, const char *text, va_list args);