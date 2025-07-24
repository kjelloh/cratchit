#pragma once

#include "event.hpp" // Event
#include "to_type_name.hpp"
#include "head.hpp"
#include "RuntimeEncoding.hpp"
#include <functional>
#include <memory>
#include <pugixml.hpp>
#include <map>
#include <ncurses.h>
#include <queue>
#include <format>
#include <spdlog/spdlog.h>
#include <csignal> // std::signal

// The Elm Aarchitecture
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
      wmove(win, line, prompt_text.size() + 1); // Move cursor after the prompt
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

  namespace runtime {
    template <typename Msg>
    using IsQuit = std::function<bool(Msg)>;
  }

  template <typename Msg>
  struct Html_Msg {
      pugi::xml_document doc{};
      std::map<std::string,std::function<std::optional<Msg>(Event)>> event_handlers{};
  };

  template <typename Model, typename Msg, typename Cmd>
  class Runtime {
  public:
    using init_result = std::tuple<Model,runtime::IsQuit<Msg>,Cmd>;
    using init_fn = std::function<init_result()>;
    using view_result = Html_Msg<Msg>;
    using view_fn = std::function<view_result(Model)>;
    using model_update_result = std::pair<Model, Cmd>;
    using update_fn = std::function<model_update_result(Model, Msg)>;

    Runtime(init_fn init, view_fn view, update_fn update, std::unique_ptr<Head> head)
        : m_init(init), m_view(view), m_update(update), m_head(std::move(head)) {};

    int run(int argc, char *argv[]) {
      spdlog::info("Runtime::run - BEGIN");

      int ch = ' '; // Variable to store the user's input
      
      // Initialize the head (UI system) with encoding from RuntimeEncoding
      auto target_encoding = RuntimeEncoding::get_assumed_terminal_encoding();
      spdlog::info("TEA::Runtime: Initializing head with encoding: {}", 
                   RuntimeEncoding::get_encoding_display_name());
      m_head->initialize(target_encoding);

      std::queue<Msg> msg_q{};
      std::queue<Cmd> cmd_q{};
      auto [model, is_quit_msg, cmd] = m_init();
      cmd_q.push(cmd);
      // Main loop
      int loop_count{};
      while (true) {

        spdlog::info("Runtime::run loop_count: {}, cmd_q size: {}, msg_q size: {}", loop_count,cmd_q.size(), msg_q.size());

        // render the ux
        auto ui = m_view(model);
        m_head->render(ui.doc);

        if (not cmd_q.empty()) {
          // Execute a command
          auto cmd = cmd_q.front();
          cmd_q.pop();
          if (cmd) {
            spdlog::info("Runtime::run: Processing Cmd type {}", to_type_name(typeid(cmd)));
            spdlog::default_logger()->flush();
            if (auto opt_msg = cmd()) {
              spdlog::info("Runtime::run: Cmd() produced  Msg");
              spdlog::default_logger()->flush();
              const Msg &msg = *opt_msg;
              if (msg) {
                msg_q.push(msg);
                spdlog::info("Runtime::run: Msg pushed");
              } else {
                spdlog::warn("Runtime::run: Command returned null message!");
              }
            }
          } else {
            spdlog::warn("Runtime::run: DESIGN_INSUFFICIENCY - runtime queue poisoned with null (default constructed?) Cmd");
          }
        } 
        else if (not msg_q.empty()) {
          auto msg = msg_q.front(); msg_q.pop();

          if (true) {
            if (msg) {
                spdlog::info("Runtime::Processing Msg type {}",to_type_name(typeid(*msg)));
            } else {
                spdlog::warn("Runtime::Attempted to log null Msg pointer");
            }
          }

          // Try client provided predicate to identify QUIT msg
          if (is_quit_msg(msg)) {
            break; // quit :)
          }

          // Run the message though the client
          auto const &[m, cmd] = m_update(model, msg);
          model = m;
          cmd_q.push(cmd);
        } else {
          // Wait for user input
          ch = m_head->get_input();
          spdlog::info("Runtime::run ch={}",ch);
          if (ui.event_handlers.contains("OnKey")) {
            Event key_event{{"Key",std::to_string(ch)}};
            if (auto optional_msg = ui.event_handlers["OnKey"](key_event)) msg_q.push(*optional_msg);
          }
          else {
            throw std::runtime_error(std::format("Runtime::DESIGN INSUFFICIENCY, Runtime::run failed to find a binding 'OnKey' from client 'view' function"));
          }
        }
        ++loop_count;
      }
      
      // Cleanup the head (UI system)
      m_head->cleanup();
      
      spdlog::info("Runtime::run - END");

      // 250717 - Quit from loop with last user key '-' means the user unwound the stack down to empty
      // , and we return non-zero to signal main 'main' to switch back to zeroth::main.
      // Kind of a development version hack to keep both variants in play until we are ready to move fully to first::main...
      return (ch == '-') ? 1 : 0;
    }

  private:
    init_fn m_init;
    view_fn m_view;
    update_fn m_update;
    std::unique_ptr<Head> m_head;
  };

} // TEA
