#include "test_head.hpp"

namespace TEA {

    void TestHead::initialize(RuntimeEncoding::DetectedEncoding target_encoding) {
        // No-op for test environment - ignore encoding parameter
        (void)target_encoding; // Suppress unused parameter warning
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