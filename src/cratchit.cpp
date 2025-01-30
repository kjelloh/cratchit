#include "cratchit.h"
#include <iostream>
#include <ncurses.h>
#include <pugixml.hpp>

namespace first {

  struct Model {
    std::string top_content;
    std::string main_content;
    std::string bottom_content;
  };

  pugi::xml_document view(const Model &model) {
    // Create a new pugi document
    pugi::xml_document doc;

    // Create the root HTML element
    pugi::xml_node html = doc.append_child("html");

    // Create the body
    pugi::xml_node body = html.append_child("body");

    // Create the top section
    pugi::xml_node top = body.append_child("div");
    top.append_attribute("id") = "top";
    top.text().set(model.top_content.c_str());

    // Create the main section
    pugi::xml_node main = body.append_child("div");
    main.append_attribute("id") = "main";
    main.text().set(model.main_content.c_str());

    // Create the bottom section
    pugi::xml_node bottom = body.append_child("div");
    bottom.append_attribute("id") = "bottom";
    bottom.text().set(model.bottom_content.c_str());

    return doc;
  }

  void render(const pugi::xml_document &doc) {
    int screen_height, screen_width;
    getmaxyx(stdscr, screen_height, screen_width); // Get screen dimensions

    // Parse the HTML structure
    pugi::xml_node html = doc.child("html");
    pugi::xml_node body = html.child("body");

    // First, count how many div elements we have
    int num_divs = 0;
    for (auto const &div : body.children("div")) {
      num_divs++;
    }

    // Calculate the height for each section (divide evenly among divs)
    int section_height = screen_height / num_divs;

    // Function to render a section of text
    auto render_section = [&](const std::string &text, int start_y,
                              int max_lines) {
      int line_count = 0;
      size_t pos = 0; // Position within the text content (not the terminal row)
      // Split the text into lines and render each line
      while (pos < text.size() && line_count < max_lines) {
        size_t next_line_pos = text.find('\n', pos);
        if (next_line_pos == std::string::npos) {
          next_line_pos = text.size();
        }

        // Extract the line of text
        std::string line = text.substr(pos, next_line_pos - pos);

        // Print the line at the current terminal row (start_y + line_count)
        mvprintw(start_y + line_count, 0, "%s", line.c_str());

        line_count++;
        pos = next_line_pos + 1; // Move to the next line in the text
      }
    };

    // Loop through divs directly and render them
    int current_y =
        0; // Track the current Y position (terminal row) for rendering
    for (auto const &div : body.children("div")) {
      // Render the content of the div, filling `section_height` lines of the
      // terminal
      render_section(div.text().as_string(), current_y, section_height);
      current_y +=
          section_height; // Update current Y position for the next section
    }

    // Refresh to show the rendered content
    refresh();
  }

  int main(int argc, char *argv[]) {
#ifdef __APPLE__
    // Quick fix to make ncurses find the terminal setting on macOS
    setenv("TERMINFO", "/usr/share/terminfo", 1);
#endif
    //
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // Define the model with content for each section
    Model model = {"Welcome to the top section",
                   "This is the main content area",
                   "This is the bottom section"};

    char ch = ' '; // Variable to store the user's input

    // Main loop
    int loop_count{};
    while (ch != 'q' && ch != '-') {
      pugi::xml_document doc = view(model);
      render(doc);
      printw("Enter 'q' to quit, '-' to switch to zeroth cratchit");
      printw("\n%d:first:>%c", loop_count++, ch);
      refresh();
      ch = getch(); // Get a character input from the user
    }

    endwin(); // End ncurses mode
    return (ch == '-') ? 1 : 0;
  }
} // namespace first

void cratchit() {

#ifdef NDEBUG
  std::cout << "cratchit/0.6: Hello World Release!\n";
#else
  std::cout << "cratchit/0.6: Hello World Debug!\n";
#endif

// ARCHITECTURES
#ifdef _M_X64
  std::cout << "  cratchit/0.6: _M_X64 defined\n";
#endif

#ifdef _M_IX86
  std::cout << "  cratchit/0.6: _M_IX86 defined\n";
#endif

#ifdef _M_ARM64
  std::cout << "  cratchit/0.6: _M_ARM64 defined\n";
#endif

#if __i386__
  std::cout << "  cratchit/0.6: __i386__ defined\n";
#endif

#if __x86_64__
  std::cout << "  cratchit/0.6: __x86_64__ defined\n";
#endif

#if __aarch64__
  std::cout << "  cratchit/0.6: __aarch64__ defined\n";
#endif

// Libstdc++
#if defined _GLIBCXX_USE_CXX11_ABI
  std::cout << "  cratchit/0.6: _GLIBCXX_USE_CXX11_ABI "
            << _GLIBCXX_USE_CXX11_ABI << "\n";
#endif

// MSVC runtime
#if defined(_DEBUG)
#if defined(_MT) && defined(_DLL)
  std::cout << "  cratchit/0.6: MSVC runtime: MultiThreadedDebugDLL\n";
#elif defined(_MT)
  std::cout << "  cratchit/0.6: MSVC runtime: MultiThreadedDebug\n";
#endif
#else
#if defined(_MT) && defined(_DLL)
  std::cout << "  cratchit/0.6: MSVC runtime: MultiThreadedDLL\n";
#elif defined(_MT)
  std::cout << "  cratchit/0.6: MSVC runtime: MultiThreaded\n";
#endif
#endif

// COMPILER VERSIONS
#if _MSC_VER
  std::cout << "  cratchit/0.6: _MSC_VER" << _MSC_VER << "\n";
#endif

#if _MSVC_LANG
  std::cout << "  cratchit/0.6: _MSVC_LANG" << _MSVC_LANG << "\n";
#endif

#if __cplusplus
  std::cout << "  cratchit/0.6: __cplusplus" << __cplusplus << "\n";
#endif

#if __INTEL_COMPILER
  std::cout << "  cratchit/0.6: __INTEL_COMPILER" << __INTEL_COMPILER << "\n";
#endif

#if __GNUC__
  std::cout << "  cratchit/0.6: __GNUC__" << __GNUC__ << "\n";
#endif

#if __GNUC_MINOR__
  std::cout << "  cratchit/0.6: __GNUC_MINOR__" << __GNUC_MINOR__ << "\n";
#endif

#if __clang_major__
  std::cout << "  cratchit/0.6: __clang_major__" << __clang_major__ << "\n";
#endif

#if __clang_minor__
  std::cout << "  cratchit/0.6: __clang_minor__" << __clang_minor__ << "\n";
#endif

#if __apple_build_version__
  std::cout << "  cratchit/0.6: __apple_build_version__"
            << __apple_build_version__ << "\n";
#endif

  // SUBSYSTEMS

#if __MSYS__
  std::cout << "  cratchit/0.6: __MSYS__" << __MSYS__ << "\n";
#endif

#if __MINGW32__
  std::cout << "  cratchit/0.6: __MINGW32__" << __MINGW32__ << "\n";
#endif

#if __MINGW64__
  std::cout << "  cratchit/0.6: __MINGW64__" << __MINGW64__ << "\n";
#endif

#if __CYGWIN__
  std::cout << "  cratchit/0.6: __CYGWIN__" << __CYGWIN__ << "\n";
#endif
}

void cratchit_print_vector(const std::vector<std::string> &strings) {
  for (std::vector<std::string>::const_iterator it = strings.begin();
       it != strings.end(); ++it) {
    std::cout << "cratchit/0.6 " << *it << std::endl;
  }
}
