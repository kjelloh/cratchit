#include "custom_raylib_log_callback.hpp"
#include "log.hpp"

void custom_raylib_log_callback(int msgType, const char *text, va_list args) {
  switch (msgType) {
    case LOG_INFO:
      log_development_trace("[raylib INFO] : {}",text);
      break;
    case LOG_ERROR: 
      log_development_trace("[raylib ERROR] : {}",text);
      break;
    case LOG_WARNING: 
      log_development_trace("[raylib WARN] : {}",text);
      break;
    case LOG_DEBUG: 
      log_development_trace("[raylib DEBUG] : {}",text);
      break;
    default: break;
  }
}
