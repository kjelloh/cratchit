#pragma once

#include "pyroceph/tui/key_reader.hpp"

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pyroceph::tui {

// The key that goes back to the state before, unless the current state maps it itself
inline constexpr CodePoint back_key = U'-';

// The error text when a state that takes no input gets text
inline constexpr std::string_view no_input_error = "Programmet tar inte emot text här.";

class State;

// What a state answers to a key: the next state, or nullptr when the state has
// no mapping for the key. The TUI takes over the new state.
using OnKeyResult = std::unique_ptr<State>;

// One option a state offers: its key and its caption (UTF-8, e.g. "Bokslut")
using Option = std::pair<CodePoint, std::string>;

// The options a state offers, in display order. The TUI formats the list.
using OptionsList = std::vector<Option>;

// The lines of a state's view: its content, shown above its options. The TUI
// formats the lines. All UTF-8.
using View = std::vector<std::string>;

// What a state answers when it accepts text the user entered (see below)
struct Accepted;

// What a state answers to text the user entered: Accepted, or an error text
// (UTF-8) when it does not accept it.
using InputResult = std::expected<Accepted, std::string>;

// What Tab adds to the input, as a state answers it (see State::complete)
struct Completion {
  std::u32string added;                // appended to the input
  std::vector<std::string> candidates; // listed (UTF-8) when nothing can be added
  std::vector<std::string> notes;      // shown (UTF-8) below the input, e.g. what a completed file holds
};

// One place in the UI. The client derives its concrete states from it.
//
// A state has two parts that the TUI shows: its view (its content) and, below
// it, its options (what the user can do from here with a key). What a key
// reaches is an option; everything else is the view.
//
// A state creates the next state itself, in on_key, when the user goes there.
// So the tree of states grows as the user navigates, and a state can offer
// options that are only known when the program runs (projects, years, ...).
class State {
public:
  virtual ~State() = default;

  // The place's name, e.g. "Huvudmeny". Shown in the path, so keep it short. UTF-8.
  virtual std::string name() const = 0;

  // The state's content, shown above the options. Empty when there is nothing to show.
  virtual View view() const = 0;

  // The options to show the user. The state keeps them in step with on_key.
  virtual OptionsList options() const = 0;

  // The next state for the key, or nullptr when the state has no mapping for it
  virtual OnKeyResult on_key(CodePoint key) = 0;

  // Acts on a key without leaving the state, e.g. showing more of a long list
  // ("Mer…"). True when it did: the TUI then shows the state again. Asked
  // before on_key. False by default.
  virtual bool on_key_in_place(CodePoint /*key*/) { return false; }

  // Acts on text the user entered. By default the state takes no input and
  // answers with no_input_error.
  virtual InputResult on_input(std::u32string_view text);

  // The text the input starts with whenever the state is shown, e.g. a folder
  // to type a path from. Empty by default: the state is navigated until the
  // user types. A state with a start text is where the user types.
  virtual std::u32string start_input() const { return {}; }

  // True for a state that is a text field: the user types there even when the
  // input is empty, so every key is text, except back_key and help_key while
  // the input is untouched, and Backspace on an empty input does nothing.
  // False by default: an empty input means navigating.
  virtual bool is_text_field() const { return false; }

  // What Tab adds to the input text, e.g. the rest of a folder name. Nothing
  // by default, and then Tab does nothing but sound the bell.
  virtual Completion complete(std::u32string_view text) const;

  // The topic of the state's user instruction, which help_key shows (see
  // HelpSource in prompt_loop.hpp). Empty by default: no instruction.
  virtual std::string help_topic() const { return {}; }

  // A state before this one on the path that the TUI goes back to after this
  // state has acted on a key or input without leading on, e.g. where the user
  // started a task that is now done. nullptr by default: stay.
  virtual State const* back_to() const { return nullptr; }

  // The state the TUI enters once it has gone back to back_to(), e.g. where a
  // task that is done leads on from there. Asked once, as the TUI goes back.
  // nullptr by default: stay at back_to().
  virtual std::unique_ptr<State> after_back() { return nullptr; }

  // Called when the user comes back to the state from a state after it, before
  // it is shown again. Lets the state read again what may have changed there.
  virtual void on_return() {}
};

// What a state answers when it accepts text the user entered: a reply to show
// (UTF-8, may be empty) and, when the input leads on, the next state, which
// the TUI takes over as if a key had led there.
struct Accepted {
  // Implicit from the reply, so that a state may answer with its reply alone
  Accepted() = default;
  Accepted(std::string reply_) : reply(std::move(reply_)) {}
  Accepted(char const* reply_) : reply(reply_) {}
  Accepted(std::unique_ptr<State> next_, std::string reply_ = {}) : reply(std::move(reply_)), next(std::move(next_)) {}

  std::string reply;
  std::unique_ptr<State> next; // nullptr: stay in the state
};

// The states from the root to the current one. Owns them: a state is
// destroyed when the user leaves it.
class Navigator {
public:
  // root must not be empty
  explicit Navigator(std::unique_ptr<State> root);

  State& current() const { return *path_.back(); }

  // The states from the root to the current state, in order
  std::vector<std::unique_ptr<State>> const& path() const { return path_; }

  // True when there is a state before the current one
  bool can_go_back() const { return path_.size() > 1; }

  // Gives the key to the current state. When it acts in place
  // (State::on_key_in_place) the state stays; else when it answers with a new
  // state, that becomes the current state; else, when it then names a state to
  // go back to (State::back_to), the navigator goes back there. True when the
  // state changed or acted, so that it is to be shown again.
  bool press(CodePoint key);

  // Goes back to the state the current state names (State::back_to), when it
  // names one on the path before it, and enters what the current state gives
  // to enter from there (State::after_back), if anything. True when the state changed.
  bool follow_back_to();

  // Makes next, which must not be empty, the current state, e.g. the state
  // that accepted input leads to
  void enter(std::unique_ptr<State> next);

  // Leaves the current state, which is destroyed, for the state before, and
  // calls its on_return. Does nothing at the root. True when the state changed.
  bool back();

private:
  std::vector<std::unique_ptr<State>> path_; // front() is the root, back() is the current state
};

} // namespace pyroceph::tui
