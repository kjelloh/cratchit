#include "main.hpp"
#include "log.hpp"

#include "raylib.h" // See https://www.raylib.com/cheatsheet/cheatsheet.html, https://github.com/raysan5/raylib

#include <print>
#include <iostream>
#include <format>

namespace second {

  // Custom logging function
  void CustomTraceLog(int msgType, const char *text, va_list args) {
    switch (msgType) {
      case LOG_INFO:
        log_development_trace("[raylib INFO] : {}",text);
        break;
      case LOG_ERROR: 
        log_development_trace("[raylib ERROR] : {}",text);
        break;
      case LOG_WARNING: 
        log_development_trace("[raylib WARN] : {}",text);
        break;
      case LOG_DEBUG: 
        log_development_trace("[raylib DEBUG] : {}",text);
        break;
      default: break;
    }
  }

  int main(int argc, char *argv[]) {
    log_business("second::cratchit START");
    log_development_trace("Hello from second::main");
    log_design_insufficiency("This is a test");

    log_business("Test with formatting int value {}",2);
    log_development_trace("Test with formatting int value {}",3);
    log_design_insufficiency("Test with formatting int value {}",4);

    // Initialization
    //--------------------------------------------------------------------------------------

    // Set custom logger
    SetTraceLogCallback(CustomTraceLog);

    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

    // MUST be done a f t e r InitWindow.
    // Also see: https://github.com/raysan5/raylib/blob/master/examples/text/text_unicode_ranges.c

    std::vector<int> codepoints{};

    // ASCII 0x0020, 0x007E
    for (int cp = 0x0020; cp <= 0x007E; ++cp)
        codepoints.push_back(cp);

    // Latin Extended 0x00C0, 0x017F
    for (int cp = 0x00C0; cp <= 0x017F; ++cp)
        codepoints.push_back(cp);

    // Does NotoSans-Regular.ttf cover more code points?
    // According to chatGPT it does

    // chatGPT: For broad European support, you may want:
    // U+0180–U+024F	Latin Extended-B	ƒ, Ǎ, Ȟ, Ș, Ț
    // U+1E00–U+1EFF	Latin Extended Additional	Vietnamese and additional accented Latin
    // U+2000–U+206F	General Punctuation	quotes, dashes, ellipsis (See https://en.wikipedia.org/wiki/General_Punctuation)

    // U+20A0–U+20CF	Currency Symbols	€, £, ¥
    for (int cp = 0x20A0; cp <= 0x20CF; ++cp)
        codepoints.push_back(cp);

    // U+2190–U+21FF	Arrows	← ↑ → ↓
    // U+2200–U+22FF	Mathematical Operators	± ≤ ≥ ≠        

    Font font = LoadFontEx(
         "resources/NotoSans-Regular.ttf"       // path to true type font file
        ,32                                     // font size height
        ,codepoints.data()                      // array*
        ,static_cast<int>(codepoints.size())    // array element count
    );
    // NOTE: According to AI raylib renders bitmaps for fonts and then scales them when required for rendering in other sizes.
    //       Scaling down should look fine. So if we load to 32 pixel bitmaps we should get good output for sizes < 32?

    if (font.texture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load font");
        // Handle failure here
        // NOTE: raylib logging sugests we may chose to carry on (deafult ASCII font still available?)
        TraceLog(LOG_ERROR, "Exits - Bye for now");
        exit(-1);
    } else {
        TraceLog(LOG_INFO, "Font loaded successfully");
        SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    }    

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------
    // BEGIN Text input mechanism
    //--------------------------------------------------------------------------------------
    const size_t MAX_INPUT_CHARS = 9;
    char name[MAX_INPUT_CHARS + 1] = "\0";
    int letterCount = 0;

    Rectangle textBox = { screenWidth/2.0f - 100, 180, 225, 50 };
    bool mouseOnText = false;

    int framesCounter = 0;    
    //--------------------------------------------------------------------------------------
    // END Text input mechanism
    //--------------------------------------------------------------------------------------

    //--------------------------------------------------------------------------------------
    // Main render window loop
    //--------------------------------------------------------------------------------------
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        //----------------------------------------------------------------------------------
        // BEGIN Update
        //----------------------------------------------------------------------------------
        if (CheckCollisionPointRec(GetMousePosition(), textBox)) mouseOnText = true;
        else mouseOnText = false;

        if (mouseOnText) {
            // Set the window's cursor to the I-Beam
            SetMouseCursor(MOUSE_CURSOR_IBEAM);

            // Get char pressed (unicode character) on the queue
            int key = GetCharPressed();

            // Check if more characters have been pressed on the same frame
            while (key > 0)
            {
                // NOTE: Only allow keys in range [32..125]
                if (key >= ' ' and (letterCount < MAX_INPUT_CHARS))
                {
                    name[letterCount] = (char)key;
                    name[letterCount+1] = '\0'; // Add null terminator at the end of the string
                    letterCount++;
                }

                key = GetCharPressed();  // Check next character in the queue
            }

            if (IsKeyPressed(KEY_BACKSPACE))
            {
                letterCount--;
                if (letterCount < 0) letterCount = 0;
                name[letterCount] = '\0';
            }
        }
        else SetMouseCursor(MOUSE_CURSOR_DEFAULT);

        if (mouseOnText) framesCounter++;
        else framesCounter = 0;
        //----------------------------------------------------------------------------------
        // END Update
        //----------------------------------------------------------------------------------

        //----------------------------------------------------------------------------------
        // BEGIN Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

          ClearBackground(RAYWHITE);

          DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);

