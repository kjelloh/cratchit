#pragma once

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <memory>

// Forward declarations
namespace first {
    class CratchitRuntime;
}

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
    
    // Test fixture for TEA runtime testing with headless mode
    class TEATestFixture : public ::testing::Test {
    protected:
        void SetUp() override;
        void TearDown() override;
        
        // Test helpers
        void run_single_iteration();
        int get_last_return_code() const;
        
        // Access to the runtime for testing
        first::CratchitRuntime* get_runtime();
        
    private:
        std::unique_ptr<first::CratchitRuntime> m_runtime;
        int m_last_return_code = 0;
    };
    
}