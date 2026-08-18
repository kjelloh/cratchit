#include "gtest/gtest.h"

#include "src/second/Parser.tpp"

TEST(ParserTest,parse_name_pair_ok) {

  auto result = parse(
       archive::persistent_acrhive_entry
      ,"magic_value=123");

  EXPECT_EQ(1,result.size());

}