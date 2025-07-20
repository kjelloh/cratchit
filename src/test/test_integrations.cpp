#include "test_integrations.hpp"
#include "test_fixtures.hpp"
#include "../cratchit_tea.hpp"
#include <gtest/gtest.h>
#include <iostream>
#include <filesystem>
#include <fstream>

namespace tests::integrations {
    
    // Namespace alias for the test fixture
    using TEATestFixture = tests::fixtures::TEATestFixture;
    
    // Dummy integration test using gtest - always passes
    TEST(IntegrationTests, DummyWorkflowAlwaysPass) {
        EXPECT_TRUE(true);
        ASSERT_STREQ("hello", "hello");
    }

    // TEA Runtime test using TEATestFixture - always passes
    TEST_F(TEATestFixture, TEARuntimeBasicTest) {
        // Verify that the runtime was created successfully
        EXPECT_NE(get_runtime(), nullptr);
        
        // Run a single iteration (TestHead will return 'q' to quit immediately)
        run_single_iteration();
        
        // Verify that we got a return code (0 for normal quit)
        int return_code = get_last_return_code();
        EXPECT_EQ(return_code, 0);
    }

    bool run_all() {
        std::cout << "Running integration tests..." << std::endl;
        
        // Run gtest with filter for integration tests only
        ::testing::GTEST_FLAG(filter) = "IntegrationTests.*:TEATestFixture.*";
        int result = RUN_ALL_TESTS();
        
        return result == 0;
    }

} // end namespace tests::integrations
