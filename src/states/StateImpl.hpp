#pragma once

#include <vector>
#include <string>
#include <utility> // std::pair
#include <map>
#include "cross_dependent.hpp"
#include "cmd.hpp"
#include "cargo/CargoBase.hpp"

namespace first {

  // ----------------------------------
  struct StateImpl {
  private:
  public:
    using UX = std::vector<std::string>;
    using Option = std::pair<std::string, StateFactory>;
    using Options = std::map<char, Option>;
    UX m_ux;
    Options m_options;
    StateImpl(UX const &ux);
    virtual ~StateImpl();
    void add_option(char ch, Option const &option);
    UX const &ux() const;
    UX &ux();
    Options const &options() const;
    virtual std::pair<std::optional<State>, Cmd> update(Msg const &msg);
    virtual Cargo get_cargo() const;
  };

} // namespace first