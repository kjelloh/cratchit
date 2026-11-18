#include "pyroceph/tui/prompt_loop.hpp"

#include "menu_state.hpp"
#include "temporary_file.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <memory>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

using charset::Encoding;
using pyroceph::tui::Navigator;
using test::menu;
using pyroceph::tui::CodePoint;
using pyroceph::tui::KeyReader;

namespace {

KeyReader keys_of(std::vector<std::uint8_t> bytes) {
  auto shared = std::make_shared<std::vector<std::uint8_t>>(std::move(bytes));
  auto next = std::make_shared<std::size_t>(0);
  return KeyReader(
      [shared, next]() -> std::optional<std::uint8_t> {
        if (*next == shared->size()) return std::nullopt;
        return (*shared)[(*next)++];
      },
      Encoding::utf8);
}

// Runs the loop and gives back what it wrote
std::string run(KeyReader& keys, Navigator& navigator, pyroceph::tui::TerminalWidth width = {},
                pyroceph::tui::HelpSource help = {}) {
  auto* out = test::temporary_file();
  if (out == nullptr) {
    ADD_FAILURE() << "No temporary file";
    return {};
  }
  pyroceph::tui::run_prompt_loop(keys, navigator, out, std::move(width), std::move(help));
  std::rewind(out);
  std::string written;
  for (int c; (c = std::fgetc(out)) != EOF;) written.push_back(static_cast<char>(c));
  std::fclose(out);
  return written;
}

std::unique_ptr<pyroceph::tui::State> tree() {
  return menu("Huvudmeny", {{U'1', "Bokslut", [] { return menu("Bokslut", {{U'\u00E5', "År", [] { return menu("År"); }}}); }}});
}

} // namespace

TEST(PromptLoop, ShowsTheFirstStateAtOnce) {
  Navigator navigator(tree());
  auto keys = keys_of({});
  EXPECT_EQ(run(keys, navigator), "\n  1 Bokslut\nHuvudmeny> \n");
}

TEST(PromptLoop, ShowsEachNewStateWithoutCommonKeys) {
  Navigator navigator(tree());
  auto keys = keys_of({'1', '-'});
  EXPECT_EQ(run(keys, navigator),
            "\n  1 Bokslut\nHuvudmeny> "
            "\n  \u00E5 År\nHuvudmeny > Bokslut> "
            "\n  1 Bokslut\nHuvudmeny> "
            "\n");
}

TEST(PromptLoop, ShowsTheViewAboveTheOptions) {
  Navigator navigator(menu("Huvudmeny", {{U'1', "Visa", [] { return test::viewing("Visa", {"Rad ett", "Rad två"}); }}}));
  auto keys = keys_of({'1', '\n', '-'});
  auto const visa = std::string("\nRad ett\nRad två\nHuvudmeny > Visa> ");
  EXPECT_EQ(run(keys, navigator),
            "\n  1 Visa\nHuvudmeny> " + visa + visa + "\n  1 Visa\nHuvudmeny> \n");
}

TEST(PromptLoop, NonAsciiKeyIsAChoice) {
  Navigator navigator(tree());
  auto keys = keys_of({'1', 0xC3, 0xA5});
  run(keys, navigator);
  EXPECT_EQ(navigator.current().name(), "År");
}

TEST(PromptLoop, HelpListsCommonKeysAndKeepsTheState) {
  Navigator navigator(tree());
  auto keys = keys_of({'1', '?'});
  EXPECT_EQ(run(keys, navigator),
            "\n  1 Bokslut\nHuvudmeny> "
            "\n  \u00E5 År\nHuvudmeny > Bokslut> "
            "\n  -       Tillbaka"
            "\n  ?       Visa gemensamma val"
            "\n  Enter   Visa valen igen"
            "\n  Esc     Töm indata (även Ctrl-U)"
            "\n  q       Avsluta"
            "\n  Ctrl-D  Avsluta (även Ctrl-C och Ctrl-Z)"
            "\nHuvudmeny > Bokslut> "
            "\n");
  EXPECT_EQ(navigator.current().name(), "Bokslut");
}

