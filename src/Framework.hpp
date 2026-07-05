#pragma once

#include "KeyValueMap.hpp"
#include <filesystem>

class Framework {
public:
  Framework(std::filesystem::path persistent_file_path);
  bool is_valid(); 
private:
  std::filesystem::path m_persistent_file_path;
  bool digest_persistent_file();
  std::optional<KeyValueMap> m_maybe_map;
};