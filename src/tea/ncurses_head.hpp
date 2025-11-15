#pragma once

#include "head.hpp"
#include "RuntimeEncoding.hpp"
#include <memory>
#include <ncurses.h>
#include <string>

namespace TEA {

    // Encoding compatibility status for NCurses
    enum class EncodingSupport {
        FULL_SUPPORT,   // Encoding fully supported
        ASCII_FALLBACK, // Fell back to ASCII-only mode
        FAILED          // Complete encoding failure
    };

    // NCurses-based UI implementation with encoding awareness
    class NCursesHead : public Head {
    public:
        NCursesHead(RuntimeEncoding const& runtime_endoding) : Head{runtime_endoding} {}
        ~NCursesHead() override;
        
        void initialize() override;
        void render(const pugi::xml_document& doc) override;
        int get_input() override;
        void cleanup() override;
        
    private:
        bool m_initialized = false;
        EncodingSupport m_encoding_support = EncodingSupport::FAILED;
        std::string m_encoding_error_message;
        
        // Encoding initialization
        bool setup_encoding_support();
        void render_encoding_error_overlay();

        text::encoding::UTF8::ToUnicodeBuffer m_utf8_to_unicode_buffer{};
    };

} // namespace TEA