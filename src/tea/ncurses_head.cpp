#include "ncurses_head.hpp"
#include "runtime.hpp"  // For nc::render functions
#include <cstdlib>      // For setenv
#include <locale.h>     // For setlocale
#include <clocale>      // For LC_ALL
#include "spdlog/spdlog.h"

namespace TEA {

  // NCurses
  namespace nc {

    inline void render_section(WINDOW *win, const std::string &text, int start_y,
                        int max_lines) {
      int line_count = 0;
      size_t pos = 0;
      while (pos < text.size() && line_count < max_lines) {
        size_t next_line_pos = text.find('\n', pos);
        if (next_line_pos == std::string::npos) {
          next_line_pos = text.size();
        }

        std::string line = text.substr(pos, next_line_pos - pos);
        mvwprintw(win, start_y + line_count, 1, "%s",
                  line.c_str()); // Render inside window

        line_count++;
        pos = next_line_pos + 1; // Move to the next line
      }
      wnoutrefresh(win); // update to buffer
    }

    inline void render_prompt(WINDOW *win, const pugi::xml_node &prompt_node) {
      int line = 1; // Start at line 1 (after border)
      
      // Render breadcrumb on line 1
      auto breadcrumb_node = prompt_node.child("div");
      if (breadcrumb_node && std::string(breadcrumb_node.attribute("class").as_string()) == "breadcrumb") {
        const std::string breadcrumb_text = breadcrumb_node.text().as_string();
        mvwprintw(win, line, 1, "%s", breadcrumb_text.c_str());
        line++;
      }
      
      // Skip line 2 (empty spacer line)
      line++;
      
      // Render user prompt on line 3
      const std::string prompt_text = prompt_node.child("label").text().as_string();
      mvwprintw(win, line, 1, "%s", prompt_text.c_str());
      
      // Use visual width for proper UTF-8 cursor positioning
      auto label_node = prompt_node.child("label");
      int visual_width = label_node.attribute("visual-width").as_int(prompt_text.size() + 1);
      wmove(win, line, visual_width + 1); // Move cursor after the prompt
      wnoutrefresh(win); // Update to buffer
    }

    // Renders doc as HTML to ncurses screen
    // Note: HTML doc semantics may be tested at:
    // https://www.w3schools.com/html/tryit.asp?filename=tryhtml_intro
    inline void render(const pugi::xml_document &doc) {
      int screen_height, screen_width;
      getmaxyx(stdscr, screen_height, screen_width); // Get screen dimensions

      // Create a window for the app screen area (excluding the last row for the
      // prompt)
      int app_screen_height =
          screen_height - 1; // Excluding the bottom row for the prompt
      int section_height =
          app_screen_height /
          3; // Divide the remaining screen height into three sections

      // Create windows for the app screen sections
      WINDOW *top_win = newwin(section_height, screen_width, 0, 0);
      box(top_win, 0, 0); // Draw border around the top section

      WINDOW *middle_win = newwin(section_height, screen_width, section_height, 0);
      box(middle_win, 0, 0); // Draw border around the middle section

      WINDOW *bottom_win =
          newwin(section_height, screen_width, 2 * section_height, 0);
      box(bottom_win, 0, 0); // Draw border around the bottom section

      // Parse the HTML-like structure
      pugi::xml_node html = doc.child("html");
      pugi::xml_node body = html.child("body");

      int current_y = 1; // Start from row 1 to leave space for the top border
      int num_divs = 0;

      // Loop through divs directly and render them in sections
      for (auto const &div : body.children("div")) {
        // Render the content of the div inside the windows
        const std::string text = div.text().as_string();
        const int max_lines = section_height - 2; // Accounting for borders

        if (div.attribute("class").as_string() == std::string("content")) {
          if (num_divs == 0) {
            render_section(top_win, text, current_y, max_lines);
          } else if (num_divs == 1) {
            render_section(middle_win, text, current_y, max_lines);
          }
        } else if (div.attribute("class").as_string() ==
                  std::string("user-prompt")) {
          render_prompt(bottom_win, div);
        }
        num_divs++;
      }
      doupdate();
    }

    inline void ncurses_cleanup_on_crash(int sig) {
        endwin();  // Reset terminal to normal
        // 20250616/KOH: This will NOT restore any ASAN linked machinery
        std::signal(sig, SIG_DFL); // Restore default handler
        std::raise(sig);           // Re-raise signal
    }

    class Ncurses {
    public:
      Ncurses() {
        // 20250616/KOH: Registring this signal handler disables any ASAN linked machinery
        // std::signal(SIGSEGV, ncurses_cleanup_on_crash);
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        refresh();
      }
      ~Ncurses() {
        endwin(); // End ncurses mode
      }
    };

  } // render
} // TEA

namespace TEA {

    NCursesHead::~NCursesHead() {
        if (m_initialized) {
            cleanup();
        }
    }

