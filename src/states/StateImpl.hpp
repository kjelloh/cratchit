#pragma once

#include "cross_dependent.hpp"
#include "cmd.hpp"
#include "cargo/CargoBase.hpp"
#include "cargo/HADsCargo.hpp"
#include "cargo/EnvironmentCargo.hpp"
#include <vector>
#include <string>
#include <utility> // std::pair
#include <map>
#include <spdlog/spdlog.h>


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

    template <typename F>
    class KeyToFunctionOptionsT {
    public:
      using FReturnType = std::invoke_result_t<F>; // F is callable that takes no arguments
      using Caption = std::string;
      using Option = std::pair<Caption,F>; // (caption,F)
      using Key = char;
      void add(Key key,Option const& option) {
        if (auto iter = std::ranges::find(m_options_map.first,key); iter == m_options_map.first.end()) {
          // Allow insertion of same option only once
          m_options_map.first.push_back(key); // remember insert order
          m_options_map.second[key] = option; // remember mapped option
        }
        else {
          spdlog::warn("KeyToFunctionOptionsT::add: Ignored add of existing update option '{}'", key);
        }
      }

      // iteratable (key,option),...
      // for (auto const& [key,option] : KeyToFunctionOptionsT<MyFunc>{}.view()) {...}
      auto view() const {
        return m_options_map.first
          | std::views::transform([this](char ch) -> std::pair<char,Option> {
            return std::make_pair(ch,m_options_map.second.at(ch));
        });
      }

      using InsertOrderedOptionsMap = std::pair<std::vector<Key>,std::map<Key, Option>>;
      // InsertOrderedOptionsMap const& value() const {return m_options_map;}

      FReturnType apply(Key key) const {        
        if (this->m_options_map.second.contains(key)) {
          return this->m_options_map.second.at(key).second();
        }
        else return FReturnType{}; // default return type
      }

      std::optional<F> operator[](Key key) const {
        if (m_options_map.second.contains(key)) {
          return m_options_map.second.at(key).second;
        }
        else return std::nullopt;
      }

    private:
      InsertOrderedOptionsMap m_options_map{};
    };

    // 'Latest' key -> (caption,UpdateFunction)
    using OptionUpdateFunction = std::function<StateUpdateResult()>;
    using UpdateOptions = KeyToFunctionOptionsT<OptionUpdateFunction>;
    using UpdateOption = UpdateOptions::Option;

    // 'Older' key -> (caption,Cmd)
    // using CmdOptions = KeyToFunctionOptionsT<Cmd>;
    // using CmdOption = CmdOptions::Option;

    // 'Latest' add of key -> (caption,update)
    // void add_update_option(char ch, UpdateOption const &option);
    // // 'Older' add of key -> (caption,cmd)
    // void add_cmd_option(char ch, CmdOption const &option);

    // 'Latest': accessor to update options (key -> (caption,update))
    UpdateOptions const& update_options() const;
    // // 'Older': accessor to command options (key -> (caption,cmd)) 
    // CmdOptions const &cmd_options() const; // // Refactoring into CmdOptions

    // Members
    StateImpl(StateImpl const&) = delete; // As Immutable = copy not allowed
    StateImpl(StateImpl&&) = delete; // As Immutable = move from not allowed
    StateImpl& operator=(StateImpl const&) = delete; // As Immutable = copy assignment not allowed
    StateImpl& operator=(StateImpl&&) = delete; // As Immutable = move assignment not allowed
    StateImpl(UX const &ux);
    virtual ~StateImpl();
    UX const &ux() const;
    // UX &ux();

    StateUpdateResult dispatch(Msg const& msg) const;
    virtual StateUpdateResult apply(cargo::DummyCargo const& cargo) const;
    // Cargo visit/apply double dispatch removed (cargo now message passed)
    // virtual StateUpdateResult apply(cargo::HADsCargo const& cargo) const;
    virtual StateUpdateResult apply(cargo::EnvironmentCargo const& cargo) const;

    // Cargo visit/apply double dispatch removed (cargo now message passed)
    // virtual Cargo get_cargo() const;
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
    mutable std::optional<StateImpl::UpdateOptions> m_transient_maybe_update_options;
    // Lazy UX generation
    mutable std::optional<UX> m_transient_maybe_ux;
    // // 'Older' key -> (caption,Cmd) map
    // std::optional<StateImpl::CmdOptions> m_transient_maybe_cmd_options;

    virtual UpdateOptions create_update_options() const; // Concrete state shall implement
    virtual UX create_ux() const; // Concrete state shall implement
    // virtual CmdOptions create_cmd_options() const; // // Concrete state shall implement

    // // 'Latest' key -> (caption,update) map
    // UpdateOptions m_update_options_;
    // // 'Older' key -> (caption,Cmd) map
    // CmdOptions m_cmd_options; // key -> Cmd (older mechanism)
    
    // State TEA update mechanism
    virtual StateUpdateResult update(Msg const& msg) const;
    StateUpdateResult default_update(Msg const& msg) const;
    StateUpdateResult default_update(char key) const;

  }; // StateImpl

} // namespace first