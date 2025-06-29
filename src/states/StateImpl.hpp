#pragma once

#include <vector>
#include <string>
#include <utility> // std::pair
#include <map>
#include "cross_dependent.hpp"
#include "cmd.hpp"
#include "cargo/CargoBase.hpp"
#include "cargo/HADsCargo.hpp"
#include "cargo/EnvironmentCargo.hpp"

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

    virtual std::pair<std::optional<State>, Cmd> apply(cargo::DummyCargo const& cargo) const; // default no-op
    virtual std::pair<std::optional<State>, Cmd> apply(cargo::HADsCargo const& cargo) const; // default no-op
    virtual std::pair<std::optional<State>, Cmd> apply(cargo::EnvironmentCargo const& cargo) const; // default no-op

    virtual Cargo get_cargo() const;

    // Refactoring into CmdOptions
    using CmdOption = std::pair<std::string, Cmd>;    
    using CmdOptions = std::pair<std::vector<char>,std::map<char, CmdOption>>;
    CmdOptions m_cmd_options;
    void add_cmd_option(char ch, CmdOption const &option);
    CmdOptions const &cmd_options() const;

    // TODO: Refactor key procesing into this method, step-by-step
    //       When done, move into update above (and remove this refactoring step)
    virtual std::optional<Cmd> cmd_from_key(char key) const;
  };

} // namespace first