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

    namespace fs = std::filesystem;

    char const* sz_TheITfiedAB20250812_145743_se = R"(
#FLAGGA 0
#FORMAT PC8
#SIETYP 4
#PROGRAM "Fortnox" 3.57.11
#GEN 20250812
#FNR 503072
#FNAMN "The ITfied AB"
#ADRESS "Kjell-Olov H�gdal" "Aron Lindgrens v�g 6, lgh 1801" "17668 J�rf�lla" "070-6850408" 
#RAR 0 20250501 20260430
#RAR -1 20240501 20250430
#ORGNR 556782-8172
#OMFATTN 20260430
#KPTYP EUBAS97

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
#VER A 4 20250601 "Account:SKV Text:Int�ktsr�nta" 20250812
{
#TRANS 1630 {} 1 "" "" 0
#TRANS 8314 {} -1 "" "" 0
}
#VER A 5 20250604 "Account:NORDEA Text:PRIS ENL SPEC" 20250812
{
#TRANS 1920 {} -1.85 "" "" 0
#TRANS 6570 {} 1.85 "" "" 0
}
#VER A 6 20250630 "Kvitto Anthropic Claude" 20250812
{
#TRANS 6230 {} 180 "" "" 0
#TRANS 1920 {} -180 "" "" 0
}
#VER A 7 20250506 "Account:NORDEA Text:PRIS ENL SPEC" 20250812
{
#TRANS 1920 {} -1.85 "" "" 0
#TRANS 6570 {} 1.85 "" "" 0
}
#VER A 8 20250512 "Account:SKV Text:Moms jan 2025 - mars 2025" 20250812
{
#TRANS 1630 {} 1997 "" "" 0
#TRANS 1650 {} -1997 "" "" 0
}
#VER A 9 20250516 "Account:NORDEA Text:BG KONTOINS Message:5050-1030 SK5567828172" 20250812
{
#TRANS 1630 {} -1997 "" "" 0
#TRANS 1920 {} 1997 "" "" 0
}
#VER A 10 20250601 "Account:SKV Text:Int�ktsr�nta" 20250812
{
#TRANS 1630 {} 1 "" "" 0
#TRANS 8314 {} -1 "" "" 0
}

)";

    class ModelTestsFixture : public ::testing::Test {
    protected:

        fs::path m_fixture_tempt_dir;
        fs::path m_fixture_sie_file_path;

        void SetUp() override {
            // unique folder per test run
            m_fixture_tempt_dir = fs::temp_directory_path() /
                      fs::path("modeltests_" + std::to_string(::getpid()) +
                                "_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()));

            fs::create_directories(m_fixture_tempt_dir);

            // create sie_in subfolder to match production relative path semantics
            fs::create_directories(m_fixture_tempt_dir / "sie_in");

            m_fixture_sie_file_path = m_fixture_tempt_dir / "sie_in" / "TheITfiedAB20250812_145743.se";

            std::ofstream ofs(m_fixture_sie_file_path, std::ios::binary);
            ASSERT_TRUE(ofs.is_open()) << "Failed to create test SIE file";

            ofs << sz_TheITfiedAB20250812_145743_se;
            ofs.close();

            ASSERT_TRUE(fs::exists(m_fixture_sie_file_path)) << "Test SIE file not created";
        } // SetUp

        void TearDown() override {
            std::error_code ec;
            fs::remove_all(m_fixture_tempt_dir, ec);

            if (ec)
            {
                std::cerr << "Warning: failed to cleanup test dir: "
                          << m_fixture_tempt_dir << " : " << ec.message() << "\n";
            }
        } // TearDown
    };  

    struct TestCratchitMDFileSystemDefacto : public CratchitMDFileSystem::defacto_value_type {
      std::map<std::string,std::string> m_sie_content_map{};
      TestCratchitMDFileSystemDefacto(
         char const* szPostedSIEContent
        ,char const* szStagedSIEContent)
        : m_sie_content_map{
             { "TheITfiedAB20250812_145743.se", szPostedSIEContent}
            ,{"cratchit_2025-05-01_2026-04-30.se", szStagedSIEContent}} {} 
      virtual persistent::in::text::MaybeIStream to_maybe_istream(std::filesystem::path file_path) & final {
        return persistent::in::text::from_string(
          m_sie_content_map[file_path.filename()]);
      }
    };

    CratchitMDFileSystem::Defacto to_test_cfs_defacto(
      char const* szPostedSIEContent
      ,char const* szStagedSIEContent) {
      return std::make_unique<TestCratchitMDFileSystemDefacto>(szPostedSIEContent,szStagedSIEContent);
    }

    TEST_F(ModelTestsFixture, Environment2ModelTest) {
      logger::scope_logger log_raii{logger::development_trace,"TEST_F(ModelTestsFixture, Environment2ModelTest)"};

      Environment environment{};
      environment["sie_file"].push_back({ // pair<size_t,map<string,string>>
        0
        ,{
          {"current",m_fixture_sie_file_path}
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
        std::println("\n\nprompt:{}",model->prompt);

        ASSERT_TRUE(model->sie_env_map.contains("current")) << std::format("Expected 'current' SIE Environment to exist ok");
        ASSERT_TRUE(model->sie_env_map.at("current").value().unposted().size() == 1 ) << std::format("Expected one staged SIE entry");
        ASSERT_TRUE(model->sie_env_map.at("current").value().journals_entry_count() == 4 ) << std::format("Expected a total of four SIE entries");
      }

    }

    TEST_F(ModelTestsFixture, Environment2ModelSIEStageOverlapOKTest) {
      logger::scope_logger log_raii{logger::development_trace,"TEST_F(ModelTestsFixture, Environment2ModelSIEStageOverlapOKTest)"};

      Environment environment{};
      environment["sie_file"].push_back({ // pair<size_t,map<string,string>>
        0
        ,{
          {"current",m_fixture_sie_file_path}
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
        std::println("\n\nprompt:{}",model->prompt);

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
        std::println("\n\nprompt:{}",model->prompt);

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

    TEST_F(ModelTestsFixture, Environment2ModelSIEStageOverlapConflictTest) {
      logger::scope_logger log_raii{logger::development_trace,"TEST_F(ModelTestsFixture, Environment2ModelSIEStageOverlapConflictTest)"};

      Environment environment{};
      environment["sie_file"].push_back({ // pair<size_t,map<string,string>>
        0
        ,{
          {"current",m_fixture_sie_file_path}
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
        std::println("\n\nprompt:{}",model->prompt);

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
  }

}