#include "RuntimeEncoding.hpp"
#include "text/encoding.hpp"

// RuntimeEncoding Implementation
RuntimeEncoding::DetectedEncoding RuntimeEncoding::m_detected_encoding = DetectedEncoding::UTF8;

RuntimeEncoding::DetectedEncoding RuntimeEncoding::detected_encoding() {
  return m_detected_encoding;
}

// void RuntimeEncoding::set_assumed_terminal_encoding(DetectedEncoding encoding) {
//   m_detected_encoding = encoding;
// }

std::string RuntimeEncoding::get_encoding_display_name() {
  return encoding::icu::EncodingDetector::enum_to_display_name(m_detected_encoding);
}

std::string RuntimeEncoding::get_encoding_canonical_name() {
  switch (m_detected_encoding) {
    case DetectedEncoding::UTF8: return "UTF-8";
    case DetectedEncoding::UTF16BE: return "UTF-16BE";
    case DetectedEncoding::UTF16LE: return "UTF-16LE";
    case DetectedEncoding::UTF32BE: return "UTF-32BE";
    case DetectedEncoding::UTF32LE: return "UTF-32LE";
    case DetectedEncoding::ISO_8859_1: return "ISO-8859-1";
    case DetectedEncoding::ISO_8859_15: return "ISO-8859-15";
    case DetectedEncoding::WINDOWS_1252: return "windows-1252";
    case DetectedEncoding::CP437: return "IBM437";
    case DetectedEncoding::Unknown: return "Unknown";
  }
  return "UTF-8";
}

// bool RuntimeEncoding::is_utf8_mode() {
//   return m_detected_encoding == DetectedEncoding::UTF8;
// }

// bool RuntimeEncoding::supports_unicode() {
//   switch (m_detected_encoding) {
//     case DetectedEncoding::UTF8:
//     case DetectedEncoding::UTF16BE:
//     case DetectedEncoding::UTF16LE:
//     case DetectedEncoding::UTF32BE:
//     case DetectedEncoding::UTF32LE:
//       return true;
//     default:
//       return false;
//   }
// }

// void RuntimeEncoding::initialize_defaults() {
//   // Default assumption: UTF-8 terminal
//   m_detected_encoding = DetectedEncoding::UTF8;
// }
