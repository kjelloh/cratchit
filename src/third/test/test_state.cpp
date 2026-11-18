#include "menu_state.hpp"

#include <gtest/gtest.h>

using pyroceph::tui::Navigator;
using test::menu;

namespace {

std::unique_ptr<pyroceph::tui::State> tree(int* alive = nullptr) {
  return menu("Rot",
              {
                  {U'1', "Ett", [=] { return menu("Ett", {{U'1', "Djup", [=] { return menu("Djup", {}, {}, alive); }}}, {}, alive); }},
                  {U'2', "Två", [=] { return menu("Två", {}, {}, alive); }},
                  {U'å', "Å", [=] { return menu("Å", {}, {}, alive); }},
                  {U'-', "Minus", [=] { return menu("Minus", {}, {}, alive); }},
              },
              {}, alive);
}

} // namespace

TEST(Navigator, StartsAtRoot) {
  Navigator navigator(tree());
  EXPECT_EQ(navigator.current().name(), "Rot");
  EXPECT_FALSE(navigator.can_go_back());
}

TEST(Navigator, KeyTheStateMapsMovesToTheNewState) {
  Navigator navigator(tree());
  EXPECT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Två");
  EXPECT_TRUE(navigator.can_go_back());
}

TEST(Navigator, AnyCodePointCanBeMapped) {
  Navigator navigator(tree());
  EXPECT_TRUE(navigator.press(U'å'));
  EXPECT_EQ(navigator.current().name(), "Å");
}

TEST(Navigator, KeyTheStateDoesNotMapChangesNothing) {
  Navigator navigator(tree());
  EXPECT_FALSE(navigator.press(U'9'));
  EXPECT_EQ(navigator.current().name(), "Rot");
}

TEST(Navigator, BackGoesToTheStateBefore) {
  Navigator navigator(tree());
  navigator.press(U'1');
  navigator.press(U'1');
  EXPECT_EQ(navigator.current().name(), "Djup");
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Ett");
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Rot");
}

TEST(Navigator, PathFollowsKeysAndBack) {
  Navigator navigator(tree());
  ASSERT_EQ(navigator.path().size(), 1U);
  EXPECT_EQ(navigator.path().front()->name(), "Rot");
  navigator.press(U'1');
  navigator.press(U'1');
  ASSERT_EQ(navigator.path().size(), 3U);
  EXPECT_EQ(navigator.path()[1]->name(), "Ett");
  EXPECT_EQ(navigator.path()[2]->name(), "Djup");
  navigator.back();
  EXPECT_EQ(navigator.path().size(), 2U);
}

TEST(Navigator, BackAtRootChangesNothing) {
  Navigator navigator(tree());
  EXPECT_FALSE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Rot");
}

TEST(Navigator, StateCanMapTheBackKey) {
  Navigator navigator(tree());
  EXPECT_TRUE(navigator.press(U'-'));
  EXPECT_EQ(navigator.current().name(), "Minus");
}

TEST(Navigator, StatesAreCreatedOnTheWayInAndDestroyedOnTheWayOut) {
  int alive = 0;
  {
    Navigator navigator(tree(&alive));
    EXPECT_EQ(alive, 1);
    navigator.press(U'1');
    navigator.press(U'1');
    EXPECT_EQ(alive, 3);
    navigator.back();
    EXPECT_EQ(alive, 2);
    navigator.press(U'1'); // a new Djup
    EXPECT_EQ(alive, 3);
  }
  EXPECT_EQ(alive, 0);
}

TEST(State, TakesNoInputByDefault) {
  Navigator navigator(tree());
  auto const result = navigator.current().on_input(U"2025");
  ASSERT_FALSE(result);
  EXPECT_EQ(result.error(), pyroceph::tui::no_input_error);
}

namespace {

class Returning : public pyroceph::tui::State {
public:
  explicit Returning(int* returns) : returns_(returns) {}
  std::string name() const override { return "Tillbaka hit"; }
  pyroceph::tui::View view() const override { return {}; }
  pyroceph::tui::OptionsList options() const override { return {}; }
  pyroceph::tui::OnKeyResult on_key(pyroceph::tui::CodePoint key) override {
    return key == U'1' ? test::menu("Nästa") : nullptr;
  }
  void on_return() override { ++*returns_; }

private:
  int* returns_;
};

} // namespace