TEST(PromptLoop, EnterShowsTheChoicesAgain) {
  Navigator navigator(tree());
  auto keys = keys_of({'1', '\n', '\r'}); // LF as on macOS and Linux, CR as from a Windows console
  auto const bokslut = std::string("\n  \u00E5 År\nHuvudmeny > Bokslut> ");
  EXPECT_EQ(run(keys, navigator), "\n  1 Bokslut\nHuvudmeny> " + bokslut + bokslut + bokslut + "\n");
  EXPECT_EQ(navigator.current().name(), "Bokslut");
}

TEST(CommonKeys, BackOnlyWhenTheNavigatorCanGoBack) {
  Navigator navigator(tree());
  auto has_back = [&] {
    for (auto const& common : pyroceph::tui::common_keys(navigator)) {
      if (common.key == "-") return true;
    }
    return false;
  };
  EXPECT_FALSE(has_back());
  navigator.press(U'1');
  EXPECT_TRUE(has_back());
}

TEST(PromptLoop, StateIsAskedBeforeTheCommonKeys) {
  Navigator navigator(menu("Rot", {{U'?', "Fråga", [] { return menu("Fråga"); }}}));
  auto keys = keys_of({'?', '-'});
  EXPECT_EQ(run(keys, navigator),
            "\n  ? Fråga\nRot> "
            "\nRot > Fråga> " // ? was the state's, Fråga does not map - so it goes back
            "\n  ? Fråga\nRot> "
            "\n");
}

TEST(PromptOf, RootIsItsName) {
  Navigator navigator(tree());
  EXPECT_EQ(pyroceph::tui::prompt_of(navigator), "Huvudmeny> ");
}

TEST(PromptOf, StateNamesFromTheRoot) {
  Navigator navigator(
      menu("Huvudmeny", {{U'1', "Öppna", [] { return menu("Projekt", {{U'2', "Välj år", [] { return menu("År 2025"); }}}); }}}));
  navigator.press(U'1');
  EXPECT_EQ(pyroceph::tui::prompt_of(navigator), "Huvudmeny > Projekt> "); // the state's name, not the caption
  navigator.press(U'2');
  EXPECT_EQ(pyroceph::tui::prompt_of(navigator), "Huvudmeny > Projekt > År 2025> ");
  navigator.back();
  EXPECT_EQ(pyroceph::tui::prompt_of(navigator), "Huvudmeny > Projekt> ");
}

TEST(PromptLoop, QuitKeysEndTheLoop) {
  for (std::uint8_t quit : {std::uint8_t{0x03}, std::uint8_t{0x04}, std::uint8_t{0x1A}}) {
    Navigator navigator(tree());
    auto keys = keys_of({quit, '1'});
    run(keys, navigator);
    EXPECT_EQ(navigator.current().name(), "Huvudmeny") << int{quit};
  }
}

namespace {

// Bokslut takes four digits and refuses anything else
std::unique_ptr<pyroceph::tui::State> tree_with_input() {
  auto year = [](std::u32string_view text) -> pyroceph::tui::InputResult {
    if (text == U"2025") return std::string("Noterat.");
    if (text == U"tyst") return std::string();
    return std::unexpected(std::string("Fel år."));
  };
  return menu("Huvudmeny", {{U'1', "Bokslut", [=] { return menu("Bokslut", {}, year); }}});
}

} // namespace

TEST(PromptLoop, KeyThatIsNoActionStartsInput) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'9', 'x'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Bokslut\nHuvudmeny> \b\b: 9x\n");
}

TEST(PromptLoop, ActionKeysAreTextWhileTyping) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'9', '1', '-', '?'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Bokslut\nHuvudmeny> \b\b: 91-?\n");
  EXPECT_EQ(navigator.current().name(), "Huvudmeny");
}

TEST(PromptLoop, BackspaceToEmptyGoesBackToNavigating) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'9', 0x7F, '1'}); // after erasing, 1 is a choice again
  run(keys, navigator);
  EXPECT_EQ(navigator.current().name(), "Bokslut");
}

TEST(PromptLoop, WindowsBackspaceErasesToo) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'9', 'x', 0x08, 0x08});
  EXPECT_EQ(run(keys, navigator), "\n  1 Bokslut\nHuvudmeny> \b\b: 9x\b \b\b \b\b\b> \n");
}

