#include "ncurses_head.hpp"
#include "runtime.hpp"  // For nc::render functions
#include <cstdlib>      // For setenv
#include <locale.h>     // For setlocale
#include <clocale>      // For LC_ALL
#include "spdlog/spdlog.h"

namespace TEA {

    NCursesHead::~NCursesHead() {
        if (m_initialized) {
            cleanup();
        }
    }

    void NCursesHead::initialize(RuntimeEncoding::DetectedEncoding target_encoding) {
        if (m_initialized) {
            return;
        }
        
        // Store the target encoding
        m_target_encoding = target_encoding;
        
        spdlog::info("NCursesHead: Initializing with target encoding: {}", 
                     encoding::icu::EncodingDetector::enum_to_display_name(target_encoding));
        
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
        
        return getch();
    }

    void NCursesHead::cleanup() {
        if (m_initialized) {
            endwin();  // End ncurses mode
            m_initialized = false;
        }
    }

    bool NCursesHead::setup_encoding_support() {
        switch (m_target_encoding) {
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
                           encoding::icu::EncodingDetector::enum_to_display_name(m_target_encoding));
                m_encoding_support = EncodingSupport::ASCII_FALLBACK;
                m_encoding_error_message = "Non-UTF8 encoding not supported - using ASCII fallback";
                return false;
            }
            
            default: {
                spdlog::error("NCursesHead: Unsupported encoding: {}", 
                            encoding::icu::EncodingDetector::enum_to_display_name(m_target_encoding));
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