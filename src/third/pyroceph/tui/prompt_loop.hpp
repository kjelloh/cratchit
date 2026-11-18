#pragma once

#include "pyroceph/tui/key_reader.hpp"
#include "pyroceph/tui/state.hpp"

#include <cstdio>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pyroceph::tui {

// True for the keys that end the prompt loop: Ctrl-C (U+0003), Ctrl-D (U+0004)
// and Ctrl-Z (U+001A), since RawMode makes them come as characters
bool is_quit(CodePoint code_point);

// The key that quits while navigating, unless the current state maps it
// itself: it leads to the quit state (make_quit_state), where it ends the
// prompt loop, so pressed twice it quits. While typing input it is text.
inline constexpr CodePoint quit_key = U'q';

// The key that shows the common keys, and pressed again right after, the
// current state's user instruction. Unless the current state maps it itself.
inline constexpr CodePoint help_key = U'?';

// Gives the lines (UTF-8) of the user instruction for a help topic (see
// State::help_topic), e.g. from the user manual. Empty when there is none.
using HelpSource = std::function<View(std::string_view topic)>;

// True for Enter, which shows the current state's options again. Enter comes
// as U+000A on macOS and Linux (the terminal turns CR into LF) and as U+000D
// from a Windows console.
bool is_enter(CodePoint code_point);

// True for Backspace, which erases the last code point of the input. It comes
// as U+007F on macOS and Linux and as U+0008 from a Windows console.
bool is_backspace(CodePoint code_point);

// Ctrl-U (U+0015) empties the input and goes back to navigating
inline constexpr CodePoint clear_key = 0x15;

// Tab completes the input while typing, as the current state answers (see
// State::complete), or sounds the bell
inline constexpr CodePoint complete_key = U'\t';

// Gives the terminal's width in columns, or nothing when it is not known (see
// terminal_width.hpp)
using TerminalWidth = std::function<std::optional<int>()>;

// A key that works the same in every state, as listed by help_key
struct CommonKey {
  std::string key;     // as shown, e.g. "-" or "Ctrl-D"
  std::string caption; // e.g. "Tillbaka"
};

// The common keys that work in the navigator's current state. Back is only
// listed when the navigator can go back. has_help: the state has a user
// instruction, so the caption of help_key tells to press it again for it.
std::vector<CommonKey> common_keys(Navigator const& navigator, bool has_help = false);

// Writes the common keys, one per line, then the prompt with no line end
void show_common_keys(Navigator const& navigator, std::FILE* out);

// Writes the current state's user instruction from help (nothing when it has
// none), then the prompt with no line end
void show_help(Navigator const& navigator, HelpSource const& help, std::FILE* out);

// Goes between the state names in the prompt
inline constexpr std::string_view path_separator = " > ";

// The names of the states from the root to the current one, then "> ",
// e.g. "Huvudmeny > Projekt> "
std::string prompt_of(Navigator const& navigator);

// As prompt_of but ending in ": ", shown while the user types input
std::string edit_prompt_of(Navigator const& navigator);

// Writes the current state's view and options, one per line as key and caption, then
// the prompt with no line end, as UTF-8. The common keys are not listed.
void show(Navigator const& navigator, std::FILE* out);

// Shows the current state, then reads one code point at a time.
//
// Navigating: a quit key ends the loop. Any other key goes to the current
// state first, and when it answers with a new state, that state is shown.
// When the state has no mapping for the key, the common keys apply: back_key
// goes back (nothing at the root), help_key shows the common keys, and
// pressed again right after, the state's user instruction from help (then
// the common keys again, and so on; any other key starts over), Enter
// shows the state again and quit_key leads to the quit state, where
// quit_key ends the loop (the safety net: quit_key twice quits). Any other printable key
// starts the input with that key.
//
// Whenever a state with a start text (State::start_input) is shown, its input
// starts with that text, e.g. a folder to type a path from. As long as the
// start text is untouched, back_key and help_key work as when navigating. A
// text field (State::is_text_field) starts the input even with no start text,
// and the input stays open when emptied.
//
// An option of the current state that does nothing when pressed (e.g. turning
// past the last page) does nothing: it starts no input.
//
// Typing input: printable keys go into the input and are echoed. Backspace
// and Delete erase one; when the input becomes empty, navigating starts
// again. Esc and Ctrl-U empty the input and navigating starts again. Tab adds
// what the state completes the input with, or lists the candidates and writes
// the input again below them; notes of the completion are shown below the
// input, which is then written again. Enter gives the input to the state's on_input:
// when accepted, the reply is shown, the input is emptied, and the state it
// leads to (if any) becomes current, else the state named by its back_to (if
// any); then the current state is shown. When
// not, the error text is shown and the input is kept, so the user can correct it.
//
// width gives the terminal's width, so that Backspace erases right in an input
// that has wrapped onto more rows. Without it Backspace assumes one row.
//
// Arrows and other function keys are read whole (see KeyParser) and do
// nothing yet. Ends at a quit key or when the input ends, and then ends the
// prompt line.
void run_prompt_loop(KeyReader& keys, Navigator& navigator, std::FILE* out, TerminalWidth width = {},
                     HelpSource help = {});

// Tells the user what a long task started by the input is doing, while the
// state's on_input runs, e.g. "Läser pdf-filer: 3 av 9": on a line of its own
// below the input, each call writing over the last (text is UTF-8, one line).
// The line stays when the state is then shown. Does nothing unless
// run_prompt_loop is giving input to a state, e.g. in tests.
void show_progress(std::string_view text);

} // namespace pyroceph::tui
