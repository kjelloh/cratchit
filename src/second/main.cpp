#include "main.hpp"
#include "log.hpp"

#include "raylib.h"

#include <print>
#include <iostream>
#include <format>

namespace second {

  int main(int argc, char *argv[]) {
    log_business("second::cratchit START");
    log_development_trace("Hello from second::main");
    log_design_insufficiency("This is a test");

    log_business("Test with formatting int value {}",2);
    log_development_trace("Test with formatting int value {}",3);
    log_design_insufficiency("Test with formatting int value {}",4);

    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // TODO: Update your variables here
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0; 
  }
} // second