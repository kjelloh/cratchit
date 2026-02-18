#include "statement_table_meta.hpp"
#include "text/functional.hpp" // text::functional::count_alpha
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

    // BEGIN FieldType

    std::string to_string(FieldType field_type) {
      std::string result{"?FieldType?"};
      switch (field_type) {
        case FieldType::Unknown: result = "Unknown"; break;
        case FieldType::Empty: result = "Empty"; break;
        case FieldType::Date: result = "Date"; break;
        case FieldType::Amount: result = "Amount"; break;
        case FieldType::OrgNo: result = "OrgNo"; break;
        case FieldType::Text: result = "Text"; break;
        case FieldType::Undefined: result = "Undefined"; break;
      }
      return result;
    }

    FieldType to_field_type(std::string const& s) {
      if (s.size()==0) {
        return FieldType::Empty;
      }
      else if (auto maybe_date = to_date(s)) {
        return FieldType::Date;
      }
      else if (auto maybe_amount = to_amount(s)) {
        return FieldType::Amount;
      }
      else if (auto maybe_org_no = SKV::to_org_no(s)) {
        return FieldType::OrgNo;
      }
      else if (text::functional::count_alpha(s) > 0) {
        return FieldType::Text;
      }
      return FieldType::Unknown;
    }

    // END FieldType

    std::string to_string(RowMap const& row_map) {
      std::string result{};

      for (auto const& entry : row_map.ixs) {
        result += std::format(" {}",to_string(entry.first));
        result += ":";
        for (auto ix : entry.second) {
          result += std::format(" {}",ix);
        }
      }
      return result;
    }

    // BEGIN FoundSaldo

    FoundSaldo::FoundSaldo(std::ptrdiff_t rix,Date date,Amount ta)
      : m_value(rix,TaggedAmount(date,to_cents_amount(ta))) {}

    // END FoundSaldo

  } // statement
} // acount