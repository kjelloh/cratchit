#pragma once

#include <functional>
#include <memory>
#include <pugixml.hpp>
#include <map>

// Event is a key-value-pair
using Event = std::map<std::string,std::string>;

template <typename Msg>
struct Html_Msg {
    pugi::xml_document doc{};
    std::map<std::string,std::function<Msg(Event)>> event_handlers{};
};

template <typename Model, typename Msg, typename Cmd>
class Runtime {
public:
  using Html = Html_Msg<Msg>;
  using init_fn = std::function<std::pair<Model, Cmd>()>;
  using view_fn = std::function<Html(Model)>;
  using update_fn = std::function<std::pair<Model, Cmd>(Model, Msg)>;
  Runtime(init_fn, view_fn, update_fn);
  int run(int argc, char *argv[]);
private:
  class RuntimeImpl;
  std::unique_ptr<RuntimeImpl> m_pimpl;
};