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
    // Refactoring into CmdOptions
    using CmdOption = std::pair<std::string, Cmd>;    
    using CmdOptions = std::pair<std::vector<char>,std::map<char, CmdOption>>;
    
    // Refactor into UpdateOptions - BEGIN
    using UpdateOptionFunction = std::function<StateUpdateResult()>;
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
    StateUpdateResult dispatch(Msg const& msg) const;    
    virtual StateUpdateResult apply(cargo::DummyCargo const& cargo) const;
    virtual StateUpdateResult apply(cargo::HADsCargo const& cargo) const;
    virtual StateUpdateResult apply(cargo::EnvironmentCargo const& cargo) const;
    // virtual StateUpdateResult apply(cargo::EditedItemCargo<HeadingAmountDateTransEntry> const& cargo) const;

    // TODO: Refactor get_cargo() -> get_on_destruct_msg mechanism
    virtual Cargo get_cargo() const;
    virtual std::optional<Msg> get_on_destruct_msg() const;

    // Helper to convert StateFactory to PushStateMsg Cmd (Phase 1)
    static Cmd cmd_from_state_factory(StateFactory factory);

  protected:
    UX m_ux;

  private:
    CmdOptions m_cmd_options; // // Refactoring into CmdOptions

    virtual StateUpdateResult update(Msg const& msg) const;
    StateUpdateResult default_update(Msg const& msg) const;
    StateUpdateResult default_update(char key) const;

  }; // StateImpl

} // namespace first