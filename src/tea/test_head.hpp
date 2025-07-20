#pragma once

#include "head.hpp"

namespace TEA {

    // Test-friendly Head implementation - minimal no-op implementation
    class TestHead : public Head {
    public:
        TestHead() = default;
        ~TestHead() override = default;
        
        void initialize() override;
        void render(const pugi::xml_document& doc) override;
        int get_input() override;
        void cleanup() override;
    };

} // namespace TEA