TEST(PromptLoop, CtrlUEmptiesInputAndGoesBackToNavigating) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'9', 'x', 0x15, '1'});
  run(keys, navigator);
  EXPECT_EQ(navigator.current().name(), "Bokslut");
}

TEST(PromptLoop, AcceptedInputShowsReplyAndGoesBackToNavigating) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'1', '2', '0', '2', '5', '\n', '-'});
  EXPECT_EQ(run(keys, navigator),
            "\n  1 Bokslut\nHuvudmeny> "
            "\nHuvudmeny > Bokslut> \b\b: 2025"
            "\nNoterat."
            "\nHuvudmeny > Bokslut> "
            "\n  1 Bokslut\nHuvudmeny> " // - went back, so navigating again
            "\n");
}

TEST(PromptLoop, EmptyReplyShowsNothingButTheState) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'1', 't', 'y', 's', 't', '\n'});
  EXPECT_EQ(run(keys, navigator),
            "\n  1 Bokslut\nHuvudmeny> "
            "\nHuvudmeny > Bokslut> \b\b: tyst"
            "\nHuvudmeny > Bokslut> "
            "\n");
}

TEST(PromptLoop, RefusedInputShowsErrorAndKeepsTheInput) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'1', '2', '0', '2', '\n', '5', '\n'}); // 2 0 2, refused, then 5 makes 2025
  EXPECT_EQ(run(keys, navigator),
            "\n  1 Bokslut\nHuvudmeny> "
            "\nHuvudmeny > Bokslut> \b\b: 202"
            "\nFel år.\nHuvudmeny > Bokslut: 202"
            "5"
            "\nNoterat."
            "\nHuvudmeny > Bokslut> "
            "\n");
}

TEST(PromptLoop, StateWithoutOnInputRefusesInput) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'x', '\n'});
  EXPECT_EQ(run(keys, navigator),
            "\n  1 Bokslut\nHuvudmeny> \b\b: x"
            "\nProgrammet tar inte emot text här.\nHuvudmeny: x"
            "\n");
}

TEST(PromptLoop, BackAtRootIsNoInput) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'-', '1'});
  run(keys, navigator);
  EXPECT_EQ(navigator.current().name(), "Bokslut");
}

TEST(PromptLoop, ControlKeyThatIsNoActionIsIgnored) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({0x15, 0x7F, '1'});
  run(keys, navigator);
  EXPECT_EQ(navigator.current().name(), "Bokslut");
}

TEST(PromptLoop, ArrowsDoNotGoIntoTheInput) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'x', 0x1B, '[', 'A', 0x1B, '[', '1', '5', '~', 'y'}); // x, up, F5, y
  EXPECT_EQ(run(keys, navigator), "\n  1 Bokslut\nHuvudmeny> \b\b: xy\n");
}

TEST(PromptLoop, ArrowsDoNothingWhileNavigating) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({0x1B, '[', 'B', '1'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Bokslut\nHuvudmeny> \nHuvudmeny > Bokslut> \n");
}

TEST(PromptLoop, EscEmptiesTheInput) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'x', 'y', 0x1B, '1'}); // Esc then 1: 1 is a choice again
  EXPECT_EQ(run(keys, navigator),
            "\n  1 Bokslut\nHuvudmeny> \b\b: xy\b \b\b \b\b\b> "
            "\nHuvudmeny > Bokslut> \n");
}

TEST(PromptLoop, DeleteErasesLikeBackspace) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'x', 'y', 0x1B, '[', '3', '~'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Bokslut\nHuvudmeny> \b\b: xy\b \b\n");
}

TEST(PromptLoop, QuitKeyAsksFirstAndQuitsWhenPressedAgain) {
  Navigator navigator(tree());
  auto keys = keys_of({'1', 'q', 'q', '-'});
  EXPECT_EQ(run(keys, navigator),
            "\n  1 Bokslut\nHuvudmeny> \n  \u00E5 År\nHuvudmeny > Bokslut> "
            "\nVill du avsluta programmet?\n  q Avsluta\n  N Nej\nHuvudmeny > Bokslut > Avsluta> \n");
  EXPECT_EQ(navigator.current().name(), "Avsluta");
}

