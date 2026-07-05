#pragma once

#include <string>
#include <map>

class KeyValueMap {
public:
  void insert(std::string const& key,std::string const& value);
  std::size_t size();
private:
  std::map<std::string,std::string> m_map{};
}; // KeyValueMap