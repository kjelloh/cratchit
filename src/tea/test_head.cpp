#include "test_head.hpp"

namespace TEA {

    void TestHead::initialize() {
        // No-op for test environment
    }

    void TestHead::render(const pugi::xml_document& doc) {
        // No-op for test environment - don't render anything
    }

    int TestHead::get_input() {
        // Always return 'q' to quit immediately in test environment
        return 'q';
    }

    void TestHead::cleanup() {
        // No-op for test environment
    }

} // namespace TEA