#include "cratchit.h"
#include <iostream>
#include <map>
#include <memory>
#include <ncurses.h>
#include <pugixml.hpp>
#include <stack>
#include <functional>
#include <queue>

namespace first {

  struct MsgImpl {
    virtual ~MsgImpl() = default;
  };
  struct NCursesKey : public MsgImpl {
    int key;
    NCursesKey(int key) : key{key}, MsgImpl{} {}
  };

  struct Msg {
    std::shared_ptr<MsgImpl> pimpl;   
  };

  using Cmd = std::function<std::optional<Msg>()>;

  std::optional<Msg> Nop() {
    return std::nullopt;
  };

  struct Model {
    std::string top_content;
    std::string main_content;
    std::string user_input;

    std::map<int, std::map<char, int>> adj_list{};
    std::vector<std::string> id2node{};
    std::stack<int> stack{};

    int id_of(std::string const &node) {
      auto iter = std::find(id2node.begin(), id2node.end(), node);
      if (iter != id2node.end())
        return std::distance(id2node.begin(), iter);
      int id = id2node.size();
      id2node.push_back(node);
      return id;
    }

    void add_transition(std::string const &from, char ch,
                        std::string const &to) {
      adj_list[id_of(from)][ch] = id_of(to);
    }
  };

  std::pair<Model,Cmd> init() {
    Model model = {"Welcome to the top section",
                   "This is the main content area", ""};
    model.add_transition("root", '1', "ITfied");
    model.add_transition("ITfied", '1', "2024-2025");
    model.add_transition("2024-2025", '1', "Rubrik Belopp Datum");
    model.add_transition("2024-2025", '2', "Momsrapport");
    model.add_transition("Momsrapport", '1', "Q1");
    model.add_transition("Momsrapport", '2', "Q2");
    model.add_transition("Momsrapport", '3', "Q3");
    model.add_transition("Momsrapport", '4', "Q4");
    model.stack.push(model.id_of("root"));
    return {model,Nop};
  }

  std::pair<Model,Cmd> update(Model model, Msg msg) {
    auto key_msg_ptr = std::dynamic_pointer_cast<NCursesKey>(msg.pimpl);
    if (key_msg_ptr != nullptr) {
      auto ch = key_msg_ptr->key; 
      if (ch == KEY_BACKSPACE || ch == 127) { // Handle backspace
        if (!model.user_input.empty()) {
          model.user_input.pop_back();
        }
      } 
      else if (ch == '\n') {
        // User pressed Enter: process command (optional)
        model.user_input.clear(); // Reset input after submission
      } 
      else {
        if (model.user_input.empty() and model.stack.size() > 0) {
          if (ch == '-') {
            model.stack.pop();
          } 
          else if (model.adj_list[model.stack.top()].contains(ch)) {
            model.stack.push(model.adj_list[model.stack.top()][ch]);
          }
          else {
            model.user_input += ch; // Append typed character
          }
        }
        else {
            model.user_input += ch; // Append typed character
        }
      }
      if (model.stack.size() > 0) {
        model.main_content.clear();
        for (auto const &[ch, to] : model.adj_list[model.stack.top()]) {
          std::string entry{};
          entry.push_back(ch);
          entry.append(" - ");
          auto caption = model.id2node[to];
          entry.append(caption);
          entry.push_back('\n');
          model.main_content.append(entry);
        }
      }

    }
    return {model,Nop}; // Return updated model
  }

  pugi::xml_document view(const Model &model) {
    // Create a new pugi document
    pugi::xml_document doc;

    // Note: HTML doc may be tested for validity at:
    // https://www.w3schools.com/html/tryit.asp?filename=tryhtml_intro

    // Create the root HTML element
    pugi::xml_node html = doc.append_child("html");

    // Create the body
    pugi::xml_node body = html.append_child("body");

    // Create the top section with class "content"
    pugi::xml_node top = body.append_child("div");
    top.append_attribute("class") = "content";
    top.text().set(model.top_content.c_str());

    // Create the main section with class "content"
    pugi::xml_node main = body.append_child("div");
    main.append_attribute("class") = "content";
    main.text().set(model.main_content.c_str());

    // Create the user prompt section with class "user-prompt"
    pugi::xml_node prompt = body.append_child("div");
    prompt.append_attribute("class") = "user-prompt";
    // Add a label element for the prompt text
    pugi::xml_node label = prompt.append_child("label");
    label.text().set((">" + model.user_input).c_str());

    // Make prompt 'html-correct' (even though render does not care for now)
    pugi::xml_node input = prompt.append_child("input");
    input.append_attribute("type") = "text";
    input.append_attribute("id") = "command";
    input.append_attribute("name") = "command";

    return doc;
  }

  void render_section(WINDOW *win, const std::string &text, int start_y,
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
    wrefresh(win); // Refresh to show changes in the window
  }

  void render_prompt(WINDOW *win, const pugi::xml_node &prompt_node) {
    // User prompt at the bottom of the screen (in the last row)
    const std::string prompt_text =
        prompt_node.child("label").text().as_string();
    mvwprintw(win, 1, 1, "%s", prompt_text.c_str());
    wmove(win, 1, prompt_text.size() + 1); // Move cursor after the prompt
    wrefresh(win);                         // Refresh to show prompt
  }

  // Renders doc as HTML to ncurses screen
  // Note: HTML doc semantics may be tested at:
  // https://www.w3schools.com/html/tryit.asp?filename=tryhtml_intro
  void render(const pugi::xml_document &doc) {
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
    wrefresh(top_win);  // Refresh to show the window

    WINDOW *middle_win =
        newwin(section_height, screen_width, section_height, 0);
    box(middle_win, 0, 0); // Draw border around the middle section
    wrefresh(middle_win);  // Refresh to show the window

    WINDOW *bottom_win =
        newwin(section_height, screen_width, 2 * section_height, 0);
    box(bottom_win, 0, 0); // Draw border around the bottom section
    wrefresh(bottom_win);  // Refresh to show the window

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
  }

  class Ncurses {
  public:
    Ncurses() {
      initscr();
      cbreak();
      noecho();
      keypad(stdscr, TRUE);
    }
    ~Ncurses() {
      endwin(); // End ncurses mode
    }
  };

  int main(int argc, char *argv[]) {
#ifdef __APPLE__
    // Quick fix to make ncurses find the terminal setting on macOS
    setenv("TERMINFO", "/usr/share/terminfo", 1);
#endif

    int ch = ' '; // Variable to store the user's input
    // init ncurses
    Ncurses ncurses{};

    std::queue<Msg> q{};
    auto [model,cmd] = init();
    if (auto msg = cmd()) q.push(*msg);
    // Main loop
    int loop_count{};
    while (model.stack.size() > 0 and
          not(model.user_input.size() == 1 and ch == 'q')) {
      pugi::xml_document doc = view(model);
      render(doc);
      ch = getch();
      q.push(Msg{std::make_shared<NCursesKey>(ch)});
      auto msg = q.front(); q.pop();
      auto const& [m,cmd] = update(model,msg);
      model = m;
      if (auto msg = cmd()) q.push(*msg);
    }
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
