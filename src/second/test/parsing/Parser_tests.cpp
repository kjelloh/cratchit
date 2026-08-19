#include "gtest/gtest.h"

#include "src/second/Parser.tpp"

// TEST(ParserTest,parse_number) {

//   auto result = parse(
//        parsing::natural_number()
//       ,"123");

//   ASSERT_TRUE(result.has_value());
//   auto const& [value,remaining] = result.value();
//   EXPECT_EQ(remaining.view().size(),0);

// }

TEST(ParserTest,parse_literal) {

  auto result = parse(
       parsing::literal("magic_value")
      ,"magic_value=123");

  ASSERT_TRUE(result.has_value());
  auto const& [value,remaining] = result.value();
  EXPECT_EQ(remaining.view().size(),4);

}
 
TEST(ParserTest,parse_sequence) {

  auto result = parse(
      parsing::sequence(
        parsing::literal("magic_value")
        ,parsing::literal("=")
      )
      ,"magic_value=123");

  ASSERT_TRUE(result.has_value());
  auto const& [value,remaining] = result.value();
  EXPECT_EQ(remaining.view().size(),3);

}

// TEST(ParserTest,parse_map) {

//   auto result = parse(
//     parsing::map(
//        parsing::literal("magic_value")
//       ,[](auto value) {
//         return value;
//       }
//     ) // map
//     ,"magic_value=123");

//   ASSERT_TRUE(result.has_value());
//   auto const& [value,remaining] = result.value();
//   EXPECT_EQ(remaining.view().size(),4);

// }

// TEST(ParserTest,parse_flat_map) {

//   auto result = parse(
//     parsing::flat_map(
//        parsing::literal("magic_value")
//       ,[](auto value) {
//         return parsing::literal("=");
//       }
//     ) // map
//     ,"magic_value=123");

//   ASSERT_TRUE(result.has_value());
//   auto const& [value,remaining] = result.value();
//   EXPECT_EQ(remaining.view().size(),3);

// }

// TEST(ParserTest,parse_choice) {

//   {
//     auto result = parse(
//         parsing::choice(
//           parsing::literal("magic_value")
//           ,parsing::literal("*should not match*")
//         )
//         ,"magic_value=123");

//     ASSERT_TRUE(result.has_value());
//     auto const& [value,remaining] = result.value();
//     EXPECT_EQ(remaining.view().size(),4);
//   }

//   {
//     auto result = parse(
//         parsing::choice(
//           parsing::literal("*should not match*")
//           ,parsing::literal("magic_value")
//         )
//         ,"magic_value=123");

//     ASSERT_TRUE(result.has_value());
//     auto const& [value,remaining] = result.value();
//     EXPECT_EQ(remaining.view().size(),4);
//   }

// }

// TEST(ParserTest,parse_name_pair) {

//   auto result = parse(
//        archive::persistent_acrhive_entry
//       ,"magic_value=123");

//   EXPECT_TRUE(result);

// }