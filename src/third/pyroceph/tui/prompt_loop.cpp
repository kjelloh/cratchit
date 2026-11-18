#include "pyroceph/tui/prompt_loop.hpp"

#include "charset/utf8.hpp"
#include "pyroceph/tui/input_buffer.hpp"
#include "pyroceph/tui/key_parser.hpp"
#include "pyroceph/tui/quit_state.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <optional>
#include <print>
#include <utility>
#include <string>
#include <vector>

namespace pyroceph::tui {

namespace {

std::string utf8_of(CodePoint code_point) {
  auto const bytes = charset::utf8::Encoder().push(code_point);
  return std::string(bytes.begin(), bytes.end());
}

std::string utf8_of(std::u32string const& text) {
  std::string result;
  for (auto const code_point : text) result += utf8_of(code_point);
  return result;
}

std::string path_of(Navigator const& navigator) {
  std::string path;
  for (auto const& state : navigator.path()) {
    if (!path.empty()) path += path_separator;
    path += state->name();
  }
  return path;
}

// The prompt ends in "> " while navigating and ": " while typing input. Both
// are two ASCII characters, so the one can be turned into the other in place.
void to_edit_prompt(std::FILE* out) { std::print(out, "\b\b: "); }
void to_navigation_prompt(std::FILE* out) { std::print(out, "\b\b> "); }

// The current state's user instruction from help, or nothing when it has none
View help_lines(Navigator const& navigator, HelpSource const& help) {
  auto const topic = navigator.current().help_topic();
  return help && !topic.empty() ? help(topic) : View{};
}

// Writes the common keys, one per line. has_help tells in the caption of
// help_key that pressing it again shows the state's user instruction.
void write_common_keys(Navigator const& navigator, bool has_help, std::FILE* out) {
  std::print(out, "\n");
  for (auto const& common : common_keys(navigator, has_help)) {
    std::print(out, "  {:<7} {}\n", common.key, common.caption);
  }
}

// Writes the lines of a user instruction
void write_help(View const& lines, std::FILE* out) {
  std::print(out, "\n");
  for (auto const& line : lines) std::print(out, "{}\n", line);
}

std::size_t code_points_of(std::string const& utf8) {
  return static_cast<std::size_t>(std::ranges::count_if(utf8, [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }));
}

// Where show_progress writes, while run_prompt_loop gives input to a state
struct Progress {
  std::FILE* out = nullptr;
  bool started = false;   // the progress line is begun, below the input
  std::size_t width = 0;  // the code points of the longest text on it, to write over
};
Progress progress;

// The loop's own state: the navigator, the input and where the terminal's cursor is
class Loop {
public:
  Loop(Navigator& navigator, std::FILE* out, TerminalWidth width, HelpSource help)
      : navigator_(navigator), out_(out), width_(std::move(width)), help_(std::move(help)) {}

  bool typing() const { return field_ || !input_.empty(); }

  // True when key is one of the current state's options
  bool is_option(CodePoint key) const {
    return std::ranges::any_of(navigator_.current().options(), [key](auto const& option) { return option.first == key; });
  }

  // Shows the current state, and starts its input when it has a start text
  void present() {
    show(navigator_, out_);
    start_input();
  }

  // Handles a key while navigating: the current state first, then the common
  // keys. An option of the state that does nothing now (e.g. turning past the
  // last page) does nothing. Starts the input with the key when it is no
  // action. False when the key ends the loop.
  bool navigate(CodePoint key) {
    if (key != help_key) help_next_ = false;
    if (navigator_.press(key)) {
      present();
    } else if (key == back_key) {
      if (navigator_.back()) present();
    } else if (key == help_key) {
      help();
      std::print(out_, "{}", prompt_of(navigator_));
    } else if (is_enter(key)) {
      present();
    } else if (key == quit_key) {
      // The safety net: quit_key leads to the quit state, and there it quits
      if (is_quit_state(navigator_.current())) return false;
      navigator_.enter(make_quit_state(&navigator_.current()));
      present();
    } else if (is_option(key)) {
      // An option that does nothing now: nothing happens, and no input starts
    } else if (input_.push(key)) {
      to_edit_prompt(out_);
      echo(key);
    }
    return true;
  }

