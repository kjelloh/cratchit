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

    TEST(ModelTests, Environment2ModelTest) {
      logger::scope_logger log_raii{logger::development_trace,"TEST(ModelTests, Environment2ModelTest)"};

      Environment environment{};

      char const* sz_sie_file_content = "??";

      auto maybe_istringstream = persistent::in::from_string(
        sz_sie_file_content
      );
      auto model = ::zeroth::model_from_environment(environment);

      // Test conflict resolution posted + stage sie entries
      {
        
      }

      ASSERT_FALSE(true) << "TODO: Implement this test";

    }

    TEST(ModelTests, Model2EnvironmentTest) {
      logger::scope_logger log_raii{logger::development_trace,"TEST(ModelTests, Model2EnvironmentTest)"};

      ASSERT_FALSE(true) << "TODO: Implement this test";

      auto model = std::make_unique<ConcreteModel>();


    }

  }

}