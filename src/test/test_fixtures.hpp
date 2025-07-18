#pragma once

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>

namespace tests::fixtures {
    
    // Global test environment to communicate settings across all tests
    class TestEnvironment : public ::testing::Environment {
    public:
      TestEnvironment() = default;
      virtual ~TestEnvironment();
      static TestEnvironment* GetInstance();
    private:
      static TestEnvironment* instance_ptr;
    };        
}