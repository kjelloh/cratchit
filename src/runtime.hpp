#pragma once

#include <functional>
#include <memory>
#include <pugixml.hpp>
#include <map>
#include <ncurses.h>
#include <queue>
#include <format>

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
  const std::string prompt_text = prompt_node.child("label").text().as_string();
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

  WINDOW *middle_win = newwin(section_height, screen_width, section_height, 0);
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

// Event is a key-value-pair
using Event = std::map<std::string,std::string>;

template <typename Msg>
struct Html_Msg {
    pugi::xml_document doc{};
    std::map<std::string,std::function<std::optional<Msg>(Event)>> event_handlers{};
};

template <typename Model, typename Msg, typename Cmd>
class Runtime {
public:
  using Html = Html_Msg<Msg>;
  using init_fn = std::function<std::pair<Model, Cmd>()>;
  using view_fn = std::function<Html(Model)>;
  using update_fn = std::function<std::pair<Model, Cmd>(Model, Msg)>;
  Runtime(init_fn init, view_fn view, update_fn update)
      : m_init(init), m_view(view), m_update(update) {};
  int run(int argc, char *argv[]) {
#ifdef __APPLE__
    // Quick fix to make ncurses find the terminal setting on macOS
    setenv("TERMINFO", "/usr/share/terminfo", 1);
#endif

    int ch = ' '; // Variable to store the user's input
    // init ncurses
    Ncurses ncurses{};

    std::queue<Msg> q{};
    auto [model, cmd] = m_init();
    if (auto optional_msg = cmd())
      q.push(*optional_msg);
    // Main loop
    int loop_count{};
    while (model.stack.size() > 0 and
           not(model.user_input.size() == 1 and ch == 'q')) {
      auto ui = m_view(model);
      render(ui.doc);
      ch = getch();
      if (ui.event_handlers.contains("OnKey")) {
        Event key_event{{"Key",std::to_string(ch)}};
        if (auto optional_msg = ui.event_handlers["OnKey"](key_event)) q.push(*optional_msg);
      }
      else {
        throw std::runtime_error(std::format("DESIGN INSUFFICIENCY, Runtime::run failed to find a binding 'OnKey' from client 'view' function"));
      }
      auto msg = q.front();
      q.pop();
      auto const &[m, cmd] = m_update(model, msg);
      model = m;
      if (auto optional_msg = cmd())
        q.push(*optional_msg);
    }
    return (ch == '-') ? 1 : 0;
  }

private:
  init_fn m_init;
  view_fn m_view;
  update_fn m_update;
};
