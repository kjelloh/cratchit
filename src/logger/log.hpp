#pragma once

#include <spdlog/spdlog.h>
#include <memory> // std::unique_ptr,
#include <sstream> // std::ostringstream,

namespace logger {
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
  
} // log
