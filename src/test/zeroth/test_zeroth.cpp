#include "test/zeroth/test_zeroth.hpp"
#include "logger/log.hpp" // logger::
#include "zeroth/main.hpp"
#include "persistent/in/MDInFramework.hpp"
#include <gtest/gtest.h>
#include <iostream>
#include <print>
#include <sstream>

namespace tests::zeroth {

  namespace modelfw_suite {

        static std::map<std::string,std::string> posted_sies_content{
          { "TheITfiedAB20250812_145743.se", R"(
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
}
          )"}
          ,{"cratchit_2025-05-01_2026-04-30.se", R"(
#GEN 20251105
#RAR 0 20250501 20260430
          )"}
        };

    struct TestCratchitMDFileSystemDefacto : public CratchitMDFileSystem::defacto_value_type {    
      virtual persistent::in::MaybeIStream to_maybe_istream(std::filesystem::path file_path) const & final {
        return persistent::in::from_string(
          posted_sies_content[file_path.filename()]);
      }

      // virtual MDMaybeSIEIStream to_md_sie_istream(ConfiguredSIEFilePath const& configured_sie_file_path) const & final {
      //   std::println("TestCratchitMDFileSystemDefacto::to_md_sie_istream('{}')",configured_sie_file_path.first);
      //   return MDMaybeSIEIStream {
      //     .meta = {
      //       .m_year_id = configured_sie_file_path.first
      //       ,.m_file_path = configured_sie_file_path.second
      //     }
      //     ,.defacto = persistent::in::from_string(
      //         posted_sies_content[configured_sie_file_path.second.filename()])
      //   };
      // }
    };

    CratchitMDFileSystem::Defacto to_test_cfs_defacto() {
      CratchitFSDefacto* ptr = new TestCratchitMDFileSystemDefacto();
      return std::make_unique<TestCratchitMDFileSystemDefacto>();
    }

    TEST(ModelTests, Environment2ModelTest) {
      logger::scope_logger log_raii{logger::development_trace,"TEST(ModelTests, Environment2ModelTest)"};

      Environment environment{};
      // sie_file:0 "-1=sie_in/TheITfiedAB20250829_181631.se;-2=sie_in/TheITfiedAB20250828_143351.se;current=sie_in/TheITfiedAB20250812_145743.se"
      environment["sie_file"].push_back({ // pair<size_t,map<string,string>>
         0
        ,{
           {"-1","sie_in/TheITfiedAB20250829_181631.se"}
          ,{"-2","sie_in/TheITfiedAB20250828_143351.se"}
          ,{"current","sie_in/TheITfiedAB20250812_145743.se"}
        }
      });

      char const* sz_sie_file_content = "??";

      auto maybe_istringstream = persistent::in::from_string(
        sz_sie_file_content
      );

      CratchitMDFileSystem md_cfs{
         .meta = {.m_root_path = "*test*"}
        ,.defacto = to_test_cfs_defacto()
      };

      auto model = ::zeroth::model_from_environment_and_md_filesystem(environment,md_cfs);
      std::println("prompt:{}",model->prompt);

      ASSERT_FALSE(true) << "TODO: Implement this test";

    }

    TEST(ModelTests, Model2EnvironmentTest) {
      logger::scope_logger log_raii{logger::development_trace,"TEST(ModelTests, Model2EnvironmentTest)"};

      ASSERT_FALSE(true) << "TODO: Implement this test";

      auto model = std::make_unique<ConcreteModel>();


    }

  }

}