TEST(PromptLoop, NoOrBackInTheQuitStateGoesBack) {
  for (std::uint8_t stay : {std::uint8_t{'N'}, std::uint8_t{'-'}}) {
    Navigator navigator(tree());
    auto keys = keys_of({'1', 'q', stay});
    run(keys, navigator);
    EXPECT_EQ(navigator.current().name(), "Bokslut") << int{stay};
    EXPECT_EQ(navigator.path().size(), 2u);
  }
  // Q is not the quit key
  Navigator navigator(tree());
  auto keys = keys_of({'Q'});
  run(keys, navigator);
  EXPECT_EQ(navigator.current().name(), "Huvudmeny");
}

TEST(PromptLoop, QuitKeyIsTextWhileTyping) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'x', 'q'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Bokslut\nHuvudmeny> \b\b: xq\n");
}

TEST(PromptLoop, StateIsAskedBeforeTheQuitKey) {
  Navigator navigator(menu("Rot", {{U'q', "Kvar", [] { return menu("Kvar"); }}}));
  auto keys = keys_of({'q', 'q', 'q', '-'}); // the root maps q, Kvar does not: the second q asks, the third quits
  run(keys, navigator);
  EXPECT_EQ(navigator.current().name(), "Avsluta");
}

namespace {

// Types a word from "/", completes it from a fixed list and leads to a state
// named by the word on Enter
class WordState : public pyroceph::tui::State {
public:
  std::string name() const override { return "Ord"; }
  pyroceph::tui::View view() const override { return {}; }
  pyroceph::tui::OptionsList options() const override { return {}; }
  pyroceph::tui::OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::u32string start_input() const override { return U"/"; }
  pyroceph::tui::Completion complete(std::u32string_view text) const override {
    if (text == U"/a") return {U"pa/", {}, {}};
    if (text == U"/b") return {{}, {"bil", "buss"}, {}};
    if (text == U"/f") return {U"il.se", {}, {"2025 · 3 ver."}};   // one file, described
    if (text == U"/fil.se") return {{}, {}, {"2025 · 3 ver."}}; // its whole name typed
    return {};
  }
  pyroceph::tui::InputResult on_input(std::u32string_view text) override {
    if (text == U"/") return std::unexpected(std::string("Skriv ett ord."));
    return pyroceph::tui::Accepted(menu("Valt"), "Ok.");
  }
};

std::unique_ptr<pyroceph::tui::State> words() {
  return menu("Rot", {{U'1', "Ord", [] { return std::make_unique<WordState>(); }}});
}

} // namespace

