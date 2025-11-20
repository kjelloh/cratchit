#include "test_runner.hpp"
#include "test_atomics.hpp"
#include "test_integrations.hpp"
#include "test_fixtures.hpp"
#include <iostream>

namespace tests {
    bool run_all(int argc, char** argv, bool keep_test_files) {
        std::cout << "Running all tests..." << std::endl;
        
        ::testing::InitGoogleTest(&argc,argv);
        
        ::testing::AddGlobalTestEnvironment(fixtures::TestEnvironment::GetInstance());
        
        int result = RUN_ALL_TESTS();        
        auto all_pass = (result == 0);
        
        std::cout << "All tests " << (all_pass ? "PASSED" : "FAILED") << std::endl;
        return all_pass;
    }
}