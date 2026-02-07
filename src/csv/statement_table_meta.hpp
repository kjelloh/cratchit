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

    struct ColumnMapping {
      int date_column = -1;
      int transaction_amount_column = -1;
      int saldo_amount_column = -1;
      int description_column = -1;
      std::vector<int> additional_description_columns;

      bool is_valid() const {
        return date_column >= 0 && transaction_amount_column >= 0 && description_column >= 0;
      }
    }; // ColumnMapping

    struct StatementMapping {
    }; // StatementMapping

    struct TableMeta {
      StatementMapping statement_mapping;
      ColumnMapping column_mapping;
      AccountID account_id;
    }; // TableMeta

  } // statement
} // acount