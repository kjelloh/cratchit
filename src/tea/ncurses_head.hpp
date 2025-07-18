#pragma once

#include "head.hpp"
#include <memory>
#include <ncurses.h>

namespace TEA {

    // NCurses-based UI implementation
    class NCursesHead : public Head {
    public:
        NCursesHead() = default;
        ~NCursesHead() override;
        
        void initialize() override;
        void render(const pugi::xml_document& doc) override;
        int get_input() override;
        void cleanup() override;
        
    private:
        bool m_initialized = false;
    };

} // namespace TEA