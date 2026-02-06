#include <string>
#include <format>

struct DomainPrefixedId {
  std::string m_prefix; // E.g., NORDEA,PG,BG,IBAN,SKV,
  std::string m_value; // E.g. a bank account, a PG account etc...
  std::string to_string() const {
    return std::format(
      "{}{}"
      ,((m_prefix.size()>0)?m_prefix + "::":std::string{}) // Prefixed -> <prefix>::
      ,m_value);
  }
};

namespace account {
  namespace statement {

    using AccountID = DomainPrefixedId;

    struct TableMeta {
      AccountID account_id;
    }; // TableMeta

  } // statement
} // acount