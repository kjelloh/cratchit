#include "test_head.hpp"

namespace TEA {

    void TestHead::initialize() {}

    void TestHead::render(const pugi::xml_document& doc) {}

    int TestHead::get_input() {
        // Always return 'q' to quit immediately in test environment
        return 'q';
    }

    void TestHead::cleanup() {}

} // namespace TEA