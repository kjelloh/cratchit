#include "KeyValueMap.hpp"

// public:

void KeyValueMap::insert(std::string const& key,std::string const& value) {
  this->m_map[key] = value;
}

std::size_t KeyValueMap::size() {return this->m_map.size();}