  // Handles a key while typing input
  void edit(CodePoint key) {
    if (key != help_key) help_next_ = false;
    if (key == back_key && input_.text() == navigator_.current().start_input()) {
      // The start text is untouched, so the user has typed nothing yet
      input_.clear();
      field_ = false;
      after_wrap_ = false;
      if (navigator_.back()) present();
    } else if (key == help_key && input_.text() == navigator_.current().start_input()) {
      help();
      reprint();
    } else if (is_backspace(key)) {
      erase_one();
    } else if (key == clear_key) {
      erase_all();
    } else if (key == complete_key) {
      complete();
    } else if (is_enter(key)) {
      enter();
    } else if (input_.push(key)) {
      echo(key);
    }
  }

  // Handles a function key while typing input. Function keys do nothing while navigating.
  void edit(FunctionKey key) {
    help_next_ = false;
    if (key == FunctionKey::escape) {
      erase_all();
    } else if (key == FunctionKey::del) {
      erase_one(); // the cursor is always at the end
    }
  }

private:
  // help_key: the common keys, or right after them the state's user
  // instruction when it has one. Writes no prompt.
  void help() {
    auto const lines = help_lines(navigator_, help_);
    if (help_next_ && !lines.empty()) {
      write_help(lines, out_);
      help_next_ = false;
    } else {
      write_common_keys(navigator_, !lines.empty(), out_);
      help_next_ = !lines.empty();
    }
  }

  void echo(CodePoint key) {
    std::print(out_, "{}", utf8_of(key));
    after_wrap_ = false;
  }

  // Writes the edit prompt and the input on a line of their own
  void reprint() {
    std::print(out_, "{}{}", edit_prompt_of(navigator_), utf8_of(input_.text()));
    after_wrap_ = false;
  }

  void start_input() {
    field_ = false;
    if (!input_.empty()) return;
    auto const text = navigator_.current().start_input();
    bool const field = navigator_.current().is_text_field();
    for (auto const code_point : text) input_.push(code_point);
    if (input_.empty() && !field) return;
    field_ = field;
    to_edit_prompt(out_);
    std::print(out_, "{}", utf8_of(input_.text()));
    after_wrap_ = false;
  }

  // Erases the last code point of the input, on the terminal too. Navigating
  // starts again when the input becomes empty, unless the state is a text field.
  //
  // Each code point is taken to fill one column. When the terminal's width is
  // known, an input that has wrapped onto more rows is erased right: the
  // terminal leaves the cursor in the last column after writing there, and
  // Backspace does not go up a row, so the last column is cleared with escape
  // sequences instead. Without the width, Backspace is used throughout.
  void erase_one() {
    if (input_.empty()) return;
    auto const width = width_ ? width_() : std::nullopt;
    auto const end = code_points_of(edit_prompt_of(navigator_)) + input_.size(); // the column after the last code point
    if (width && end % static_cast<std::size_t>(*width) == 0) {
      // The code point is in the last column. The cursor is on it, or at the
      // start of the next row when a code point there was just erased.
      if (after_wrap_) std::print(out_, "\x1b[A");
      std::print(out_, "\x1b[{}G\x1b[K", *width);
      after_wrap_ = false;
    } else {
      std::print(out_, "\b \b");
      after_wrap_ = width && (end - 1) % static_cast<std::size_t>(*width) == 0;
    }
    input_.erase();
    if (input_.empty() && !field_) to_navigation_prompt(out_);
  }

  // Empties the input, and navigating starts again unless the state is a text field
  void erase_all() {
    while (!input_.empty()) erase_one();
  }

  // Adds what the state completes the input with, or lists the candidates
  // and writes the input again below them, or sounds the bell. Notes are
  // shown below the input, which is then written again.
  void complete() {
    auto const completion = navigator_.current().complete(input_.text());
    if (!completion.added.empty()) {
      for (auto const code_point : completion.added) {
        if (input_.push(code_point)) echo(code_point);
      }
    }
    if (!completion.notes.empty()) {
      std::print(out_, "\n");
      for (auto const& note : completion.notes) std::print(out_, "      {}\n", note);
      reprint();
    } else if (completion.added.empty()) {
      if (completion.candidates.empty()) {
        std::print(out_, "\a");
      } else {
        std::print(out_, "\n");
        for (auto const& candidate : completion.candidates) std::print(out_, "  {}\n", candidate);
        reprint();
      }
    }
  }

