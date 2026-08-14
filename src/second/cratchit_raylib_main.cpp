#include "cratchit_raylib_main.hpp"
#include "log.hpp"
#include "custom_raylib_log_callback.hpp"
#include "utf8.hpp"

// TEA
#include "init.hpp"
#include "view.hpp"
#include "update.hpp"
#include "subscriptions.hpp"
#include "SubHandler.hpp"
#include "CmdHandler.hpp"

#include "enumerate_view.hpp"

char const* const WATERMARK = "CRATCHIT";
char const* const WINDOW_CAPTION = "CRATCHIT";

#define ABC80_AMBER_BRIGHT CLITERAL(Color){255,190,70,255}
#define ABC80_AMBER_NORMAL CLITERAL(Color){230,160,50,255}
#define ABC80_AMBER_DIM    CLITERAL(Color){150,90,20,255}
#define ABC80_AMBER_GLOW   CLITERAL(Color){255,180,60,70}
#define ABC80_CRT_BG       CLITERAL(Color){18,10,0,255}

#define WINDOW_BACGROUND_COLOR  ABC80_CRT_BG
#define PANE_BACKGROUND_COLOR  ABC80_CRT_BG
#define ACTIVE_PANE_FRAME_COLOR ABC80_AMBER_BRIGHT
#define PASSIVE_PANE_FRAME_COLOR ABC80_AMBER_NORMAL
#define TEXT_COLOR ABC80_AMBER_BRIGHT

namespace tea {

  int CratchitRaylibApp::run(int, char**) {
    log_development_trace("Hello from cratchit_raylib_main");

    int posix_result{0};

    //--------------------------------------------------------------------------------------
    // Initialization
    //--------------------------------------------------------------------------------------

    // Set custom logger
    SetTraceLogCallback(custom_raylib_log_callback);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE); // Make InitWindow create a resizeable window

    InitWindow(
      INITIAL_SCREEN_WIDTH
      ,INITIAL_SCREEN_HEIGHT
      ,WINDOW_CAPTION
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

    this->m_current_font = LoadFontEx(
        "resources/NotoSans-Regular.ttf"       // path to true type this->m_current_font file
        ,FONT_HEIGHT                                     // this->m_current_font size height
        ,codepoints.data()                      // array*
        ,static_cast<int>(codepoints.size())    // array element count
    );
    // NOTE: According to AI raylib renders bitmaps for fonts and then scales them when required for rendering in other sizes.
    //       Scaling down should look fine. So if we load to 32 pixel bitmaps we should get good output for sizes < 32?

    if (this->m_current_font.texture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load this->m_current_font");
        // Handle failure here
        // NOTE: raylib logging sugests we may chose to carry on (deafult ASCII this->m_current_font still available?)
        // TraceLog(LOG_ERROR, "Exits - Bye for now");
        log_flush();
        // exit(-1);
    } else {
        TraceLog(LOG_INFO, "Font loaded successfully");
        SetTextureFilter(this->m_current_font.texture, TEXTURE_FILTER_BILINEAR);
    }
    //--------------------------------------------------------------------------------------
    // END: Load and Pre-render bitmap fonts for supported unicode code points 
    //--------------------------------------------------------------------------------------

    // Disable the default "Escape closes the window" behavior
    SetExitKey(KEY_NULL);

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

    SubHandler sub_handler{};
    CmdHandler cmd_handler{};

    //--------------------------------------------------------------------------------------
    // Main render window loop
    //--------------------------------------------------------------------------------------

    // #app
    auto [model,cmd] = app::init();
    cmd_handler.execute(cmd);
    
    while (!WindowShouldClose()) {

      // #TEA::events: Update active events as returned by call to client subscriptions: model -> Subs
      sub_handler.update(subscriptions(model));

      auto this_frame_events_msgs = [&sub_handler]() {

        std::vector<app::Msg> result{};
        
        // #TEA::events: Call subscriptions handler for fired events
        for (auto const& msg : sub_handler.poll()) {
          result.push_back(msg);
        }

        // Poll raylib state for ALL keyboard events
        {
          if (int key = GetCharPressed();key>0) {
            if (key >= ' ') {
              result.push_back(app::Msg{app::UnicodeKeyMsg{key}});
            }
          }
          if (IsKeyPressed(KEY_BACKSPACE)) {
              result.push_back(app::Msg{app::BackspaceKeyMsg{}});
          }
          if (IsKeyPressed(KEY_ENTER)) {
              result.push_back(app::Msg{app::EnterKeyMsg{}});
          }

          if (IsKeyPressed(KEY_ESCAPE)) {
              result.push_back(app::Msg{app::EscapeKeyMsg{}});
          }

        } // Poll for keyboard events

        return result;

      };

      // update model for all events that have occured since last frame
      for (auto const& msg : this_frame_events_msgs()) {
        std::tie(model,cmd) = app::update(model,msg);
        cmd_handler.execute(cmd);
      }

      auto this_frame_cmds_msgs = [&cmd_handler](){
        std::vector<app::Msg> result{};
        for (auto const& msg : cmd_handler.poll()) {
          result.push_back(msg);        
        }
        return result;
      }; // this_frame_cmds_msgs

      for (auto const& msg : this_frame_cmds_msgs()) {
        std::tie(model,cmd) = app::update(model,msg);
        cmd_handler.execute(cmd);
      }

      // #app
      auto ux = app::view(model);

      // #runtime
      this->render(ux);

    } // while window

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return posix_result;
  } // run

