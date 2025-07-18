#pragma once

#include "cross_dependent.hpp"
#include "cmd.hpp"
#include "msgs/msg.hpp"
#include <vector>
#include <optional>
#include <memory>
#include <string>
#include <immer/box.hpp>
#include <unicode/uchar.h>

namespace first {

  // ----------------------------------
  class UserInputBufferState {
  public:

    template <class S>
    struct UpdateResultT {
      std::optional<S> maybe_state;
      Cmd maybe_null_cmd;
      operator bool() const {return maybe_state.has_value() or maybe_null_cmd != nullptr;}
      void apply(S& state,Cmd& cmd) const {
        if (maybe_state) state = *maybe_state;
        if (maybe_null_cmd) cmd = maybe_null_cmd;
      }
    };
    using UpdateResult = UpdateResultT<UserInputBufferState>;

    enum class State { Editing, Committed };
    
    UserInputBufferState() : m_buffer(""), m_state(State::Editing) {}
    explicit UserInputBufferState(immer::box<std::string> buffer) : m_buffer(buffer), m_state(State::Editing) {}

    UpdateResult update(Msg const& msg) const {
      // Only handle NCursesKeyMsg
      auto key_msg = std::dynamic_pointer_cast<NCursesKeyMsg>(msg);
      if (!key_msg) {
        return {std::nullopt}; // Not our concern
      }
      
      auto ch = key_msg->key;
      return handle_char_input(ch);
    }

    std::string buffer() const {
      return *m_buffer;
    }

    State state() const {
      return m_state;
    }

    bool is_committed() const {
      return m_state == State::Committed;
    }

    UserInputBufferState clear() const {
      return UserInputBufferState();
    }

  private:
    immer::box<std::string> m_buffer;
    State m_state;
    
    static UpdateResult make_result(std::optional<UserInputBufferState> state, Cmd cmd = {}) {
      return {.maybe_state = state, .maybe_null_cmd = cmd};
    }
    
    static UpdateResult make_result(UserInputBufferState state, Cmd cmd = {}) {
      return {.maybe_state = state, .maybe_null_cmd = cmd};
    }
        
    UserInputBufferState with_buffer(immer::box<std::string> buffer) const {
      UserInputBufferState result = *this;
      result.m_buffer = buffer;
      return result;
    }

    UserInputBufferState commit() const {
      UserInputBufferState result = *this;
      result.m_state = State::Committed;
      return result;
    }

    UpdateResult handle_char_input(int ch) const {
      if (!m_buffer->empty() && ch == 127) { // Backspace
        auto new_buffer = m_buffer->substr(0, m_buffer->length() - 1);
        return make_result(with_buffer(immer::box<std::string>(new_buffer)));
      }
      else if (u_isprint(static_cast<UChar32>(static_cast<unsigned char>(ch)))) {
        auto new_buffer = *m_buffer + static_cast<char>(ch);
        return make_result(with_buffer(immer::box<std::string>(new_buffer)));
      }
      else if (!m_buffer->empty() && ch == '\n') {
        auto cmd = [entry = *m_buffer]() -> std::optional<Msg> {
          return std::make_shared<UserEntryMsg>(entry);
        };
        return make_result(clear(), cmd);
      }
      return make_result(std::nullopt); // Didn't handle this input
    }
    
  };

  // ----------------------------------
  struct Model {
    UserInputBufferState user_input_state;
    std::vector<State> ui_states{};
  };

} // namespace first