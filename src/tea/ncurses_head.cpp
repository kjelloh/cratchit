#include "ncurses_head.hpp"
#include "runtime.hpp"  // For nc::render functions
#include <cstdlib>      // For setenv

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
        
        #ifdef __APPLE__
        // Quick fix to make ncurses find the terminal setting on macOS
        setenv("TERMINFO", "/usr/share/terminfo", 1);
        #endif
        
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        refresh();
        
        m_initialized = true;
    }

    void NCursesHead::render(const pugi::xml_document& doc) {
        if (!m_initialized) {
            return;
        }
        
        nc::render(doc);
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

} // namespace TEA