#pragma once

#include "cross_dependent.hpp"
#include "cmd.hpp"
#include "cargo/CargoBase.hpp"
#include <vector>
#include <string>
#include <utility> // std::pair
#include <map>
#include "logger/log.hpp"
#include <algorithm> // std::ranges::find,
#include <ranges> // std::views::transform,

namespace first {  

  // Helper class to manage a list of char -> F:f (F is a callable)
  // The app uses this to map a user key input to calling a registered function.
  // The cratchit app uses it to map a key to a StateUpdateFunction.
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

    // Call registered function if it exists
    FReturnType apply(Key key) const {        
      if (this->m_options_map.second.contains(key)) {
        return this->m_options_map.second.at(key).second();
      }
      else return FReturnType{}; // default return type
    }

    // Access the registered function if it exist
    std::optional<F> operator[](Key key) const {
      if (m_options_map.second.contains(key)) {
        return m_options_map.second.at(key).second;
      }
      else return std::nullopt;
    }

  private:
    // A map the keeps track of the order the keys are added.
    InsertOrderedOptionsMap m_options_map{};
  };

  class StateImpl {
  public:
    using UX = std::vector<std::string>;
    // 'Latest' key -> (caption,UpdateFunction)
    using OptionUpdateFunction = std::function<StateUpdateResult()>;
    using UpdateOptions = KeyToFunctionOptionsT<OptionUpdateFunction>;
    using UpdateOption = UpdateOptions::Option;

    StateImpl(StateImpl const&) = delete; // As Immutable = copy not allowed
    StateImpl(StateImpl&&) = delete; // As Immutable = move from not allowed
    StateImpl& operator=(StateImpl const&) = delete; // As Immutable = copy assignment not allowed
    StateImpl& operator=(StateImpl&&) = delete; // As Immutable = move assignment not allowed

    StateImpl(std::optional<std::string> caption = std::nullopt);
    virtual ~StateImpl();
    virtual std::string caption() const;
    UX const &ux() const;
    UpdateOptions const& update_options() const;

    // State TEA update mechanism
    virtual StateUpdateResult update(Msg const& msg) const;

    // virtual std::optional<Msg> get_on_destruct_msg() const;
    static Cmd cmd_from_state_factory(StateFactory factory);

  protected:
    mutable std::optional<std::string> m_caption; // transient caption
  private:
    // 'Transient' data = not regarded as immutable
    mutable std::optional<StateImpl::UpdateOptions> m_transient_maybe_update_options;
    mutable std::optional<UX> m_transient_maybe_ux;

    // Helper to 'refresh' update options and ux to current state instance
    virtual UpdateOptions create_update_options() const; // Concrete state shall implement
    virtual UX create_ux() const; // Concrete state shall implement
    
  }; // StateImpl

} // namespace first