#include "main.hpp"
#include "log.hpp"

#include "vendored/raylib/raylib.h"
#include "vendored/imgui/rlImGui.h"
#include "vendored/imgui/imgui.h"
#include "ui/jui.hpp"

#include <print>
#include <iostream>

// Followed example in Youtube video https://youtu.be/el7p-HC77g8
// Code from https://github.com/jonkero9/universe_generation_demo/blob/main/src/main.cpp
void init_imgui() {
  rlImGuiSetup(true);

  ImGuiStyle& style = ImGui::GetStyle();
  style.Colors[ImGuiCol_WindowBg] = JUI::to_imvec4(Jcolor::Crust);

  style.Colors[ImGuiCol_Text] = JUI::to_imvec4(Jcolor::Cornsilk);
  style.Colors[ImGuiCol_FrameBg] = JUI::to_imvec4(Jcolor::Greenyell);
  style.WindowRounding = 8.0f;
  style.FrameRounding = 4.0f;
  style.WindowPadding = {12, 12};
  style.FramePadding = {6, 6};
  style.CellPadding = {4, 4};

  style.FontSizeBase = 28.f;  
}

namespace second {

  int main(int argc, char *argv[]) {
    log_business("second::cratchit START");
    log_development_trace("Hello from second::main");
    log_design_insufficiency("This is a test");

    log_business("Test with formatting int value {}",2);
    log_development_trace("Test with formatting int value {}",3);
    log_design_insufficiency("Test with formatting int value {}",4);

    // Followed example in Youtube video https://youtu.be/el7p-HC77g8
    // Code from https://github.com/jonkero9/universe_generation_demo/blob/main/src/main.cpp

    /* raylib */ InitWindow(900,800,"cratchit second demo");
    /* raylib */ SetTargetFPS(75);
    /* our */    init_imgui();

    // Main ImGui Loop
    bool should_run{true};

    while (should_run) {

      if (/* raylib */ IsKeyPressed(KEY_CAPS_LOCK)) {
        should_run = false;
      }

      /* raylib */ BeginDrawing();
      /* rlImGui */ rlImGuiBegin();

      /* raylib */ ClearBackground(JUI::to_ray_color(Jcolor::Crust));

      /* rlImGui */ rlImGuiEnd();
      /* raylib */ EndDrawing();

    } // while

    /* rlImGui */ rlImGuiShutdown();
    /* raylib */ CloseWindow();

    return 0;
  }
} // second