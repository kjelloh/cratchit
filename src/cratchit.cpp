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

  class State {
  public:
    using pointer = std::shared_ptr<State>;
    State(int state_id, State::pointer parent) 
      :   m_state_id{state_id} 
         ,m_parent{parent} {}

    int id() const {return m_state_id;}
        
  private:
    State::pointer m_parent;
    int m_state_id;
  };

  class NavigationGraph {
  public:
    using Adj = std::map<char, int>;
    using AdjList = std::map<int, Adj>;
    class Node {
    public:
      Node(std::string const& caption) : m_caption{caption} {}
      Node(char const* caption) : m_caption{caption} {}
      virtual ~Node() {}
      std::string const& caption() const {return m_caption;}
      bool operator<=>(Node const& other) const = default;
      virtual State::pointer actual(int state_id,State::pointer current) const {
        return std::make_shared<State>(state_id,current);
      }
    private:
      std::string m_caption;
    };

    NavigationGraph() {}

    Adj const& adj(int id) const {return m_adj_list.at(id);}

    void add_transition(Node const &from, char ch,
                        Node const &to) {
      m_adj_list[id_of(from)][ch] = id_of(to);
    }

    int id_of(Node const& node) {
      auto iter = std::find(m_id2node.begin(), m_id2node.end(), node);
      if (iter != m_id2node.end()) {
        return std::distance(m_id2node.begin(), iter);
      }
      else {
        int id = m_id2node.size();
        m_id2node.push_back(node);
        m_adj_list[id];
        return id;
      }
    }

    Node const& node_of(int id) const {return m_id2node[id];}

  private:
    AdjList m_adj_list{};
    std::vector<Node> m_id2node{};
  };

  struct Model {
    std::string top_content;
    std::string main_content;
    std::string user_input;
    NavigationGraph possible{};
    std::stack<State::pointer> stack{};
  };

  std::pair<Model,Cmd> init() {
    Model model = {"Welcome to the top section",
                   "This is the main content area", ""};
    NavigationGraph::Node root("root");
    model.possible.add_transition(root, '1', "ITfied");
    model.possible.add_transition("ITfied", '1', "2024-2025");
    model.possible.add_transition("2024-2025", '1', "Rubrik Belopp Datum");
    model.possible.add_transition("2024-2025", '2', "Momsrapport");
    model.possible.add_transition("Momsrapport", '1', "Q1");
    model.possible.add_transition("Momsrapport", '2', "Q2");
    model.possible.add_transition("Momsrapport", '3', "Q3");
    model.possible.add_transition("Momsrapport", '4', "Q4");
    model.stack.push(root.actual(model.possible.id_of(root),nullptr));
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
          else if (model.possible.adj(model.stack.top()->id()).contains(ch)) {
            int next_id = model.possible.adj(model.stack.top()->id()).at(ch);
            auto next = model.possible.node_of(next_id).actual(next_id,model.stack.top());
            model.stack.push(next);
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
        for (auto const &[ch, to] : model.possible.adj(model.stack.top()->id())) {
          std::string entry{};
          entry.push_back(ch);
          entry.append(" - ");
          auto node = model.possible.node_of(to);
          entry.append(node.caption());
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
