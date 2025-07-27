#pragma once

#include <pugixml.hpp>
#include "RuntimeEncoding.hpp"

namespace TEA {

    // Abstract interface for UI rendering and input handling
    class Head {
    public:
      Head(RuntimeEncoding const& runtime_endoding = RuntimeEncoding{})
        : m_runtime_endoding{runtime_endoding} {}
      virtual ~Head() = default;
      
      // Initialize the UI system with encoding information
      virtual void initialize() = 0;
      
      // Render the UI document
      virtual void render(const pugi::xml_document& doc) = 0;
      
      // Get user input (blocking call)
      virtual int get_input() = 0;
      
      // Cleanup the UI system
      virtual void cleanup() = 0;
    protected:
      RuntimeEncoding m_runtime_endoding;
    };

} // namespace TEA