  void CratchitRaylibApp::render(tea::Ux const& ux) {
    const int padding{5};
    auto current_screen_width = GetScreenWidth();
    auto current_screen_height = GetScreenHeight();

    auto pane_width = current_screen_width - 2*padding;

    auto bottom_pane_row_count = 3;
    auto bottom_pane_height = bottom_pane_row_count*FONT_HEIGHT + (bottom_pane_row_count+1)*padding;

    auto middle_pane_row_count = 10;
    auto middle_pane_height = middle_pane_row_count*FONT_HEIGHT + (middle_pane_row_count+1)*padding;

    auto top_pane_height = current_screen_height - bottom_pane_height - middle_pane_height - 4*padding;
    // auto top_pane_row_count = top_pane_height / (padding + FONT_HEIGHT);

    Rectangle bottom_pane = { 
        static_cast<float>(padding)                      // Rectangle top-left corner position x (col)
      ,static_cast<float>(current_screen_height - bottom_pane_height - padding)        // Rectangle top-left corner position y (row)
      ,static_cast<float>(pane_width)                    // Rectangle width
      ,static_cast<float>(bottom_pane_height)                     // Rectangle height
    };

    Rectangle middle_pane = { 
        static_cast<float>(padding)                      // Rectangle top-left corner position x (col)
      ,static_cast<float>(bottom_pane.y - middle_pane_height - padding)        // Rectangle top-left corner position y (row)
      ,static_cast<float>(pane_width)                    // Rectangle width
      ,static_cast<float>(middle_pane_height)                     // Rectangle height
    };

    Rectangle top_pane = { 
        static_cast<float>(padding)                      // Rectangle top-left corner position x (col)
      ,static_cast<float>(padding)        // Rectangle top-left corner position y (row)
      ,static_cast<float>(pane_width)                    // Rectangle width
      ,static_cast<float>(top_pane_height)                     // Rectangle height
    };

    //----------------------------------------------------------------------------------
    // BEGIN Update
    //----------------------------------------------------------------------------------

    bool mouse_is_on_top_pane = false;
    bool mouse_is_on_middle_pane = false;
    bool mouse_is_on_bottom_pane = false;

    if (CheckCollisionPointRec(GetMousePosition(), top_pane)) mouse_is_on_top_pane = true;
    else mouse_is_on_top_pane = false;

    if (CheckCollisionPointRec(GetMousePosition(), middle_pane)) mouse_is_on_middle_pane = true;
    else mouse_is_on_middle_pane = false;

    if (CheckCollisionPointRec(GetMousePosition(), bottom_pane)) mouse_is_on_bottom_pane = true;
    else mouse_is_on_bottom_pane = false;

    if (mouse_is_on_bottom_pane) {
        // Set the window's cursor to the I-Beam
        SetMouseCursor(MOUSE_CURSOR_IBEAM);
    }
    else SetMouseCursor(MOUSE_CURSOR_DEFAULT);

    //----------------------------------------------------------------------------------
    // END Update
    //----------------------------------------------------------------------------------

    //----------------------------------------------------------------------------------
    // BEGIN Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
    {

      this->m_frames_counter++;

      ClearBackground(WINDOW_BACGROUND_COLOR);

      //----------------------------------------------------------------------------------
      // BEGIN key input processing and rendering
      //----------------------------------------------------------------------------------

      {
        auto pane = top_pane;
        auto mouse_is_on_pane = mouse_is_on_top_pane;

        auto background_colour = PANE_BACKGROUND_COLOR;
        auto passive_colour = PASSIVE_PANE_FRAME_COLOR;
        auto active_colour = ACTIVE_PANE_FRAME_COLOR;

        DrawRectangleRec(pane, background_colour);
        if (mouse_is_on_pane) DrawRectangleLines((int)pane.x, (int)pane.y, (int)pane.width, (int)pane.height, active_colour);
        else DrawRectangleLines((int)pane.x, (int)pane.y, (int)pane.width, (int)pane.height, passive_colour);
      }
      {
        auto pane = middle_pane;
        auto mouse_is_on_pane = mouse_is_on_middle_pane;

        auto background_colour = PANE_BACKGROUND_COLOR;
        auto passive_colour = PASSIVE_PANE_FRAME_COLOR;
        auto active_colour = ACTIVE_PANE_FRAME_COLOR;

        DrawRectangleRec(pane, background_colour);
        if (mouse_is_on_pane) DrawRectangleLines((int)pane.x, (int)pane.y, (int)pane.width, (int)pane.height, active_colour);
        else DrawRectangleLines((int)pane.x, (int)pane.y, (int)pane.width, (int)pane.height, passive_colour);
      }
      {
        auto pane = bottom_pane;
        auto mouse_is_on_pane = mouse_is_on_bottom_pane;
        auto background_colour = PANE_BACKGROUND_COLOR;
        auto passive_colour = PASSIVE_PANE_FRAME_COLOR;
        auto active_colour = ACTIVE_PANE_FRAME_COLOR;

        DrawRectangleRec(pane, background_colour);
        if (mouse_is_on_pane) DrawRectangleLines((int)pane.x, (int)pane.y, (int)pane.width, (int)pane.height, active_colour);
        else DrawRectangleLines((int)pane.x, (int)pane.y, (int)pane.width, (int)pane.height, passive_colour);
      }

      // Render top pane
      {
        // Render WATERMARK
        {
          auto text_size =  MeasureTextEx( // Font this->m_current_font, const char *text, float fontSize, float spacing
              this->m_current_font
            ,WATERMARK
            ,FONT_HEIGHT
            ,0
          );

          DrawTextEx(
            this->m_current_font                    // this->m_current_font
            ,WATERMARK    // UTF8 chars
            ,Vector2{ 
              top_pane.x + (pane_width - text_size.x)/2     // x (col)
              ,top_pane.y + (top_pane_height - text_size.y)/2        // y (row)
            }
            ,FONT_HEIGHT            // this->m_current_font size (pixels)
            ,0                      // Spacing (pixels)
            ,TEXT_COLOR              // tint
          );
        }

        // Test render row placeholders
        for (auto const& [row_ix,utf8_text] : until_std::views::enumerate(ux.top_pane_rows())) {
          // Render row
          {
            auto pane = top_pane;
            auto text_top_left = Vector2{
                pane.x + padding         // x (col)
              ,pane.y + row_ix*(padding + FONT_HEIGHT)        // y (row)
            };

            auto text_size =  MeasureTextEx( // Font this->m_current_font, const char *text, float fontSize, float spacing
                this->m_current_font
              ,utf8_text.c_str()
              ,FONT_HEIGHT
              ,0
            );

            if (text_top_left.y + text_size.y < top_pane.y + top_pane_height) {
              DrawTextEx(
                this->m_current_font                                  // this->m_current_font
                ,utf8_text.c_str()             // UTF8 chars
                ,text_top_left
                ,FONT_HEIGHT            // this->m_current_font size (pixels)
                ,0                      // Spacing (pixels)
                ,TEXT_COLOR               // tint
              );
            }

          } // render row
        } // for
      } // pane

      // render middle pane
      {

        // Test render row placeholders
        {
          auto pane = middle_pane;
          // auto row_count = middle_pane_row_count;

          for (auto const& [row_ix,utf8_text] : until_std::views::enumerate(ux.middle_pane_rows())) {
            // Render row
            {
              auto text_top_left = Vector2{
                  pane.x + padding         // x (col)
                ,pane.y + row_ix*(padding + FONT_HEIGHT)        // y (row)
              };

              auto text_size =  MeasureTextEx( // Font this->m_current_font, const char *text, float fontSize, float spacing
                this->m_current_font
                ,utf8_text.c_str()
                ,FONT_HEIGHT
                ,0
              );

              if (text_top_left.y + text_size.y < pane.y + pane.height) {
                DrawTextEx(
                  this->m_current_font                                  // this->m_current_font
                  ,utf8_text.c_str()             // UTF8 chars
                  ,text_top_left
                  ,FONT_HEIGHT            // this->m_current_font size (pixels)
                  ,0                      // Spacing (pixels)
                  ,TEXT_COLOR               // tint
                );
              }

            } // render row
          } // for
        }

      } // middle pane

      // Render bottom pane
      {

        auto pane = bottom_pane;
        // auto row_count = bottom_pane_row_count;

        for (auto const& [row_ix,utf8_text] : until_std::views::enumerate(ux.bottom_pane_rows())) {
          // Render row
          {
            auto text_top_left = Vector2{
                pane.x + padding         // x (col)
              ,pane.y + row_ix*(padding + FONT_HEIGHT)        // y (row)
            };

            auto text_size =  MeasureTextEx( // Font this->m_current_font, const char *text, float fontSize, float spacing
              this->m_current_font
              ,utf8_text.c_str()
              ,FONT_HEIGHT
              ,0
            );

            if (text_top_left.y + text_size.y < pane.y + pane.height) {
              DrawTextEx(
                this->m_current_font                                  // this->m_current_font
                ,utf8_text.c_str()             // UTF8 chars
                ,text_top_left
                ,FONT_HEIGHT            // this->m_current_font size (pixels)
                ,0                      // Spacing (pixels)
                ,TEXT_COLOR               // tint
              );
            }

          } // render row
        } // for
      } // bottom pane

      //----------------------------------------------------------------------------------
      // END key input processing and rendering
      //----------------------------------------------------------------------------------

    //----------------------------------------------------------------------------------
    // END Draw
    //----------------------------------------------------------------------------------
    } // anonymous drawing scope
    EndDrawing();

  }

} // tea


