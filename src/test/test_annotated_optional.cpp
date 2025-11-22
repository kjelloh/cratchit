#include "test_atomics.hpp"
#include "test_fixtures.hpp"
#include "logger/log.hpp"
#include "functional/maybe.hpp"
#include <gtest/gtest.h>
#include <optional>
#include <string>

namespace tests::annotated_optional {

  namespace from_suite {

    TEST(AnnotatedOptionalFromTests, FromWithValueReturnsPopulatedOptional) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AnnotatedOptionalFromTests, FromWithValueReturnsPopulatedOptional)"};

      auto result = cratchit::functional::AnnotatedOptional<int>::from(
        std::optional<int>{42}, "error message"
      );

      ASSERT_TRUE(result) << "Expected populated optional";
      EXPECT_EQ(result.value(), 42);
      EXPECT_TRUE(result.m_messages.empty()) << "Expected no messages on success";
    }

    TEST(AnnotatedOptionalFromTests, FromWithNulloptReturnsEmptyWithMessage) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AnnotatedOptionalFromTests, FromWithNulloptReturnsEmptyWithMessage)"};

      auto result = cratchit::functional::AnnotatedOptional<int>::from(
        std::nullopt, "value was missing"
      );

      EXPECT_FALSE(result) << "Expected empty optional";
      ASSERT_EQ(result.m_messages.size(), 1);
      EXPECT_EQ(result.m_messages[0], "value was missing");
    }

    TEST(AnnotatedOptionalFromTests, FromWithEmptyMessageOnNullopt) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AnnotatedOptionalFromTests, FromWithEmptyMessageOnNullopt)"};

      auto result = cratchit::functional::AnnotatedOptional<std::string>::from(
        std::nullopt, ""
      );

      EXPECT_FALSE(result) << "Expected empty optional";
      ASSERT_EQ(result.m_messages.size(), 1);
      EXPECT_EQ(result.m_messages[0], "") << "Expected empty message string";
    }

    TEST(AnnotatedOptionalFromTests, FromPreservesValueExactly) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AnnotatedOptionalFromTests, FromPreservesValueExactly)"};

      std::string expected = "test string with special chars: åäö";
      auto result = cratchit::functional::AnnotatedOptional<std::string>::from(
        std::optional<std::string>{expected}, "error"
      );

      ASSERT_TRUE(result) << "Expected populated optional";
      EXPECT_EQ(result.value(), expected) << "Expected exact value preservation";
    }

  } // from_suite

  namespace to_caption_suite {

    TEST(AnnotatedOptionalToCaptionTests, ToCaptionReturnsString) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AnnotatedOptionalToCaptionTests, ToCaptionReturnsString)"};

      cratchit::functional::AnnotatedOptional<int> opt{};
      opt.m_value = 42;

      auto caption = opt.to_caption();

      EXPECT_FALSE(caption.empty()) << "Expected non-empty caption";
    }

    TEST(AnnotatedOptionalToCaptionTests, ToCaptionOnEmptyOptional) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AnnotatedOptionalToCaptionTests, ToCaptionOnEmptyOptional)"};

      cratchit::functional::AnnotatedOptional<int> empty_opt{};

      auto caption = empty_opt.to_caption();

      EXPECT_FALSE(caption.empty()) << "Expected caption even for empty optional";
    }

    TEST(AnnotatedOptionalToCaptionTests, ToCaptionOnPopulatedOptional) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AnnotatedOptionalToCaptionTests, ToCaptionOnPopulatedOptional)"};

      auto opt = cratchit::functional::AnnotatedOptional<std::string>::from(
        std::optional<std::string>{"hello"}, "error"
      );

      auto caption = opt.to_caption();

      // Currently returns "?caption?" - test that it returns something
      EXPECT_FALSE(caption.empty()) << "Expected non-empty caption";
    }

  } // to_caption_suite

  namespace to_annotated_nullopt_suite {

    TEST(ToAnnotatedNulloptTests, LiftedFunctionReturnsAnnotatedOptionalWithValue) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(ToAnnotatedNulloptTests, LiftedFunctionReturnsAnnotatedOptionalWithValue)"};

      auto parse_int = [](std::string_view s) -> std::optional<int> {
        if (s == "42") return 42;
        return std::nullopt;
      };

      auto lifted = cratchit::functional::to_annotated_nullopt(
        parse_int, "failed to parse integer"
      );

      auto result = lifted("42");

      ASSERT_TRUE(result) << "Expected successful parse";
      EXPECT_EQ(result.value(), 42);
      EXPECT_TRUE(result.m_messages.empty()) << "Expected no messages on success";
    }

    TEST(ToAnnotatedNulloptTests, LiftedFunctionReturnsAnnotatedOptionalWithMessageOnNullopt) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(ToAnnotatedNulloptTests, LiftedFunctionReturnsAnnotatedOptionalWithMessageOnNullopt)"};

      auto parse_int = [](std::string_view s) -> std::optional<int> {
        if (s == "42") return 42;
        return std::nullopt;
      };

      auto lifted = cratchit::functional::to_annotated_nullopt(
        parse_int, "failed to parse integer"
      );

      auto result = lifted("not a number");

      EXPECT_FALSE(result) << "Expected failed parse";
      ASSERT_EQ(result.m_messages.size(), 1);
      EXPECT_EQ(result.m_messages[0], "failed to parse integer");
    }

    TEST(ToAnnotatedNulloptTests, LiftedFunctionPreservesArguments) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(ToAnnotatedNulloptTests, LiftedFunctionPreservesArguments)"};

      auto add_if_positive = [](int a, int b) -> std::optional<int> {
        if (a > 0 && b > 0) return a + b;
        return std::nullopt;
      };

      auto lifted = cratchit::functional::to_annotated_nullopt(
        add_if_positive, "both arguments must be positive"
      );

      auto result = lifted(10, 20);

      ASSERT_TRUE(result) << "Expected successful addition";
      EXPECT_EQ(result.value(), 30) << "Expected correct sum";
    }

    TEST(ToAnnotatedNulloptTests, LiftedFunctionComposesWithAndThen) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(ToAnnotatedNulloptTests, LiftedFunctionComposesWithAndThen)"};

      auto parse_int = [](std::string_view s) -> std::optional<int> {
        if (s == "42") return 42;
        return std::nullopt;
      };

      auto double_if_even = [](int n) -> cratchit::functional::AnnotatedOptional<int> {
        if (n % 2 == 0) {
          return cratchit::functional::AnnotatedOptional<int>::from(
            std::optional<int>{n * 2}, "odd number"
          );
        }
        return cratchit::functional::AnnotatedOptional<int>::from(
          std::nullopt, "odd number"
        );
      };

      auto lifted = cratchit::functional::to_annotated_nullopt(
        parse_int, "failed to parse integer"
      );

      auto result = lifted("42").and_then(double_if_even);

      ASSERT_TRUE(result) << "Expected successful composition";
      EXPECT_EQ(result.value(), 84) << "Expected doubled value";
      EXPECT_TRUE(result.m_messages.empty()) << "Expected no messages on success";
    }

  } // to_annotated_nullopt_suite

} // tests::annotated_optional
