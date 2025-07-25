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
#include <unicode/unistr.h>  // ICU UnicodeString
#include <unicode/brkiter.h> // ICU BreakIterator for grapheme counting

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
    
    UserInputBufferState() : m_buffer(icu::UnicodeString()), m_state(State::Editing) {}
    explicit UserInputBufferState(immer::box<icu::UnicodeString> buffer) : m_buffer(buffer), m_state(State::Editing) {}

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
      std::string utf8_result;
      m_buffer->toUTF8String(utf8_result);
      return utf8_result;
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

    // Visual width for cursor positioning (count grapheme clusters)
    size_t visual_width() const {
      UErrorCode status = U_ZERO_ERROR;
      std::unique_ptr<icu::BreakIterator> iter(
        icu::BreakIterator::createCharacterInstance(icu::Locale::getDefault(), status));
      
      if (U_FAILURE(status) || !iter) {
        // Fallback to character count if BreakIterator fails
        return m_buffer->countChar32();
      }
      
      iter->setText(*m_buffer);
      size_t count = 0;
      while (iter->next() != icu::BreakIterator::DONE) {
        count++;
      }
      return count;
    }

  private:
    immer::box<icu::UnicodeString> m_buffer;
    State m_state;
    
    static UpdateResult make_result(std::optional<UserInputBufferState> state, Cmd cmd = {}) {
      return {.maybe_state = state, .maybe_null_cmd = cmd};
    }
    
    static UpdateResult make_result(UserInputBufferState state, Cmd cmd = {}) {
      return {.maybe_state = state, .maybe_null_cmd = cmd};
    }
        
    UserInputBufferState with_buffer(immer::box<icu::UnicodeString> buffer) const {
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
      if (!m_buffer->isEmpty() && ch == 127) { // Backspace
        // Remove last grapheme cluster using Unicode-aware method
        auto char_count = m_buffer->countChar32();
        if (char_count > 0) {
          auto new_buffer = m_buffer->tempSubString(0, char_count - 1);
          return make_result(with_buffer(immer::box<icu::UnicodeString>(new_buffer)));
        }
      }
      else if (u_isprint(static_cast<UChar32>(ch))) {
        // Append Unicode code point to buffer
        auto new_buffer = *m_buffer + static_cast<UChar32>(ch);
        return make_result(with_buffer(immer::box<icu::UnicodeString>(new_buffer)));
      }
      else if (!m_buffer->isEmpty() && ch == '\n') {
        // Convert Unicode buffer to UTF-8 string for the message
        std::string utf8_entry;
        m_buffer->toUTF8String(utf8_entry);
        auto cmd = [entry = utf8_entry]() -> std::optional<Msg> {
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