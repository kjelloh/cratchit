#pragma once

#include <spdlog/spdlog.h>
#include <memory> // std::unique_ptr,
#include <sstream> // std::ostringstream,
#include <format>

namespace logger {

  class proxy {
  public:
    proxy(std::string const& caption) 
      :  m_caption{caption}
        ,m_indent_string{} {}

    /*
      * proxy("String with {}...",args to format) 
      */
    template <typename... Args>
    proxy& operator()(std::format_string<Args...> fmt, Args&&... args) {
        auto msg = std::format(fmt, std::forward<Args>(args)...);
        auto log_string = std::format("{}: {}", m_caption, msg);
        spdlog::info("{}{}",m_indent_string,log_string);
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

    proxy& indent(std::size_t step = 2) {
      m_indent_string.append(step, ' ');
      return *this;
    }

    proxy& unindent(std::size_t step = 2) {
        if (m_indent_string.size() >= step)
            m_indent_string.resize(m_indent_string.size() - step);
        else
            m_indent_string.clear();
        return *this;
    }
  private:
    std::string m_caption;
    std::string m_indent_string;
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
  inline proxy business{"BUSINESS"}; // Business log -> application user
  inline proxy design_insufficiency{"DESIGN_INSUFFICIENCY"}; // Design insufficiency -> developer
  inline proxy development_trace{"TRACE"}; // development trace -> developer

  class scope_logger {
  public:
    scope_logger(proxy& p,std::string const& caption)
      :  m_proxy{p}
        ,m_caption(caption) {
      m_proxy("BEGIN {}",m_caption).indent(2);
    }
    ~scope_logger() {
      m_proxy.unindent(2)("END {}",m_caption);
    }
  private:
    proxy& m_proxy;
    std::string m_caption;
  }; 
  
} // logger

static char const* NL = "\n";
static char const* NLT = "\n\t";