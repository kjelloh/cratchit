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
  class StateImpl {
  public:
    using UX = std::vector<std::string>;
    using Option = std::pair<std::string, StateFactory>;
    using Options = std::map<char, Option>;
    // Refactoring into CmdOptions
    using CmdOption = std::pair<std::string, Cmd>;    
    using CmdOptions = std::pair<std::vector<char>,std::map<char, CmdOption>>;

    // Members
    StateImpl(UX const &ux);
    virtual ~StateImpl();
    void add_option(char ch, Option const &option);
    void add_cmd_option(char ch, CmdOption const &option); // // Refactoring into CmdOptions
    UX const &ux() const;
    UX &ux();
    std::string const& input_buffer() const;
    Options const &options() const;
    CmdOptions const &cmd_options() const; // // Refactoring into CmdOptions
    std::pair<std::optional<State>, Cmd> dispatch(Msg const& msg);    
    virtual std::pair<std::optional<State>, Cmd> apply(cargo::DummyCargo const& cargo) const;
    virtual std::pair<std::optional<State>, Cmd> apply(cargo::HADsCargo const& cargo) const;
    virtual std::pair<std::optional<State>, Cmd> apply(cargo::EnvironmentCargo const& cargo) const;
    virtual Cargo get_cargo() const;

  protected:
    UX m_ux;

  private:
    Options m_options;
    CmdOptions m_cmd_options; // // Refactoring into CmdOptions
    std::string m_input_buffer;

    virtual std::pair<std::optional<State>, Cmd> update(Msg const& msg);
    std::pair<std::optional<State>, Cmd> default_update(Msg const& msg);
    std::pair<std::optional<State>, Cmd> default_update(char key) const;

  }; // StateImpl

} // namespace first