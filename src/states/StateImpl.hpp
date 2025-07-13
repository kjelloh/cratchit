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
  // TODO: Consider to make it more explicit that States are NOT copyable.
  //       For now this base class has the copy constructor deleted.
  //       The reason is that UpdateOptions (and as long as they exist CmdOptions)
  //       must be 'refreshed' for each new instance. This is beacuse they
  //       are allowed to define lambdas that captures 'this'.
  class StateImpl {
  public:
    using UX = std::vector<std::string>;

    using Caption = std::string;

    // 'Latest' key -> (caption,UpdateFunction)
    using StateUpdateFunction = std::function<StateUpdateResult()>;
    using UpdateOption = std::pair<Caption,StateUpdateFunction>; // (caption,update)
    using UpdateOptions = std::pair<std::vector<char>,std::map<char, UpdateOption>>;

    // 'Older' key -> (caption,Cmd)
    using CmdOption = std::pair<std::string, Cmd>;    
    using CmdOptions = std::pair<std::vector<char>,std::map<char, CmdOption>>;

    // 'Latest' add of key -> (caption,update)
    void add_update_option(char ch, UpdateOption const &option);
    // 'Older' add of key -> (caption,cmd)
    void add_cmd_option(char ch, CmdOption const &option);

    // 'Latest': sccessor to update options (key -> (caption,update))
    UpdateOptions const &update_options() const;
    // 'Older': accessor to command options (key -> (caption,cmd)) 
    CmdOptions const &cmd_options() const; // // Refactoring into CmdOptions

    // Members
    StateImpl(StateImpl const&) = delete; // As Immutable = copy not allowed
    StateImpl(StateImpl&&) = delete; // As Immutable = move from not allowed
    StateImpl& operator=(StateImpl const&) = delete; // As Immutable = copy assignment not allowed
    StateImpl& operator=(StateImpl&&) = delete; // As Immutable = move assignment not allowed
    StateImpl(UX const &ux);
    virtual ~StateImpl();
    UX const &ux() const;
    UX &ux();

    StateUpdateResult dispatch(Msg const& msg) const;    
    virtual StateUpdateResult apply(cargo::DummyCargo const& cargo) const;
    virtual StateUpdateResult apply(cargo::HADsCargo const& cargo) const;
    virtual StateUpdateResult apply(cargo::EnvironmentCargo const& cargo) const;

    // TODO: Refactor get_cargo() -> get_on_destruct_msg mechanism
    virtual Cargo get_cargo() const;
    virtual std::optional<Msg> get_on_destruct_msg() const;

    // Helper to convert StateFactory to PushStateMsg Cmd (Phase 1)
    static Cmd cmd_from_state_factory(StateFactory factory);

  protected:
    UX m_ux;

    template <typename Derived, typename... Args>
    static std::unique_ptr<Derived> to_cloned(const Derived&, Args&&... args) {
        return std::make_unique<Derived>(std::forward<Args>(args)...);
    }    

  private:
    // 'Latest' key -> (caption,update) map
    UpdateOptions m_update_options;
    // 'Older' key -> (caption,Cmd) map
    CmdOptions m_cmd_options; // key -> Cmd (older mechanism)

    // State TEA update mechanism
    virtual StateUpdateResult update(Msg const& msg) const;
    StateUpdateResult default_update(Msg const& msg) const;
    StateUpdateResult default_update(char key) const;

  }; // StateImpl

} // namespace first