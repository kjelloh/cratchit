#include "test_runner.hpp"
#include "test_atomics.hpp"
#include "test_integrations.hpp"
#include "test_fixtures.hpp"
#include <iostream>

namespace tests {
    bool run_all(bool keep_test_files) {
        std::cout << "Running all tests..." << std::endl;
        
        ::testing::InitGoogleTest();
        
        // Set up global test environment with preserve setting
        ::testing::AddGlobalTestEnvironment(fixtures::TestEnvironment::GetInstance());
        
        // bool atomics_pass = atomics::run_all();
        // bool integrations_pass = integrations::run_all();
        
        // bool all_pass = atomics_pass && integrations_pass;

        int result = RUN_ALL_TESTS();        
        auto all_pass = (result == 0);
        
        std::cout << "All tests " << (all_pass ? "PASSED" : "FAILED") << std::endl;
        return all_pass;
    }
}