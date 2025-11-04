#include "test/zeroth/test_zeroth.hpp"
#include "logger/log.hpp" // logger::
#include "zeroth/main.hpp"
#include <gtest/gtest.h>
#include <iostream>
#include <print>

namespace tests::zeroth {

  namespace modelfw_suite {

    TEST(ModelTests, Environment2ModelTest) {
      logger::scope_logger log_raii{logger::development_trace,"TEST(ModelTests, Environment2ModelTest)"};

      

      ASSERT_FALSE(true) << "Dummy test should fail";

    }

    TEST(ModelTests, Model2EnvironmentTest) {
      logger::scope_logger log_raii{logger::development_trace,"TEST(ModelTests, Model2EnvironmentTest)"};

      ASSERT_FALSE(true) << "Dummy test should fail";

      auto model = std::make_unique<ConcreteModel>();


    }

  }

}