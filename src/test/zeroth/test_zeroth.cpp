#include "test/zeroth/test_zeroth.hpp"
#include "logger/log.hpp" // logger::
#include "zeroth/main.hpp"
#include "persistent/in/maybe.hpp"
#include <gtest/gtest.h>
#include <iostream>
#include <print>
#include <sstream>

namespace tests::zeroth {

  char const* szThreePostedSIE = R"(
#GEN 20251105
#RAR 0 20250501 20260430

#VER A 1 20250506 "Account:NORDEA Text:PRIS ENL SPEC" 20250812
{
#TRANS 1920 {} -1.85 "" "" 0
#TRANS 6570 {} 1.85 "" "" 0
}
#VER A 2 20250512 "Account:SKV Text:Moms jan 2025 - mars 2025" 20250812
{
#TRANS 1630 {} 1997 "" "" 0
#TRANS 1650 {} -1997 "" "" 0
}
#VER A 3 20250516 "Account:NORDEA Text:BG KONTOINS Message:5050-1030 SK5567828172" 20250812
{
#TRANS 1630 {} -1997 "" "" 0
#TRANS 1920 {} 1997 "" "" 0
})";

  char const* szOneNewStagedSIE = R"(
#GEN 20251105
#RAR 0 20250501 20260430

#VER A 4 20250516 "Account:NORDEA Text:BG KONTOINS Message:5050-1030 SK5567828172" 20250812
{
#TRANS 1630 {} -1997 "" "" 0
#TRANS 1920 {} 1997 "" "" 0
})";

  char const* szOneNowPostedStagedSIE = R"(
#GEN 20251105
#RAR 0 20250501 20260430

#VER A 3 20250516 "Account:NORDEA Text:BG KONTOINS Message:5050-1030 SK5567828172" 20250812
{
#TRANS 1630 {} -1997 "" "" 0
#TRANS 1920 {} 1997 "" "" 0
})";

  char const* szOneConflictingStagedSIE = R"(
#GEN 20251105
#RAR 0 20250501 20260430

