#include "statement_table_meta.hpp"
#include "logger/log.hpp"

namespace SKV {
  std::optional<OrgNo> to_org_no(std::string_view sv) {
    auto len = sv.size();
    int digits_count{};
    int hyphen_count{};
    for (size_t i=0;i<sv.size();++i) {
      auto ch = sv[i];
      if (std::isdigit(ch)) ++digits_count;
      if (ch == '-') {
        if (i!=len-5) return {};
        ++hyphen_count;
      }
    }

    if (true) logger::development_trace(
      "sv:{}:{} digits_count:{} hyphen_count:{}"
      ,sv
      ,len
      ,digits_count
      ,hyphen_count);

    if (hyphen_count>1) return {};
    if (digits_count + hyphen_count != len) return {};
    if (!(digits_count == 10 or digits_count == 12)) return {};

    if (true) logger::development_trace("org-no, candidate has acceptable counts");

    return OrgNo{
        .value = std::string(sv.substr(0,len-4-hyphen_count))
      ,.control = std::string(sv.substr(len-4,4))
    };
  }; // to_org_no

} // SKV

namespace account {
  namespace statement {

    std::string to_string(FieldType field_type) {
      std::string result{"?FieldType?"};
      switch (field_type) {
        case FieldType::Unknown: result = "Unknown"; break;
        case FieldType::Empty: result = "Empty"; break;
        case FieldType::Date: result = "Date"; break;
        case FieldType::Amount: result = "Amount"; break;
        case FieldType::Text: result = "Text"; break;
        case FieldType::Undefined: result = "Undefined"; break;
        default: result = std::format("?FieldType:{}?",static_cast<int>(field_type)); break;
      }
      return result;
    }


    FoundSaldo::FoundSaldo(std::ptrdiff_t rix,Date date,Amount ta)
      : m_value(rix,TaggedAmount(date,to_cents_amount(ta))) {}

  } // statement
} // acount