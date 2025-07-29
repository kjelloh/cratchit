#pragma once

#include <spdlog/spdlog.h>
#include <memory> // std::unique_ptr,
#include <sstream> // std::ostringstream,
#include <format>

namespace logger {

  class proxy {
  public:
      proxy(std::string const& caption) : m_caption{caption} {}

      /*
       * proxy("String with {}...",args to format) 
       */
      template <typename... Args>
      proxy& operator()(std::format_string<Args...> fmt, Args&&... args) {
          auto msg = std::format(fmt, std::forward<Args>(args)...);
          auto log_string = std::format("{}: {}", m_caption, msg);
          spdlog::info("{}", log_string);
          return *this;
      }

      /*
       * proxy("single string") 
       */
      proxy& operator()(const char* msg) {
        auto formatted = std::format("{}: {}", m_caption, msg);
        spdlog::info("{}", formatted);
        return *this;
      }      

  private:
    std::string m_caption;
  };

  class std_out_proxy {
  public:
    std_out_proxy();
    virtual ~std_out_proxy();
    template <typename T>
    std_out_proxy& operator<<(T const& x) {
      oss << x;
      try_flush_to_spdlog();
      return *this;
    }
    std_out_proxy& operator<<(std::ostream& (*manip)(std::ostream&));
    std_out_proxy& flush();
  private:
    std::ostringstream oss{};
    void try_flush_to_spdlog();
  };

  inline std_out_proxy cout_proxy{};
  inline proxy business{"BUSINESS"};
  
} // log
