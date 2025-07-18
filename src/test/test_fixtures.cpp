#include "test_fixtures.hpp"
#include "../cratchit_tea.hpp"
#include "../tea/test_head.hpp"

namespace tests::fixtures {

    TestEnvironment* TestEnvironment::instance_ptr = nullptr;

    TestEnvironment::~TestEnvironment() {
        // Google test will own this as the global test environment.
        // Thus we must NOT delete the instance (we do NOT own it)
        // Google test will delete it (a delete here will result in double delete...)
    }

    TestEnvironment* TestEnvironment::GetInstance() {
        if (TestEnvironment::instance_ptr == nullptr) {
            TestEnvironment::instance_ptr = new TestEnvironment();
        }
        return TestEnvironment::instance_ptr;
    }
    
    void TEATestFixture::SetUp() {
        // Create a TestHead for headless testing
        auto test_head = std::make_unique<TEA::TestHead>();
        
        // Create the TEA runtime with the TestHead
        m_runtime = std::make_unique<first::CratchitRuntime>(
            first::init, 
            first::view, 
            first::update, 
            std::move(test_head)
        );
    }
    
    void TEATestFixture::TearDown() {
        // Reset the runtime (unique_ptr handles cleanup automatically)
        m_runtime.reset();
    }
    
    void TEATestFixture::run_single_iteration() {
        // Run the runtime (TestHead will immediately return 'q' to quit)
        m_last_return_code = m_runtime->run(0, nullptr);
    }
    
    int TEATestFixture::get_last_return_code() const {
        return m_last_return_code;
    }
    
    first::CratchitRuntime* TEATestFixture::get_runtime() {
        return m_runtime.get();
    }
    
}