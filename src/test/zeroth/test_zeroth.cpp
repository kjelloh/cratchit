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

    struct TestCratchitMDFileSystemDefacto : public CratchitMDFileSystem::defacto_value_type {
      std::map<std::string,char const*> posted_sies_content{
        {"current",R"()"}
        ,{"-1",R"()"}
      };
      virtual MDMaybeSIEIStreams to_md_sie_istreams(ConfiguredSIEFilePaths const& configured_sie_file_paths) const final {
        return configured_sie_file_paths
          | std::views::transform([this](auto const& configured_sie_file_path){
              return MDMaybeSIEIStream {
                .meta = {
                  .m_year_id = configured_sie_file_path.first
                  ,.m_file_path = configured_sie_file_path.second
                }
                ,.defacto = persistent::in::to_maybe_istream(
                    posted_sies_content.at(configured_sie_file_path.second))
              };
            })
          | std::ranges::to<MDMaybeSIEIStreams>();

      }
    };

    CratchitMDFileSystem::Defacto to_test_cfs_defacto() {
      CratchitFSDefacto* ptr = new TestCratchitMDFileSystemDefacto();
      return std::make_unique<TestCratchitMDFileSystemDefacto>();
    }

    TEST(ModelTests, Environment2ModelTest) {
      logger::scope_logger log_raii{logger::development_trace,"TEST(ModelTests, Environment2ModelTest)"};

      Environment environment{};

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