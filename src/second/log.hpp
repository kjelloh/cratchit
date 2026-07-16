#pragma once
#include <string_view>
#include <format>

void log_business(std::string_view sv);
void log_development_trace(std::string_view sv);
void log_design_insufficiency(std::string_view sv);

template <typename... Args>
void log_business(std::format_string<Args...> fmt, Args&&... args) {
  log_business(std::format(fmt,std::forward<Args>(args)...));
}

template <typename... Args>
void log_development_trace(std::format_string<Args...> fmt, Args&&... args) {
  log_development_trace(std::format(fmt,std::forward<Args>(args)...));
}

template <typename... Args>
void log_design_insufficiency(std::format_string<Args...> fmt, Args&&... args) {
  log_design_insufficiency(std::format(fmt,std::forward<Args>(args)...));
}