TEST(PromptLoop, StartTextStartsTheInputWhenTheStateIsShown) {
  Navigator navigator(words());
  auto keys = keys_of({'1'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Ord\nRot> \nRot > Ord> \b\b: /\n");
}

TEST(PromptLoop, TabAddsWhatTheStateCompletes) {
  Navigator navigator(words());
  auto keys = keys_of({'1', 'a', '\t'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Ord\nRot> \nRot > Ord> \b\b: /apa/\n");
}

TEST(PromptLoop, TabListsTheCandidatesAndWritesTheInputAgain) {
  Navigator navigator(words());
  auto keys = keys_of({'1', 'b', '\t'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Ord\nRot> \nRot > Ord> \b\b: /b\n  bil\n  buss\nRot > Ord: /b\n");
}

TEST(PromptLoop, TabShowsTheNotesBelowTheCompletedInputAndWritesItAgain) {
  Navigator navigator(words());
  auto keys = keys_of({'1', 'f', '\t', '\t'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Ord\nRot> \nRot > Ord> \b\b: /fil.se\n      2025 · 3 ver.\nRot > Ord: /fil.se"
                                  "\n      2025 · 3 ver.\nRot > Ord: /fil.se\n");
}

TEST(PromptLoop, TabWithoutCompletionSoundsTheBell) {
  Navigator navigator(words());
  auto keys = keys_of({'1', 'x', '\t'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Ord\nRot> \nRot > Ord> \b\b: /x\a\n");
}

TEST(PromptLoop, AcceptedInputLeadsToTheNextState) {
  Navigator navigator(words());
  auto keys = keys_of({'1', 'x', '\n', '-'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Ord\nRot> \nRot > Ord> \b\b: /x"
                                  "\nOk.\nRot > Ord > Valt> "
                                  "\nRot > Ord> \b\b: /\n");
  EXPECT_EQ(navigator.current().name(), "Ord");
}

TEST(PromptLoop, EscEmptiesTheStartTextAndBackGoesBack) {
  Navigator navigator(words());
  auto keys = keys_of({'1', 0x1B, '-'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Ord\nRot> \nRot > Ord> \b\b: /\b \b\b\b> \n  1 Ord\nRot> \n");
}

// "Rot > Ord: /" is 12 columns, so on a terminal 13 wide x is in the last column and y wraps
TEST(PromptLoop, BackspaceErasesAcrossARowWhenTheWidthIsKnown) {
  Navigator navigator(words());
  auto keys = keys_of({'1', 'x', 'y', 0x7F, 0x7F, 0x7F});
  auto const width = []() -> std::optional<int> { return 13; };
  EXPECT_EQ(run(keys, navigator, width), "\n  1 Ord\nRot> \nRot > Ord> \b\b: /xy"
                                         "\b \b"            // y, in the first column of the second row
                                         "\x1b[A\x1b[13G\x1b[K" // x, in the last column of the first row
                                         "\b \b\b\b> \n");   // /
}

TEST(PromptLoop, BackspaceInTheLastColumnStaysOnTheRow) {
  Navigator navigator(words());
  auto keys = keys_of({'1', 'x', 0x7F});
  auto const width = []() -> std::optional<int> { return 13; };
  EXPECT_EQ(run(keys, navigator, width), "\n  1 Ord\nRot> \nRot > Ord> \b\b: /x\x1b[13G\x1b[K\n");
}

TEST(PromptLoop, BackGoesBackWhileTheStartTextIsUntouched) {
  Navigator navigator(words());
  auto keys = keys_of({'1', '-', '1', 'x', '-'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Ord\nRot> \nRot > Ord> \b\b: /"
                                  "\n  1 Ord\nRot> "
                                  "\nRot > Ord> \b\b: /x-\n");
  EXPECT_EQ(navigator.current().name(), "Ord");
}

namespace {

// Ord with a help topic
class HelpedWordState : public WordState {
public:
  std::string help_topic() const override { return "ord"; }
};

pyroceph::tui::View help_of(std::string_view topic) {
  if (topic == "ord") return {"Skriv ett ord."};
  return {};
}

} // namespace

namespace {

std::string const common_keys_after_back =
    "  Enter   Visa valen igen\n  Esc     Töm indata (även Ctrl-U)\n  q       Avsluta\n"
    "  Ctrl-D  Avsluta (även Ctrl-C och Ctrl-Z)\n";

std::unique_ptr<pyroceph::tui::State> helped() {
  return menu("Rot", {{U'1', "Ord", [] { return std::make_unique<HelpedWordState>(); }}});
}

} // namespace

TEST(PromptLoop, HelpKeyShowsOnlyTheCommonKeysWhereThereIsNoHelp) {
  Navigator navigator(helped());
  auto keys = keys_of({'?', '?'});
  auto const once = "\n  ?       Visa gemensamma val\n" + common_keys_after_back + "Rot> ";
  EXPECT_EQ(run(keys, navigator, {}, help_of), "\n  1 Ord\nRot> " + once + once + "\n");
}

TEST(PromptLoop, HelpKeyAgainShowsTheHelpThenTheCommonKeysAgain) {
  Navigator navigator(helped());
  auto keys = keys_of({'1', 0x1B, '?', '?', '?'}); // Esc empties the start text, so Ord is navigated
  auto const keys_once = "\n  -       Tillbaka\n  ?       Visa gemensamma val, tryck igen för hjälp om Ord\n" +
                         common_keys_after_back + "Rot > Ord> ";
  EXPECT_EQ(run(keys, navigator, {}, help_of), "\n  1 Ord\nRot> \nRot > Ord> \b\b: /\b \b\b\b> " + keys_once +
                                                   "\nSkriv ett ord.\nRot > Ord> " + keys_once + "\n");
}

TEST(PromptLoop, AnotherKeyBetweenStartsTheHelpOver) {
  Navigator navigator(helped());
  auto keys = keys_of({'1', 0x1B, '?', '\n', '?'});
  auto const written = run(keys, navigator, {}, help_of);
  EXPECT_EQ(written.find("Skriv ett ord."), std::string::npos) << written;
}

TEST(PromptLoop, HelpKeyWorksWhileTheStartTextIsUntouched) {
  Navigator navigator(helped());
  auto keys = keys_of({'1', '?', '?', 'x', '?'});
  EXPECT_EQ(run(keys, navigator, {}, help_of),
            "\n  1 Ord\nRot> \nRot > Ord> \b\b: /"
            "\n  -       Tillbaka\n  ?       Visa gemensamma val, tryck igen för hjälp om Ord\n" +
                common_keys_after_back + "Rot > Ord: /" + "\nSkriv ett ord.\nRot > Ord: /x?\n");
}

namespace {

// A text field that takes any text but an empty one
class Field : public pyroceph::tui::State {
public:
  std::string name() const override { return "Fält"; }
  pyroceph::tui::View view() const override { return {}; }
  pyroceph::tui::OptionsList options() const override { return {}; }
  pyroceph::tui::OnKeyResult on_key(CodePoint) override { return nullptr; }
  bool is_text_field() const override { return true; }
  pyroceph::tui::InputResult on_input(std::u32string_view text) override {
    if (text.empty()) return std::unexpected(std::string("Skriv något."));
    return pyroceph::tui::Accepted(menu("Klart"));
  }
};

std::unique_ptr<pyroceph::tui::State> with_field() {
  return menu("Rot", {{U'1', "Fält", [] { return std::make_unique<Field>(); }}});
}

} // namespace

TEST(PromptLoop, TextFieldTypesEvenWhenEmpty) {
  Navigator navigator(with_field());
  auto keys = keys_of({'1', 'q', 'x'}); // q is text in a field
  EXPECT_EQ(run(keys, navigator), "\n  1 Fält\nRot> \nRot > Fält> \b\b: qx\n");
  EXPECT_EQ(navigator.current().name(), "Fält");
}

TEST(PromptLoop, TextFieldStaysOpenWhenEmptied) {
  Navigator navigator(with_field());
  auto keys = keys_of({'1', 'x', 0x7F, 0x7F, 'y', 0x15, 'z'}); // Backspace on empty does nothing; Ctrl-U empties
  EXPECT_EQ(run(keys, navigator), "\n  1 Fält\nRot> \nRot > Fält> \b\b: x\b \by\b \bz\n");
}

TEST(PromptLoop, BackLeavesAnUntouchedTextField) {
  Navigator navigator(with_field());
  auto keys = keys_of({'1', '-', '1', 'x', '-'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Fält\nRot> \nRot > Fält> \b\b: "
                                  "\n  1 Fält\nRot> "
                                  "\nRot > Fält> \b\b: x-\n");
}

TEST(PromptLoop, EnterInATextFieldGivesTheInputToTheState) {
  Navigator navigator(with_field());
  auto keys = keys_of({'1', '\n', 'x', '\n'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Fält\nRot> \nRot > Fält> \b\b: "
                                  "\nSkriv något.\nRot > Fält: "
                                  "x\nRot > Fält > Klart> \n");
}

TEST(PromptLoop, AnOptionThatDoesNothingNowStartsNoInput) {
  // ">" leads nowhere now (its next is nothing), e.g. at the last page: pressing it shows nothing new and types nothing
  Navigator navigator(menu("Rot", {{U'>', "Nästa", [] { return std::unique_ptr<pyroceph::tui::State>(); }}}));
  auto keys = keys_of({'>', '>'});
  EXPECT_EQ(run(keys, navigator), "\n  > Nästa\nRot> \n");
}

TEST(PromptLoop, ProgressIsShownOnOneLineBelowTheInput) {
  int alive = 0;
  Navigator navigator(test::menu("Rot", {}, [](std::u32string_view) -> pyroceph::tui::InputResult {
    pyroceph::tui::show_progress("Läser 1 av 2");
    pyroceph::tui::show_progress("Klart");
    return pyroceph::tui::Accepted("ok");
  }, &alive));
  auto keys = keys_of({'x', '\n'});
  EXPECT_EQ(run(keys, navigator), "\nRot> \b\b: x\n\rL\xC3\xA4ser 1 av 2\rKlart       \nok\nRot> \n");
  // Outside the loop it does nothing
  pyroceph::tui::show_progress("ingenting");
}
