#include "cratchit.h"
#include "cross_dependent.hpp"
#include "cmd.hpp"
#include "sub.hpp"
#include "tea/runtime.hpp"
#include "states/StateImpl.hpp"
#include "states/FrameworkState.hpp"
#include "to_type_name.hpp"
#include <iostream>
#include <map>
#include <memory>
#include <ncurses.h>
#include <stack>
#include <functional>
#include <queue>
#include <filesystem>
#include <cmath>  // std::pow,...
#include <immer/vector.hpp>

namespace first {

  // ----------------------------------
  // ----------------------------------
  // h-file parts
  // ----------------------------------
  // ----------------------------------

  // ----------------------------------
  struct Model {
    std::string top_content;
    std::string main_content;
    std::string user_input;
    /*
    The stack contains the 'path of states' the user has navigated to.
    */
    std::stack<State> stack{};
  };

  // ----------------------------------
  extern bool is_quit_msg(Msg const& msg);

  // ----------------------------------
  extern std::tuple<Model,runtime::IsQuit<Msg>,Cmd> init();
  extern std::pair<Model,Cmd> update(Model model, Msg msg);
  extern Html_Msg<Msg> view(const Model &model);


  // ----------------------------------
  // ----------------------------------
  // cpp-file parts
  // ----------------------------------
  // ----------------------------------

  // ----------------------------------
  auto framework_state_factory = []() {
    auto framework_ux = StateImpl::UX{"Framework UX"};
    std::filesystem::path root_path = std::filesystem::current_path();
    return std::make_shared<FrameworkState>(framework_ux,root_path);
  };

  // ----------------------------------
  bool is_quit_msg(Msg const& msg) {
    // std::cout << "\nis_quit_msg sais Hello" << std::flush;
    return msg == QUIT_MSG;
  }

  // ----------------------------------
  std::tuple<Model,runtime::IsQuit<Msg>,Cmd> init() {
    // std::cout << "\ninit sais Hello :)" << std::flush;
    Model model = { "Welcome to the top section"
                   ,"This is the main content area"
                   ,""};

    // model.stack.push(framework_state_factory());
    auto new_framework_state_cmd = []() -> Msg {
      auto msg = std::make_shared<PushStateMsg>(framework_state_factory());
      return msg;
    };
    return {model,is_quit_msg,new_framework_state_cmd};
  }

