#pragma once

#include <spdlog/spdlog.h>
#include <memory> // std::unique_ptr,
#include <sstream> // std::ostringstream,

namespace logger {
  class std_out_proxy {
  public:
    template <typename T>
    std_out_proxy& operator<<(T const& x) {
      oss << x;
      try_flush_to_spdlog();
      return *this;
    }
    std_out_proxy& operator<<(std::ostream& (*manip)(std::ostream&));
  private:
    std::ostringstream oss{};
    void try_flush_to_spdlog();
  };

  inline std_out_proxy cout_proxy{};   
  
} // log
