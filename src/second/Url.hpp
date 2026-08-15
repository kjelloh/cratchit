#pragma once

#include <string>
#include <algorithm>

class Url {
public:
  Url(std::string);
  std::string string() const;
private:
  std::string m_s;
}; // Url

bool is_local_file_url(Url const& url);
