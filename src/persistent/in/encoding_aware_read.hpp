#include "raw_text_read.hpp"
#include "text/charset.hpp"
#include "text/encoding.hpp"

namespace persistent {
  namespace in {

    namespace ISO_8859_1  {

      class istream : public text::bom_istream {
      public:
        istream(std::istream& in);
        // getline: Transcodes input from ISO8859-1 encoding to Unicode and then applies F to decode it to target encoding (std::nullopt on failure)
        // Emtpy line is allowed (returns nullopt only on failure to read)
        template <class F>
        std::optional<typename F::value_type> getline(F const& f) {
          typename std::optional<typename F::value_type> result{};
          std::string raw_entry{};
          if (std::getline(raw_in,raw_entry)) {
            auto unicode_s = charset::ISO_8859_1::iso8859ToUnicode(raw_entry);
            result = f(unicode_s);
          }
          // if (result.size() > 0) return result;
          // else return std::nullopt;
          return result;
        }
      };
    
    } // ISO_8859_1

    namespace UTF8 {

      class istream : public text::bom_istream {
      public:
        istream(std::istream& in);
        // getline: Transcodes input from UTF8 encoding to Unicode and then applies F to decode it to target encoding (std::nullopt on failure)
        // Emtpy line is allowed (returns nullopt only on failure to read)
        template <class F>
        std::optional<typename F::value_type> getline(F const& f) {
          typename std::optional<typename F::value_type> result{};
          std::string raw_entry{};
          if (std::getline(raw_in,raw_entry)) {
            auto unicode_s = ::text::encoding::UTF8::utf8ToUnicode(raw_entry);
            result = f(unicode_s);
          }
          return result;
        }
      };

    } // UTF8

    namespace CP437 {
      class istream : public text::bom_istream {
      public:
        istream(std::istream& in);
        // getline: Transcodes input from CP437 encoding to Unicode and then applies F to decode it to target encoding (std::nullopt on failure)
        // Emtpy line is allowed (returns nullopt only on failure to read)
        template <class F>
        std::optional<typename F::value_type> getline(F const& f) {
          typename std::optional<typename F::value_type> result{};
          std::string raw_entry{};
          if (std::getline(raw_in,raw_entry)) {
            auto unicode_s = charset::CP437::cp437ToUnicode(raw_entry);
            result = f(unicode_s);
          }
          // if (result.size() > 0) return result;
          // else return std::nullopt;
          return result;
        }
      };    

    } // CP437

  } // in
} // persistent
