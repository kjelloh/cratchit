#pragma once
#include <string>
#include <algorithm> // std::copy_if,

namespace functional {
  namespace text {
    // TODO: Refactor away?
    std::string filtered(std::string const& s,auto filter) {
      std::string result{};;
      std::copy_if(s.begin(),s.end(),std::back_inserter(result),filter);
      return result;
    }
  }
}
