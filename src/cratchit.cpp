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
#include <functional>
#include <queue>
#include <filesystem>
#include <cmath>  // std::pow,...
#include <immer/vector.hpp>
#include <immer/box.hpp>
#include <unicode/uchar.h>  // for u_isprint
#include <ranges>

namespace first {

  // ----------------------------------
  // ----------------------------------
  // h-file parts
  // ----------------------------------
  // ----------------------------------

  // ----------------------------------
  class UserInputBufferState {
  public:

    template <class S>
    struct UpdateResultT {
      std::optional<S> maybe_state;
      Cmd maybe_null_cmd;
      operator bool() const {return maybe_state.has_value() or maybe_null_cmd != nullptr;}
      void apply(S& state,Cmd& cmd) const {
        if (maybe_state) state = *maybe_state;
        if (maybe_null_cmd) cmd = maybe_null_cmd;
      }
    };
    using UpdateResult = UpdateResultT<UserInputBufferState>;

    enum class State { Editing, Committed };
    
    UserInputBufferState() : m_buffer(""), m_state(State::Editing) {}
    explicit UserInputBufferState(immer::box<std::string> buffer) : m_buffer(buffer), m_state(State::Editing) {}

    UpdateResult update(Msg const& msg) const {
      // Only handle NCursesKeyMsg
      auto key_msg = std::dynamic_pointer_cast<NCursesKeyMsg>(msg);
      if (!key_msg) {
        return {std::nullopt}; // Not our concern
      }
      
      auto ch = key_msg->key;
      return handle_char_input(ch);
    }

    std::string buffer() const {
      return *m_buffer;
    }

    State state() const {
      return m_state;
    }

    bool is_committed() const {
      return m_state == State::Committed;
    }

    UserInputBufferState clear() const {
      return UserInputBufferState();
    }

  private:
    immer::box<std::string> m_buffer;
    State m_state;
    
    static UpdateResult make_result(std::optional<UserInputBufferState> state, Cmd cmd = {}) {
      return {.maybe_state = state, .maybe_null_cmd = cmd};
    }
    
    static UpdateResult make_result(UserInputBufferState state, Cmd cmd = {}) {
      return {.maybe_state = state, .maybe_null_cmd = cmd};
    }
        
    UserInputBufferState with_buffer(immer::box<std::string> buffer) const {
      UserInputBufferState result = *this;
      result.m_buffer = buffer;
      return result;
    }

    UserInputBufferState commit() const {
      UserInputBufferState result = *this;
      result.m_state = State::Committed;
      return result;
    }

    UpdateResult handle_char_input(int ch) const {
      if (!m_buffer->empty() && ch == 127) { // Backspace
        auto new_buffer = m_buffer->substr(0, m_buffer->length() - 1);
        return make_result(with_buffer(immer::box<std::string>(new_buffer)));
      }
      else if (u_isprint(static_cast<UChar32>(static_cast<unsigned char>(ch)))) {
        auto new_buffer = *m_buffer + static_cast<char>(ch);
        return make_result(with_buffer(immer::box<std::string>(new_buffer)));
      }
      else if (!m_buffer->empty() && ch == '\n') {
        auto cmd = [entry = *m_buffer]() -> std::optional<Msg> {
          return std::make_shared<UserEntryMsg>(entry);
        };
        return make_result(clear(), cmd);
      }
      return make_result(std::nullopt); // Didn't handle this input
    }
    
  };

