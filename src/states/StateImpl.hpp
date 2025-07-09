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
#include "cargo/EditedItemCargo.hpp"

namespace first {
  
  // ----------------------------------
  class StateImpl {
  public:
    using UX = std::vector<std::string>;
    // Refactoring into CmdOptions
    using CmdOption = std::pair<std::string, Cmd>;    
    using CmdOptions = std::pair<std::vector<char>,std::map<char, CmdOption>>;

    // Refactor into UpdateOptions - BEGIN
    using UpdateResult = std::pair<std::optional<State>, Cmd>;
    using UpdateOptionFunction = std::function<UpdateResult()>;
    using UpdateOption = std::pair<std::string,UpdateOptionFunction>;
    using UpdateOptions = std::pair<std::vector<char>,std::map<char, UpdateOption>>;
    void add_update_option(char ch, UpdateOption const &option);
    UpdateOptions m_update_options;
    // Refactor into UpdateOptions - END

    // Members
    StateImpl(UX const &ux);
    virtual ~StateImpl();
    void add_cmd_option(char ch, CmdOption const &option); // // Refactoring into CmdOptions
    UX const &ux() const;
    UX &ux();
    CmdOptions const &cmd_options() const; // // Refactoring into CmdOptions
    UpdateOptions const &update_options() const;
    std::pair<std::optional<State>, Cmd> dispatch(Msg const& msg);    
    virtual std::pair<std::optional<State>, Cmd> apply(cargo::DummyCargo const& cargo) const;
    virtual std::pair<std::optional<State>, Cmd> apply(cargo::HADsCargo const& cargo) const;
    virtual std::pair<std::optional<State>, Cmd> apply(cargo::EnvironmentCargo const& cargo) const;
    virtual std::pair<std::optional<State>, Cmd> apply(cargo::EditedItemCargo<HeadingAmountDateTransEntry> const& cargo) const;
    virtual Cargo get_cargo() const;

    // Helper to convert StateFactory to PushStateMsg Cmd (Phase 1)
    static Cmd cmd_from_state_factory(StateFactory factory);

  protected:
    UX m_ux;

  private:
    CmdOptions m_cmd_options; // // Refactoring into CmdOptions

    virtual std::pair<std::optional<State>, Cmd> update(Msg const& msg);
    std::pair<std::optional<State>, Cmd> default_update(Msg const& msg);
    std::pair<std::optional<State>, Cmd> default_update(char key) const;

  }; // StateImpl

} // namespace first