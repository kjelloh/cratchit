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
 
TEST(ParserTest,parse_both) {

  {
    auto result = parse(
        parsing::both(
          parsing::literal("magic_value")
          ,parsing::literal("=")
        )
        ,"magic_value=123");

    ASSERT_TRUE(result.has_value());
    auto const& [value,remaining] = result.value();
    EXPECT_EQ(remaining.view().size(),3);
    EXPECT_EQ(value.lhs,parsing::Text{"magic_value"});
    EXPECT_EQ(value.rhs,parsing::Text{"="});
  }

  {
    // first fail
    auto result = parse(
        parsing::both(
          parsing::literal("*not in input*")
          ,parsing::literal("=")
        )
        ,"magic_value=123");

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().pos,0);
  }

  {
    // second fail
    auto result = parse(
        parsing::both(
          parsing::literal("magic_value")
          ,parsing::literal("-")
        )
        ,"magic_value=123");

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().pos,11);
  }


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

TEST(ParserTest,parse_either) {

  {
    auto result = parse(
        parsing::either(
          parsing::literal("magic_value")
          ,parsing::literal("*not in input*")
        )
        ,"magic_value=123");

    ASSERT_TRUE(result.has_value());
    auto const& [value,remaining] = result.value();
    EXPECT_EQ(remaining.view().size(),4);
    EXPECT_TRUE(value.lhs.has_value());
    EXPECT_FALSE(value.rhs.has_value());
  }

  {
    auto result = parse(
        parsing::either(
          parsing::literal("*not in input*")
          ,parsing::literal("magic_value")
        )
        ,"magic_value=123");

    ASSERT_TRUE(result.has_value());
    auto const& [value,remaining] = result.value();
    EXPECT_EQ(remaining.view().size(),4);
    EXPECT_FALSE(value.lhs.has_value());
    EXPECT_TRUE(value.rhs.has_value());
  }

  {
    auto result = parse(
        parsing::either(
          parsing::literal("magic_value")
          ,parsing::literal("magic_value")
        )
        ,"magic_value=123");

    ASSERT_TRUE(result.has_value());
    auto const& [value,remaining] = result.value();
    EXPECT_EQ(remaining.view().size(),4);
    EXPECT_TRUE(value.lhs.has_value());
    EXPECT_FALSE(value.rhs.has_value());
  }

  {
    auto result = parse(
        parsing::either(
          parsing::literal("*not in input*")
          ,parsing::literal("*also not in input*")
        )
        ,"magic_value=123");

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().pos,0);
  }

} // TEST

// TEST(ParserTest,parse_name_pair) {

//   auto result = parse(
//        archive::persistent_acrhive_entry
//       ,"magic_value=123");

//   EXPECT_TRUE(result);

// }