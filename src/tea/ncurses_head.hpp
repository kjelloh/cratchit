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
        NCursesHead() = default;
        ~NCursesHead() override;
        
        void initialize(RuntimeEncoding::DetectedEncoding target_encoding) override;
        void render(const pugi::xml_document& doc) override;
        int get_input() override;
        void cleanup() override;
        
    private:
        bool m_initialized = false;
        EncodingSupport m_encoding_support = EncodingSupport::FAILED;
        RuntimeEncoding::DetectedEncoding m_target_encoding = RuntimeEncoding::DetectedEncoding::UTF8;
        std::string m_encoding_error_message;
        
        // Encoding initialization
        bool setup_encoding_support();
        void render_encoding_error_overlay();

        encoding::UTF8::ToUnicodeBuffer m_utf8_to_unicode_buffer{};
    };

} // namespace TEA