#VER A 3 20250516 "Account:NORDEA Text:BG KONTOINS Message:5050-1030 SK5567828172" 20250812
{
#TRANS 1630 {} -1998 "" "" 0
#TRANS 1920 {} 1998 "" "" 0
})";

  namespace modelfw_suite {

    // static std::map<std::string,std::string> sie_content_map{
    //   { "TheITfiedAB20250812_145743.se", szThreePostedSIE}
    //   ,{"cratchit_2025-05-01_2026-04-30.se", szOneNewStagedSIE}
    // };

    struct TestCratchitMDFileSystemDefacto : public CratchitMDFileSystem::defacto_value_type {
      std::map<std::string,std::string> m_sie_content_map{};
      TestCratchitMDFileSystemDefacto(
         char const* szPostedSIEContent
        ,char const* szStagedSIEContent)
        : m_sie_content_map{
             { "TheITfiedAB20250812_145743.se", szPostedSIEContent}
            ,{"cratchit_2025-05-01_2026-04-30.se", szStagedSIEContent}} {} 
      virtual persistent::in::MaybeIStream to_maybe_istream(std::filesystem::path file_path) & final {
        return persistent::in::from_string(
          m_sie_content_map[file_path.filename()]);
      }
    };

    CratchitMDFileSystem::Defacto to_test_cfs_defacto(
      char const* szPostedSIEContent
      ,char const* szStagedSIEContent) {
      return std::make_unique<TestCratchitMDFileSystemDefacto>(szPostedSIEContent,szStagedSIEContent);
    }

    TEST(ModelTests, Environment2ModelTest) {
      logger::scope_logger log_raii{logger::development_trace,"TEST(ModelTests, Environment2ModelTest)"};

      Environment environment{};
      // sie_file:0 "-1=sie_in/TheITfiedAB20250829_181631.se;-2=sie_in/TheITfiedAB20250828_143351.se;current=sie_in/TheITfiedAB20250812_145743.se"
      environment["sie_file"].push_back({ // pair<size_t,map<string,string>>
        0
        ,{
          //  {"-1","sie_in/TheITfiedAB20250829_181631.se"}
          // ,{"-2","sie_in/TheITfiedAB20250828_143351.se"}
          // ,{"current","sie_in/TheITfiedAB20250812_145743.se"}
          {"current","sie_in/TheITfiedAB20250812_145743.se"}
        }
      });

      {
        CratchitMDFileSystem md_cfs{
          .meta = {.m_root_path = "*test dummy path*"}
          ,.defacto = to_test_cfs_defacto(
             szThreePostedSIE
            ,szOneNewStagedSIE)
        };

        auto model = ::zeroth::model_from_environment_and_md_filesystem(environment,md_cfs);
        std::println("prompt:{}",model->prompt);

        // ASSERT_TRUE(model->sie_env_map. ) << std::format("");
        ASSERT_TRUE(model->sie_env_map.contains("current")) << std::format("Expected 'current' SIE Environment to exist ok");
        ASSERT_TRUE(model->sie_env_map.at("current").value().unposted().size() == 1 ) << std::format("Expected one staged SIE entry");
        ASSERT_TRUE(model->sie_env_map.at("current").value().journals_entry_count() == 4 ) << std::format("Expected a total of four SIE entries");
      }

    }

    TEST(ModelTests, Environment2ModelSIEStageOverlapOKTest) {
      logger::scope_logger log_raii{logger::development_trace,"TEST(ModelTests, Environment2ModelSIEStageOverlapOKTest)"};

      Environment environment{};
      // sie_file:0 "-1=sie_in/TheITfiedAB20250829_181631.se;-2=sie_in/TheITfiedAB20250828_143351.se;current=sie_in/TheITfiedAB20250812_145743.se"
      environment["sie_file"].push_back({ // pair<size_t,map<string,string>>
        0
        ,{
          //  {"-1","sie_in/TheITfiedAB20250829_181631.se"}
          // ,{"-2","sie_in/TheITfiedAB20250828_143351.se"}
          // ,{"current","sie_in/TheITfiedAB20250812_145743.se"}
          {"current","sie_in/TheITfiedAB20250812_145743.se"}
        }
      });

      {
        CratchitMDFileSystem md_cfs{
          .meta = {.m_root_path = "*test dummy path*"}
          ,.defacto = to_test_cfs_defacto(
             szThreePostedSIE
            ,szOneNowPostedStagedSIE)
        };

        auto model = ::zeroth::model_from_environment_and_md_filesystem(environment,md_cfs);
        std::println("prompt:{}",model->prompt);

        // ASSERT_TRUE(model->sie_env_map. ) << std::format("");
        auto const& sie_env = model->sie_env_map.at("current").value();
        auto const&  unposted = sie_env.unposted();
        auto journals_entry_count = sie_env.journals_entry_count();
        ASSERT_TRUE(unposted.size() == 0 ) << std::format(
           "Expected zero now staged SIE entry but encountered:{}"
          ,unposted.size());
        ASSERT_TRUE(journals_entry_count == 3 ) << std::format(
           "Expected a total of three now posted SIE entries but encountered:{}"
          ,journals_entry_count);
      }

      {
        CratchitMDFileSystem md_cfs{
          .meta = {.m_root_path = "*test dummy path*"}
          ,.defacto = to_test_cfs_defacto(
             szThreePostedSIE
            ,szThreePostedSIE) // staged = posted (all should cancel out)
        };

        auto model = ::zeroth::model_from_environment_and_md_filesystem(environment,md_cfs);
        std::println("prompt:{}",model->prompt);

        // ASSERT_TRUE(model->sie_env_map. ) << std::format("");
        auto const& sie_env = model->sie_env_map.at("current").value();
        auto const&  unposted = sie_env.unposted();
        auto journals_entry_count = sie_env.journals_entry_count();
        ASSERT_TRUE(unposted.size() == 0 ) << std::format(
           "Expected zero now staged SIE entry but encountered:{}"
          ,unposted.size());
        ASSERT_TRUE(journals_entry_count == 3 ) << std::format(
           "Expected a total of three now posted SIE entries but encountered:{}"
          ,journals_entry_count);
      }

    }

    TEST(ModelTests, Environment2ModelSIEStageOverlapConflictTest) {
      logger::scope_logger log_raii{logger::development_trace,"TEST(ModelTests, Environment2ModelSIEStageOverlapConflictTest)"};

      Environment environment{};
      // sie_file:0 "-1=sie_in/TheITfiedAB20250829_181631.se;-2=sie_in/TheITfiedAB20250828_143351.se;current=sie_in/TheITfiedAB20250812_145743.se"
      environment["sie_file"].push_back({ // pair<size_t,map<string,string>>
        0
        ,{
          //  {"-1","sie_in/TheITfiedAB20250829_181631.se"}
          // ,{"-2","sie_in/TheITfiedAB20250828_143351.se"}
          // ,{"current","sie_in/TheITfiedAB20250812_145743.se"}
          {"current","sie_in/TheITfiedAB20250812_145743.se"}
        }
      });

      {
        CratchitMDFileSystem md_cfs{
          .meta = {.m_root_path = "*test dummy path*"}
          ,.defacto = to_test_cfs_defacto(
             szThreePostedSIE
            ,szOneConflictingStagedSIE)
        };

        auto model = ::zeroth::model_from_environment_and_md_filesystem(environment,md_cfs);
        std::println("prompt:{}",model->prompt);

        // ASSERT_TRUE(model->sie_env_map. ) << std::format("");
        auto const& sie_env = model->sie_env_map.at("current").value();
        auto const&  unposted = sie_env.unposted();
        auto journals_entry_count = sie_env.journals_entry_count();
        ASSERT_TRUE(unposted.size() == 0 ) << std::format(
           "Expected zero now staged SIE entry but encountered:{}"
          ,unposted.size());
        ASSERT_TRUE(journals_entry_count == 3 ) << std::format(
           "Expected a total of three now posted SIE entries but encountered:{}"
          ,journals_entry_count);
      }


    }

    TEST(ModelTests, Model2EnvironmentTest) {
      logger::scope_logger log_raii{logger::development_trace,"TEST(ModelTests, Model2EnvironmentTest)"};

      ASSERT_FALSE(true) << "TODO: Implement this test";

      auto model = std::make_unique<ConcreteModel>();


    }

  }

}