  // ----------------------------------
  // TODO: Consider if model by value (as immutable) is actually a good thing?
  //       I mean, immutable is good and all that. But is it practical?
  //       At least I trust we get return value optimization?
  //       Fun to explore though...
  std::pair<Model,Cmd> update(Model model, Msg msg) {
    Cmd cmd = Nop;
    std::optional<State> new_state{};

    // State stack top processing
    if (model.stack.size()>0) {
      // Pass msg to state for state update
      if (auto pimpl = std::dynamic_pointer_cast<PoppedStateCargoMsg>(msg)) {
        if (model.stack.top() == pimpl->m_top) {
          if (pimpl->m_cargo) {
            const auto &cargo = *pimpl->m_cargo;
            spdlog::info("PoppedStateCargoMsg: {}", to_type_name(typeid(cargo)));
            auto [maybe_new_state, state_cmd] = cargo.visit(model.stack.top());
            new_state = maybe_new_state;
            cmd = state_cmd;
          } else {
            spdlog::info("PoppedStateCargoMsg: NULL cargo");
          }
        }
      }

      else {
        auto pp = model.stack.top()->update(msg);
        new_state = pp.first;
        cmd = pp.second;
      }
    }

    if (new_state) {
      // Let 'StateImpl' update itself
      auto& ref = *new_state; // Consume any shared pointer dereference side effects here.
      spdlog::info("New state:{}",to_type_name(typeid(ref)));
      model.stack.top() = *new_state; // mutate
    }
    else {

      // Process StateImpl transition or user input
      // TODO: How much of this can we migrate into State::update?

      // Process keyboard input
      if (auto key_msg_ptr = std::dynamic_pointer_cast<NCursesKey>(msg);key_msg_ptr != nullptr) {      
        auto ch = key_msg_ptr->key;

        // Use refactoring path to move key processing into state
        auto maybe_cmd = (model.stack.size() > 0) ? model.stack.top()->cmd_from_key(ch) : std::nullopt;
        if (maybe_cmd) {
          cmd = *maybe_cmd; // cirrent state acted on key and returned a cmd
        }
        else {
          // Process key here in model
          // TODO: Refactor step by step into State::cmd_from_key
          if (ch == KEY_BACKSPACE || ch == 127) { // Handle backspace
            if (!model.user_input.empty()) {
              model.user_input.pop_back();
            }
          } 
          else if (ch == '\n') {
            // User pressed Enter: process command (optional)
            cmd = [entry = model.user_input]() {
              auto msg = std::make_shared<UserEntryMsg>(entry);
              return msg;
            };
            model.user_input.clear(); // Reset input after capture to command
          } 
          else {
            if (model.user_input.empty() and ch == 'q' or model.stack.size()==0) {
              // std::cout << "\nTime to QUIT!" << std::flush;
              cmd = DO_QUIT;
            }
            else if (model.user_input.empty() and model.stack.size() > 0) {
              if (ch == '-') {
                // (1) Transition back to previous state
                auto popped_state = model.stack.top(); // share popped state for command to free
                model.stack.pop();
                cmd = [popped_state,rx_state = (model.stack.size()>0)?model.stack.top():nullptr]() mutable -> Msg {
                  if (popped_state.use_count() != 1) {
                    // Design insufficiency (We should be the only user)
                    spdlog::error("DESIGN_INSUFFICIENCY: Failed to execute State destructor in command (user count {} != 1)",popped_state.use_count());
                  }
                  else {
                    spdlog::info("State destructor executed in command (user count {} == 1)",popped_state.use_count());
                    auto& ref = *popped_state; // Consume any shared pointer dereference side effects here.
                    // Then log the type name of the referenced state
                    spdlog::info("Destructing State type {}",to_type_name(typeid(ref)));
                  }
                  auto msg = std::make_shared<PoppedStateCargoMsg>(rx_state,popped_state->get_cargo());
                  popped_state.reset(); // Free popped state
                  return msg;
                };
              } 
              // Moved to State::cmd_from_key
              // else if (model.stack.top()->options().contains(ch)) {
              //   // (2) Transition to new StateImpl
              //   cmd = [ch,parent = model.stack.top()]() -> Msg {
              //     State new_state = parent->options().at(ch).second();
              //     auto msg = std::make_shared<PushStateMsg>(parent,new_state);
              //     return msg;
              //   };
              // }
              else {
                model.user_input += ch; // Append typed character
              }
            }
            else {
                model.user_input += ch; // Append typed character
            }
          }
        }
      }

      // Process Push of new state
      else if (auto pimpl = std::dynamic_pointer_cast<PushStateMsg>(msg);pimpl != nullptr) {
        model.stack.push(pimpl->m_state);
      }

      // Process Cargo from popped state
      // 20250623 - Now in State processing above
      // else if (auto pimpl = std::dynamic_pointer_cast<PoppedStateCargoMsg>(msg);pimpl != nullptr) {
      //   if (model.stack.size()>0 and model.stack.top() == pimpl->m_top) {
      //     // the cargo is targeted to current top ok.
      //     // TODO: Handle cargo
      //     if (pimpl->m_cargo != nullptr) {
      //       auto& cargo = *pimpl->m_cargo; // Consume any shared pointer dereference side effects here.
      //       spdlog::info("PoppedStateCargoMsg: {}",to_type_name(typeid(cargo)));
      //       auto [maybe_new_state,state_cmd] = cargo.visit(model.stack.top());
      //       if (maybe_new_state) {
      //         model.stack.top() = *maybe_new_state; // mutate
      //       }
      //       cmd = state_cmd;
      //     }
      //     else {
      //       spdlog::info("PoppedStateCargoMsg: NULL cargo");
      //     }
      //   }
      // } 

      // ------------...

    }
    // Update UX
    if (model.stack.size() > 0) {
      // StateImpl UX (top window)
      model.top_content.clear();
      for (std::size_t i=0;i<model.stack.top()->ux().size();++i) {
        if (i>0) model.top_content.push_back('\n');
        model.top_content += model.stack.top()->ux()[i];
      }
      // StateImpl transition UX (Midle window)
      model.main_content.clear();
      for (auto const &[ch, option] : model.stack.top()->options()) {
        std::string entry{};
        entry.push_back(ch);
        entry.append(" - ");
        entry.append(option.first);
        entry.push_back('\n');
        model.main_content.append(entry);
      }
    }

    return {model,cmd}; // Return updated model
  }

  // ----------------------------------
  Html_Msg<Msg> view(const Model &model) {
    // std::cout << "\nview sais Hello :)" << std::flush;

    // Create a new pugi document
    Html_Msg<Msg> ui{};
    auto& doc = ui.doc;

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

    ui.event_handlers["OnKey"] = onKey;
    return ui;
  }

  // ----------------------------------
  // ----------------------------------

  int main(int argc, char *argv[]) {
    // std::cout << "\nFirst sais Hello :)" << std::flush;
    Runtime<Model,Msg,Cmd> app(init,view,update);
    // std::cout << "\nFirst to call run :)" << std::flush;
    return app.run(argc,argv);
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