TEST(State, BackCallsOnReturnOfTheStateBefore) {
  int returns = 0;
  Navigator navigator(std::make_unique<Returning>(&returns));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(returns, 0);
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(returns, 1);
}

TEST(State, EnterMakesTheStateCurrent) {
  Navigator navigator(tree());
  navigator.enter(test::menu("Inmatat"));
  EXPECT_EQ(navigator.current().name(), "Inmatat");
  EXPECT_TRUE(navigator.back());
}

TEST(State, CompletesNothingAndStartsNoInputByDefault) {
  Navigator navigator(tree());
  EXPECT_TRUE(navigator.current().start_input().empty());
  auto const completion = navigator.current().complete(U"/x");
  EXPECT_TRUE(completion.added.empty());
  EXPECT_TRUE(completion.candidates.empty());
}

namespace {

// Goes back to target once it has acted on 1, and then, when given, on to a state named next
class Done : public pyroceph::tui::State {
public:
  explicit Done(State const* target, std::string next = {}) : target_(target), next_(std::move(next)) {}
  std::string name() const override { return "Klar"; }
  pyroceph::tui::View view() const override { return {}; }
  pyroceph::tui::OptionsList options() const override { return {}; }
  pyroceph::tui::OnKeyResult on_key(pyroceph::tui::CodePoint key) override {
    done_ = key == U'1';
    return nullptr;
  }
  State const* back_to() const override { return done_ ? target_ : nullptr; }
  std::unique_ptr<State> after_back() override { return next_.empty() ? nullptr : test::menu(next_); }

private:
  State const* target_;
  std::string next_;
  bool done_ = false;
};

} // namespace

TEST(State, BackToGoesBackSeveralStatesAndCallsOnReturn) {
  int returns = 0;
  Navigator navigator(std::make_unique<Returning>(&returns));
  ASSERT_TRUE(navigator.press(U'1')); // Nästa
  navigator.enter(test::menu("Mellan"));
  auto const* root = navigator.path().front().get();
  navigator.enter(std::make_unique<Done>(root));
  EXPECT_FALSE(navigator.press(U'2')); // not done: stays
  EXPECT_EQ(navigator.current().name(), "Klar");
  EXPECT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.path().size(), 1u);
  EXPECT_EQ(navigator.current().name(), "Tillbaka hit");
  EXPECT_EQ(returns, 1);
}

TEST(State, AfterBackIsEnteredOnceBack) {
  int returns = 0;
  Navigator navigator(std::make_unique<Returning>(&returns));
  ASSERT_TRUE(navigator.press(U'1')); // Nästa
  auto const* root = navigator.path().front().get();
  navigator.enter(std::make_unique<Done>(root, "Vidare"));
  EXPECT_TRUE(navigator.press(U'1'));
  ASSERT_EQ(navigator.path().size(), 2u);
  EXPECT_EQ(navigator.path()[0]->name(), "Tillbaka hit");
  EXPECT_EQ(navigator.current().name(), "Vidare");
  EXPECT_EQ(returns, 1);
}

TEST(State, BackToAStateNotOnThePathDoesNothing) {
  auto const elsewhere = test::menu("Någon annanstans");
  Navigator navigator(tree());
  navigator.enter(std::make_unique<Done>(elsewhere.get()));
  EXPECT_FALSE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Klar");
}

namespace {

// Counts how often it acted in place on 1
class Counter : public pyroceph::tui::State {
public:
  std::string name() const override { return "Räknare"; }
  pyroceph::tui::View view() const override { return {std::to_string(count)}; }
  pyroceph::tui::OptionsList options() const override { return {{U'1', "Mer…"}}; }
  pyroceph::tui::OnKeyResult on_key(pyroceph::tui::CodePoint) override { return nullptr; }
  bool on_key_in_place(pyroceph::tui::CodePoint key) override { return key == U'1' && ++count > 0; }
  int count = 0;
};

} // namespace

TEST(State, KeyActedOnInPlaceKeepsTheState) {
  Navigator navigator(std::make_unique<Counter>());
  EXPECT_TRUE(navigator.press(U'1'));
  EXPECT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.path().size(), 1u);
  EXPECT_EQ(navigator.current().view(), (pyroceph::tui::View{"2"}));
  EXPECT_FALSE(navigator.press(U'2'));
}