  // Gives the input to the state. When accepted, the reply is shown, the
  // input is emptied, the state it leads to (if any) becomes current and is
  // shown. When not, the error text is shown and the input is kept.
  void enter() {
    progress = Progress{out_};
    auto result = navigator_.current().on_input(input_.text());
    progress = Progress{};
    if (result) {
      input_.clear();
      field_ = false;
      if (!result->reply.empty()) std::print(out_, "\n{}", result->reply);
      if (result->next) {
        navigator_.enter(std::move(result->next));
      } else {
        navigator_.follow_back_to();
      }
      present();
    } else {
      std::print(out_, "\n{}\n", result.error());
      reprint();
    }
  }

  Navigator& navigator_;
  std::FILE* out_;
  TerminalWidth width_;
  HelpSource help_;
  bool help_next_ = false; // the last key showed the common keys, so help_key shows the user instruction
  InputBuffer input_;
  bool field_ = false;      // the current state is a text field: typing even when the input is empty
  bool after_wrap_ = false; // the cursor is at the start of a row because a code point there was erased
};

} // namespace

bool is_quit(CodePoint code_point) {
  return code_point == 0x03 || code_point == 0x04 || code_point == 0x1A;
}

bool is_enter(CodePoint code_point) { return code_point == U'\n' || code_point == U'\r'; }

bool is_backspace(CodePoint code_point) { return code_point == 0x7F || code_point == 0x08; }

std::vector<CommonKey> common_keys(Navigator const& navigator, bool has_help) {
  std::vector<CommonKey> keys;
  if (navigator.can_go_back()) keys.push_back({utf8_of(back_key), "Tillbaka"});
  keys.push_back({utf8_of(help_key), has_help ? std::format("Visa gemensamma val, tryck igen för hjälp om {}",
                                                            navigator.current().name())
                                              : std::string("Visa gemensamma val")});
  keys.push_back({"Enter", "Visa valen igen"});
  keys.push_back({"Esc", "Töm indata (även Ctrl-U)"});
  keys.push_back({utf8_of(quit_key), "Avsluta"});
  keys.push_back({"Ctrl-D", "Avsluta (även Ctrl-C och Ctrl-Z)"});
  return keys;
}

void show_common_keys(Navigator const& navigator, std::FILE* out) {
  write_common_keys(navigator, false, out);
  std::print(out, "{}", prompt_of(navigator));
  std::fflush(out);
}

void show_help(Navigator const& navigator, HelpSource const& help, std::FILE* out) {
  write_help(help_lines(navigator, help), out);
  std::print(out, "{}", prompt_of(navigator));
  std::fflush(out);
}

std::string prompt_of(Navigator const& navigator) { return path_of(navigator) + "> "; }

std::string edit_prompt_of(Navigator const& navigator) { return path_of(navigator) + ": "; }

void show(Navigator const& navigator, std::FILE* out) {
  std::print(out, "\n");
  for (auto const& line : navigator.current().view()) {
    std::print(out, "{}\n", line);
  }
  for (auto const& [key, caption] : navigator.current().options()) {
    std::print(out, "  {} {}\n", utf8_of(key), caption);
  }
  std::print(out, "{}", prompt_of(navigator));
  std::fflush(out);
}

void show_progress(std::string_view text) {
  if (!progress.out) return;
  if (!progress.started) {
    std::print(progress.out, "\n");
    progress.started = true;
  }
  auto const shown = code_points_of(std::string(text));
  auto const padding = progress.width > shown ? std::string(progress.width - shown, ' ') : std::string();
  std::print(progress.out, "\r{}{}", text, padding);
  progress.width = std::max(progress.width, shown);
  std::fflush(progress.out);
}

void run_prompt_loop(KeyReader& keys, Navigator& navigator, std::FILE* out, TerminalWidth width, HelpSource help) {
  KeyParser parser(keys);
  Loop loop(navigator, out, std::move(width), std::move(help));
  loop.present();
  while (auto const key = parser.next()) {
    if (auto const* code_point = std::get_if<CodePoint>(&*key)) {
      if (is_quit(*code_point)) break;
      if (!loop.typing()) {
        if (!loop.navigate(*code_point)) break;
      } else {
        loop.edit(*code_point);
      }
    } else if (loop.typing()) {
      loop.edit(std::get<FunctionKey>(*key));
    }
    std::fflush(out);
  }
  std::print(out, "\n");
  std::fflush(out);
}

} // namespace pyroceph::tui