    void NCursesHead::initialize() {
        if (m_initialized) {
            return;
        }
                
        spdlog::info("NCursesHead: Initializing with target encoding: {}", 
                     this->m_runtime_endoding.get_encoding_display_name());
        
        // Setup encoding support
        bool encoding_ok = setup_encoding_support();
        
        #ifdef __APPLE__
        // Quick fix to make ncurses find the terminal setting on macOS
        setenv("TERMINFO", "/usr/share/terminfo", 1);
        #endif
        
        // Initialize NCurses
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        refresh();
        
        if (!encoding_ok) {
            spdlog::warn("NCursesHead: Encoding setup failed, will display error overlay");
        }
        
        m_initialized = true;
    }

    void NCursesHead::render(const pugi::xml_document& doc) {
        if (!m_initialized) {
            return;
        }
        
        // Render the main content
        nc::render(doc);
        
        // Overlay encoding error if needed
        render_encoding_error_overlay();
    }

    int NCursesHead::get_input() {
      if (!m_initialized) {
          return 'q';  // Default to quit if not initialized
      }

      while (true) {
        auto nc_key = getch();

        switch (m_runtime_endoding.detected_encoding()) {
          case encoding::icu::DetectedEncoding::UTF8: {
            // transform utf-8 to unicode code point
            if (auto cp = m_utf8_to_unicode_buffer.push(nc_key)) {
              return *cp;
            }
          } break;
          default: {
            return nc_key; // default single char code points
          }
        }
      }
    }

    void NCursesHead::cleanup() {
        if (m_initialized) {
            endwin();  // End ncurses mode
            m_initialized = false;
        }
    }

    bool NCursesHead::setup_encoding_support() {
        switch (m_runtime_endoding.detected_encoding()) {
            case encoding::icu::DetectedEncoding::UTF8: {
                // Try to set UTF-8 locale using setlocale approach
                spdlog::info("NCursesHead: Setting up UTF-8 support using setlocale");
                
                // Try different UTF-8 locale variants
                const char* utf8_locales[] = {
                    "en_US.UTF-8",  // Common on macOS/Linux
                    "C.UTF-8",      // Common on Linux
                    "UTF-8",        // Simplified
                    ""              // System default
                };
                
                bool locale_set = false;
                for (const char* locale : utf8_locales) {
                    if (setlocale(LC_ALL, locale)) {
                        spdlog::info("NCursesHead: Successfully set locale to: {}", locale);
                        locale_set = true;
                        break;
                    }
                }
                
                if (!locale_set) {
                    spdlog::error("NCursesHead: Failed to set any UTF-8 locale");
                    m_encoding_support = EncodingSupport::ASCII_FALLBACK;
                    m_encoding_error_message = "UTF-8 locale setup failed - using ASCII fallback";
                    return false;
                }
                
                m_encoding_support = EncodingSupport::FULL_SUPPORT;
                return true;
            }
            
            case encoding::icu::DetectedEncoding::ISO_8859_1:
            case encoding::icu::DetectedEncoding::CP437: {
                spdlog::warn("NCursesHead: Non-UTF8 encoding requested: {}, falling back to ASCII", 
                           m_runtime_endoding.get_encoding_display_name());
                m_encoding_support = EncodingSupport::ASCII_FALLBACK;
                m_encoding_error_message = "Non-UTF8 encoding not supported - using ASCII fallback";
                return false;
            }
            
            default: {
                spdlog::error("NCursesHead: Unsupported encoding: {}", 
                            m_runtime_endoding.get_encoding_display_name());
                m_encoding_support = EncodingSupport::FAILED;
                m_encoding_error_message = "Unsupported encoding - terminal display may be corrupted";
                return false;
            }
        }
    }

    void NCursesHead::render_encoding_error_overlay() {
        if (m_encoding_support == EncodingSupport::FULL_SUPPORT) {
            return; // No error to display
        }
        
        // Get terminal dimensions
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        
        // Calculate position for error message (bottom of screen)
        int error_y = max_y - 3;
        if (error_y < 0) error_y = 0;
        
        // Clear the error area and display ASCII-safe error message
        move(error_y, 0);
        clrtoeol();
        
        // Display encoding status in ASCII
        std::string status_line;
        switch (m_encoding_support) {
            case EncodingSupport::ASCII_FALLBACK:
                status_line = "ENCODING WARNING: " + m_encoding_error_message;
                break;
            case EncodingSupport::FAILED:
                status_line = "ENCODING ERROR: " + m_encoding_error_message;
                break;
            default:
                return;
        }
        
        // Truncate message if too long for terminal
        if (status_line.length() > static_cast<size_t>(max_x - 1)) {
            status_line = status_line.substr(0, max_x - 4) + "...";
        }
        
        // Use only basic printw for ASCII safety
        mvprintw(error_y, 0, "%s", status_line.c_str());
        
        // Add a separator line
        move(error_y + 1, 0);
        for (int i = 0; i < max_x - 1; ++i) {
            addch('-');
        }
        
        refresh();
    }

} // namespace TEA