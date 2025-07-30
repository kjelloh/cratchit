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
#include "logger/log.hpp"
#include <csignal> // std::signal

// The Elm Aarchitecture
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

  template <class Q>
  std::optional<std::decay_t<decltype(std::declval<Q&>().front())>> try_pop(Q& q) {
      if (q.size() > 0) {
          auto item = q.front();
          q.pop();
          return item; // will copy or move the value
      }
      return std::nullopt;
  }

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

      m_head->initialize();

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

        if (auto maybe_cmd = try_pop(cmd_q)) {
          // Execute a command
          if (auto cmd = *maybe_cmd) {
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
