#pragma once 

#include "fiscal/amount/TaggedAmount.hpp"
#include <string>
#include <format>
#include <map>

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

// TODO: Move to shared TU if/when appropriate
//       For now introduced for parsing SKV account statement csv tables
namespace SKV {
  struct OrgNo {
    // 10 digits 'personnummer' or 'orgnaisationsnummer'
    // 10 or 12 digits 'personnummer' YYYYMMDD-XXXX or YYYYMMDD-XXXX
    // With or without '-' to separate last four digits
    // Ex: YYMMDD-XXXX
    // Ex: YYYYMMDD-XXXX
    // Ex: NNNNNN-XXXX
    // Ex: YYMMDDXXXX
    // Ex: YYYYMMDDXXXX
    // Ex: NNNNNNXXXX
    std::string value;   // 10 or 12 'value' digits
    std::string control; // four last digits
    std::string with_hyphen() const {
      return std::format("{}-{}",value,control);
    }
    std::string without_hyphen() const {
      return std::format("{}{}",value,control);
    }
  }; // OrgNo

  std::optional<OrgNo> to_org_no(std::string_view sv);
} // SKV

namespace account {
  namespace statement {

    struct FoundSaldo {
      FoundSaldo(std::ptrdiff_t rix,Date date,Amount ta);
      using Value = std::pair<std::ptrdiff_t,TaggedAmount>;
      Value m_value;
    }; // FoundSaldo

    struct FoundSaldos {
      FoundSaldo m_in_saldo;
      FoundSaldo m_out_saldo;
    }; // FoundSaldos

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

    enum class FieldType {
        Unknown
      ,Empty
      ,Date
      ,Amount
      ,Text 
      ,Undefined
    };

    using FieldIx = unsigned;

    struct RowMap {
      std::map<FieldType,std::vector<FieldIx>> ixs;
      bool operator==(RowMap const&) const = default;      
    }; // RowMap

    enum class EntryAmountsType {
       Undefined
      ,TransOnly
      ,TransThenSaldo
      ,Unknown
    };

    struct StatementMapping {
      RowMap m_row_0_map;
      bool has_heading;
      std::optional<FoundSaldos> m_maybe_in_out_saldos;
      EntryAmountsType entry_amounts_type;
      RowMap common;
    }; // StatementMapping

    struct TableMeta {
      StatementMapping statement_mapping;
      ColumnMapping column_mapping;
      AccountID account_id;
    }; // TableMeta

  } // statement
} // acount