  // ----------------------------------
  struct Model {
    UserInputBufferState user_input_state;
    std::vector<State> ui_states{};
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
    Model model{}; // Default initialization

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
  //       250630/KoH - use_count for shared_ptrs (e.g., states) will show 2 (passed model + our copy)
  std::pair<Model,Cmd> update(Model model, Msg msg) {
    Cmd cmd = Nop;

    // HACK - 250709 / KoH
    // This mechanism helps us know if to ask the state to process the message forst os not.
    // TODO: Consider to find a cleaner way to separate 'our' model update from state-conscern update? 
    auto key_msg_ptr = std::dynamic_pointer_cast<NCursesKeyMsg>(msg);
    bool ask_state_first =
          (model.ui_states.size() > 0)
      and (    (key_msg_ptr == nullptr)
            or (model.user_input_state.buffer().size() == 0));

    auto [mutated_top, state_cmd] = 
       (ask_state_first)
      ?(model.ui_states.back()->dispatch(msg))
      :(std::make_pair<std::optional<State>,Cmd>({},{}));

    // 250709 - HACK! / KoH
    // The cargo::visit still takes State (shared_ptr) so we cant move it to StateImpl::default_update just yet
    if (not (mutated_top or state_cmd)) {
      if (auto pimpl = std::dynamic_pointer_cast<PoppedStateCargoMsg>(msg)) {
        if (pimpl->m_cargo) {
          const auto &cargo = *pimpl->m_cargo;
          spdlog::info("PoppedStateCargoMsg: {}", to_type_name(typeid(cargo)));
          auto [maybe_new_state, maybe_cmd] = cargo.visit(model.ui_states.back());
          mutated_top = maybe_new_state;
          state_cmd = maybe_cmd;
        } else {
          spdlog::info("PoppedStateCargoMsg: NULL cargo");
        }
      }
    }

    // Apply result - if any
    if (mutated_top or state_cmd) {
      // State handled the message - apply the changes
      if (mutated_top) {
        auto& ref = *mutated_top;
        spdlog::info("update(model,msg): state::update -> state:{}[{}] <- use_count: {}"
          ,to_type_name(typeid(ref))
          ,static_cast<void*>(mutated_top->get())
          ,mutated_top->use_count());
        model.ui_states.back() = mutated_top.value(); // replace
      }
      if (state_cmd) {
        spdlog::info("update(model,msg): state::update -> cmd");
        cmd = state_cmd;
      }
    }
    else if (auto update_result = model.user_input_state.update(msg)) {
      update_result.apply(model.user_input_state,cmd);
    }      
    else if (auto pimpl = std::dynamic_pointer_cast<PushStateMsg>(msg); pimpl != nullptr) {
      model.ui_states.push_back(pimpl->m_state);
    }
    else if (auto pimpl = std::dynamic_pointer_cast<PopStateMsg>(msg); pimpl != nullptr) {
      if (model.ui_states.size() > 0) {
        spdlog::info("update(model,msg): Popping top state with use_count: {}", model.ui_states.back().use_count());
        auto popped_state = model.ui_states.back();
        model.ui_states.pop_back();

        if (true) {
          auto const& ref = *popped_state.get();
          spdlog::info("update(model,msg): Popped {}[{}]", to_type_name(typeid(ref)), static_cast<void*>(popped_state.get()));
        }

        cmd = [cargo = popped_state->get_cargo()]() mutable -> std::optional<Msg> {
          auto msg = std::make_shared<PoppedStateCargoMsg>(cargo);
          return msg;
        };
      }
    }
    else {
      // Trace ignored messages
      auto const& ref = *msg;
      spdlog::info("update(model,msg): Ignored message type: {}", to_type_name(typeid(ref)));
    }

    // Dump stack to log
    if (true) {
      spdlog::info("update(model,msg): State stack size:{}",model.ui_states.size());

      auto enumerated_view = [](auto && range) {
        return std::views::zip(
          std::views::iota(0)
          ,std::forward<decltype(range)>(range));
      };

      for (auto const& [index,m] :  enumerated_view(model.ui_states)) {
        auto& ref = *m; // Silence typeid(*m) warning about side effects
        spdlog::info("   {}: {}[{}] <- use_count: {}"
          ,index
          ,to_type_name(typeid(ref))
          ,static_cast<void*>(m.get())
          ,m.use_count());
      }
    }

    // UI generation moved to view() - proper TEA separation

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

    // Generate UI content from current state (TEA-style)
    std::string top_content;
    std::string main_content;
    
    if (model.ui_states.size() > 0) {
      // StateImpl UX (top window)
      for (std::size_t i=0;i<model.ui_states.back()->ux().size();++i) {
        if (i>0) top_content.push_back('\n');
        top_content += model.ui_states.back()->ux()[i];
      }
      
      // StateImpl transition UX (Middle window)
      // iterate as defined by CmdOptions.first (vector of chars)
      auto ordered_cmd_options_view = [](StateImpl::CmdOptions const& cmd_options) {
        return cmd_options.first
          | std::views::transform([&cmd_options](char ch) -> std::pair<char,StateImpl::CmdOption> {
            return std::make_pair(ch,cmd_options.second.at(ch));
          });
      };

      for (auto const& [ch, cmd_option] : ordered_cmd_options_view(model.ui_states.back()->cmd_options())) {
        std::string entry{};
        entry.push_back(ch);
        entry.append(" = ");
        entry.append(cmd_option.first);
        entry.push_back('\n');
        main_content.append(entry);
      }

      // Also process UpdateOptions (similar to cmd_options)
      auto ordered_update_options_view = [](StateImpl::UpdateOptions const& update_options) {
        return update_options.first
          | std::views::transform([&update_options](char ch) -> std::pair<char,StateImpl::UpdateOption> {
            return std::make_pair(ch,update_options.second.at(ch));
          });
      };

      for (auto const& [ch, update_option] : ordered_update_options_view(model.ui_states.back()->update_options())) {
        std::string entry{};
        entry.push_back(ch);
        entry.append(" = ");
        entry.append(update_option.first);
        entry.push_back('\n');
        main_content.append(entry);
      }
    }

    // Create the top section with class "content"
    pugi::xml_node top = body.append_child("div");
    top.append_attribute("class") = "content";
    top.text().set(top_content.c_str());

    // Create the main section with class "content"
    pugi::xml_node main = body.append_child("div");
    main.append_attribute("class") = "content";
    main.text().set(main_content.c_str());

    // Create the user prompt section with class "user-prompt"
    pugi::xml_node prompt = body.append_child("div");
    prompt.append_attribute("class") = "user-prompt";
    // Add a label element for the prompt text
    pugi::xml_node label = prompt.append_child("label");
    std::string input_text = model.user_input_state.buffer();
    label.text().set((">" + input_text).c_str());

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