          //----------------------------------------------------------------------------------
          // BEGIN key input processing and rendering
          //----------------------------------------------------------------------------------

          DrawText("PLACE MOUSE OVER INPUT BOX!", 240, 140, 20, GRAY);

          DrawRectangleRec(textBox, LIGHTGRAY);
          if (mouseOnText) DrawRectangleLines((int)textBox.x, (int)textBox.y, (int)textBox.width, (int)textBox.height, RED);
          else DrawRectangleLines((int)textBox.x, (int)textBox.y, (int)textBox.width, (int)textBox.height, DARKGRAY);

          DrawText(name, (int)textBox.x + 5, (int)textBox.y + 8, 40, MAROON);

          DrawText(TextFormat("INPUT CHARS: %i/%i", letterCount, MAX_INPUT_CHARS), 315, 250, 20, DARKGRAY);

          if (mouseOnText) {
            if (letterCount < MAX_INPUT_CHARS) {
                // Draw blinking underscore char
                if (((framesCounter/20)%2) == 0) DrawText("_", (int)textBox.x + 8 + MeasureText(name, 40), (int)textBox.y + 12, 40, MAROON);
            }
            else DrawText("Press BACKSPACE to delete chars...", 230, 300, 20, GRAY);
          }

          // Test Swedish UTF-8
          // void DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint); // Draw text using font and additional parameters
          DrawTextEx(
             font
            ,R"(Unicode test: Hallå Världen. €'"`)" // UTF-8 text
            ,(Vector2){ 5, screenHeight-80 } // x/column , y/row
            ,32         // font size (pixels)
            ,5          // Spacing (pixels)
            ,DARKGRAY   // tint
          );

          std::string message{};
          for (size_t ix=0;ix<MAX_INPUT_CHARS;++ix) {
            if (name[ix]==0) break;
            message += std::format("<{:x}>",name[ix]);
          }
          DrawText(message.data(),5,screenHeight-40,40,MAROON);

          //----------------------------------------------------------------------------------
          // END key input processing and rendering
          //----------------------------------------------------------------------------------

        //----------------------------------------------------------------------------------
        // END Draw
        //----------------------------------------------------------------------------------
        EndDrawing();
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0; 
  }
} // second