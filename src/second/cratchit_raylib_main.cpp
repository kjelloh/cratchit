#include "cratchit_raylib_main.hpp"
#include "log.hpp"
#include "raylib.h" // See https://www.raylib.com/cheatsheet/cheatsheet.html, https://github.com/raysan5/raylib
#include "custom_raylib_log_callback.hpp"
#include "utf8.hpp"

char const* const WATERMARK = "CRATCHIT";

int cratchit_raylib_main(int argc, char *argv[]) {
  log_development_trace("Hello from cratchit_raylib_main");

  int posix_result{0};

  //--------------------------------------------------------------------------------------
  // Initialization
  //--------------------------------------------------------------------------------------

  // Set custom logger
  SetTraceLogCallback(custom_raylib_log_callback);

  const int INITIAL_SCREEN_WIDTH = 800;
  const int INITIAL_SCREEN_HEIGHT = 450;

  SetConfigFlags(FLAG_WINDOW_RESIZABLE); // Make InitWindow create a resizeable window

  InitWindow(
     INITIAL_SCREEN_WIDTH
    ,INITIAL_SCREEN_HEIGHT
    ,"raylib [core] example - basic window"
  );

  //--------------------------------------------------------------------------------------
  // BEGIN: Load and Pre-render bitmap fonts for supported unicode code points 
  // MUST be done a f t e r InitWindow.
  // Also see: https://github.com/raysan5/raylib/blob/master/examples/text/text_unicode_ranges.c
  //--------------------------------------------------------------------------------------

  // raylib utilises a vector of unicode code points to map to a rendered bitmap for that 'charachter'
  // A unicde code point is in the range 0..17x0xFFFF (https://en.wikipedia.org/wiki/Code_point)
  // raylib uses signed int for code point values
  std::vector<int> codepoints{};

  // Make room for supported unicode code blocks
  // See https://en.wikipedia.org/wiki/Unicode_block

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

  const int FONT_HEIGHT = 32;

  Font font = LoadFontEx(
      "resources/NotoSans-Regular.ttf"       // path to true type font file
      ,FONT_HEIGHT                                     // font size height
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
  //--------------------------------------------------------------------------------------
  // END: Load and Pre-render bitmap fonts for supported unicode code points 
  //--------------------------------------------------------------------------------------

  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
  //--------------------------------------------------------------------------------------
  // BEGIN Text input mechanism
  //--------------------------------------------------------------------------------------
  std::vector<int> code_point_buffer{};

  bool mouseOnText = false;

  int framesCounter = 0;    
  //--------------------------------------------------------------------------------------
  // END Text input mechanism
  //--------------------------------------------------------------------------------------

  //--------------------------------------------------------------------------------------
  // Main render window loop
  //--------------------------------------------------------------------------------------
  while (!WindowShouldClose()) {

      const int padding{5};
      auto current_screen_width = GetScreenWidth();
      auto current_screen_height = GetScreenHeight();

      auto pane_width = current_screen_width - 2*padding;
      auto top_pane_row_count = 10;
      auto top_pane_height = top_pane_row_count*FONT_HEIGHT + (top_pane_row_count+1)*padding;
      auto bottom_pane_row_count = 3;
      auto bottom_pane_height = bottom_pane_row_count*FONT_HEIGHT + (bottom_pane_row_count+1)*padding;
  
      Rectangle top_pane = { 
         static_cast<float>(padding)                      // Rectangle top-left corner position x (col)
        ,static_cast<float>(padding)        // Rectangle top-left corner position y (row)
        ,static_cast<float>(pane_width)                    // Rectangle width
        ,static_cast<float>(top_pane_height)                     // Rectangle height
      };

      Rectangle bottom_pane = { 
         static_cast<float>(padding)                      // Rectangle top-left corner position x (col)
        ,static_cast<float>(current_screen_height - bottom_pane_height - 2*padding)        // Rectangle top-left corner position y (row)
        ,static_cast<float>(pane_width)                    // Rectangle width
        ,static_cast<float>(bottom_pane_height)                     // Rectangle height
      };

      //----------------------------------------------------------------------------------
      // BEGIN Update
      //----------------------------------------------------------------------------------
      if (CheckCollisionPointRec(GetMousePosition(), bottom_pane)) mouseOnText = true;
      else mouseOnText = false;

      if (int key = GetCharPressed();key>0) {
        if (key >= ' ') code_point_buffer.push_back(key);
      }
      if (IsKeyPressed(KEY_BACKSPACE)) {
          if (code_point_buffer.size() > 0) code_point_buffer.pop_back();
      }

      if (mouseOnText) {
          // Set the window's cursor to the I-Beam
          SetMouseCursor(MOUSE_CURSOR_IBEAM);
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

        //----------------------------------------------------------------------------------
        // BEGIN key input processing and rendering
        //----------------------------------------------------------------------------------

        DrawRectangleRec(top_pane, LIGHTGRAY);
        DrawRectangleRec(bottom_pane, LIGHTGRAY);

        if (mouseOnText) DrawRectangleLines((int)bottom_pane.x, (int)bottom_pane.y, (int)bottom_pane.width, (int)bottom_pane.height, RED);
        else DrawRectangleLines((int)bottom_pane.x, (int)bottom_pane.y, (int)bottom_pane.width, (int)bottom_pane.height, DARKGRAY);

        // Render WATERMARK
        {
          auto text_size =  MeasureTextEx( // Font font, const char *text, float fontSize, float spacing
              font
            ,WATERMARK
            ,FONT_HEIGHT
            ,0
          );

          DrawTextEx(
            font                    // font
            ,WATERMARK    // UTF8 chars
            ,Vector2{ 
               top_pane.x + (pane_width - text_size.x)/2     // x (col)
              ,top_pane.y + (top_pane_height - text_size.y)/2        // y (row)
            }
            ,FONT_HEIGHT            // font size (pixels)
            ,0                      // Spacing (pixels)
            ,DARKGRAY              // tint
          );
        }

        // Render hex values of read unicode code point characters (development trace)
        {
          auto row_ix = 0;

          std::string unicode_hex_message{"UNICODE:"};
          for (auto const& code_point : code_point_buffer) {
            unicode_hex_message += std::format("<{:X}>",static_cast<uint32_t>(code_point));
          }
          DrawTextEx(
            font                                  // font
            ,unicode_hex_message.c_str()             // UTF8 chars
            ,(Vector2){ 
               bottom_pane.x + padding         // x (col)
              ,bottom_pane.y + row_ix*(padding + FONT_HEIGHT)        // y (row)
            }
            ,FONT_HEIGHT            // font size (pixels)
            ,0                      // Spacing (pixels)
            ,MAROON               // tint
          );
        }

        // Render hex values of read unicode code point characters (development trace)
        {
          auto row_ix = 1;

          std::string utf8_hex_message{"UTF8:"};
          for (auto const& code_point : code_point_buffer) {
            auto utf8_bytes = unicode_to_utf8(static_cast<uint32_t>(code_point));
            for (auto const& utf8_byte : utf8_bytes) {
              utf8_hex_message += std::format("<{:X}>",utf8_byte);
            }
          }
          DrawTextEx(
            font                                  // font
            ,utf8_hex_message.c_str()             // UTF8 chars
            ,(Vector2){ 
               bottom_pane.x + padding         // x (col)
              ,bottom_pane.y + row_ix*(padding + FONT_HEIGHT)        // y (row)
            }
            ,FONT_HEIGHT            // font size (pixels)
            ,0                      // Spacing (pixels)
            ,MAROON               // tint
          );
        }

        // Render user input text
        {
          auto row_ix = 2;

          //----------------------------------------------------------------------------------
          // BEGIN Unicode to UTF8 string
          //----------------------------------------------------------------------------------

          std::string utf8_string{">"};
          for (auto const& code_point : code_point_buffer) {
            auto utf8_bytes = unicode_to_utf8(static_cast<uint32_t>(code_point));
            for (auto utf8_byte : utf8_bytes) utf8_string += static_cast<char>(utf8_byte);
          }

          //----------------------------------------------------------------------------------
          // END Unicode to UTF8 string
          //----------------------------------------------------------------------------------


          DrawTextEx(
            font                    // font
            ,utf8_string.c_str()    // UTF8 chars
            ,(Vector2){ 
              bottom_pane.x + padding         // x (col)
              ,bottom_pane.y + row_ix*(padding + FONT_HEIGHT)        // y (row)
            }
            ,FONT_HEIGHT            // font size (pixels)
            ,0                      // Spacing (pixels)
            ,DARKGRAY               // tint
          );
          if (mouseOnText) {
            if (((framesCounter/20)%2) == 0) {
              auto text_size =  MeasureTextEx( // Font font, const char *text, float fontSize, float spacing
                font
                ,utf8_string.c_str()
                ,FONT_HEIGHT
                ,0
              );
              DrawTextEx(
                font                    // font
                ,"_"    // UTF8 chars
                ,Vector2{ 
                   bottom_pane.x + text_size.x + padding         // x (col)
                  ,bottom_pane.y + row_ix*(padding + FONT_HEIGHT)        // y (row)
                }
                ,FONT_HEIGHT            // font size (pixels)
                ,0                      // Spacing (pixels)
                ,MAROON               // tint
              );
            }
          }
        }

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

  return posix_result;
}
