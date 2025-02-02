#include "cratchit.h"
#include "runtime.hpp"
#include <iostream>
#include <map>
#include <memory>
#include <ncurses.h>
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

  std::optional<Msg> onKey(Event event) {
    if (event.contains("Key")) {
      return Msg{std::make_shared<NCursesKey>(std::stoi(event["Key"]))};
    }
    return std::nullopt;
  }

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

  Html_Msg<Msg> view(const Model &model) {
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

  int main(int argc, char *argv[]) {
    Runtime<Model,Msg,Cmd> app(init,view,update);
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
