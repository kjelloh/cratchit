#include "AccountStatement.hpp"
#include "../../logger/log.hpp"
#include <sstream>

AccountStatement:: AccountStatement(
   AccountStatementEntries const& entries
  ,OptionalAccountDescriptor account_descriptor)
    :  m_entries{entries}
      ,m_account_descriptor{account_descriptor} {}

namespace CSV {
  namespace project {
    ExpectedAccountStatement to_account_statement(CSV::project::HeadingId const& csv_heading_id, CSV::OptionalTable const& maybe_csv_table) {
      logger::development_trace("to_account_statement: csv_heading_id:{}",static_cast<unsigned int>(csv_heading_id));
      if (maybe_csv_table) {
        logger::development_trace("to_account_statement: table heading:{} rows.size():{}",maybe_csv_table->heading,maybe_csv_table->rows.size());
        auto to_tagged_amount = make_tagged_amount_projection(csv_heading_id,maybe_csv_table->heading);
        AccountStatementEntries entries{};
        for (auto const& field_row : maybe_csv_table->rows) {
          if (auto o_ta = to_tagged_amount(field_row)) {
            logger::development_trace("to_account_statement: to_tagged_amount({}):{}",to_string(field_row),to_string(o_ta.value()));
            Delta<TaggedAmount> saldo_delta{o_ta.value()};
            entries.push_back(saldo_delta);
            // Is there a 'Saldo' field in the tagged amount?
            if (auto maybe_saldo = o_ta->tag_value("Saldo")) {
              TaggedAmount saldo_ta(*o_ta);
              State<TaggedAmount> saldo_state{saldo_ta};
              entries.push_back(AccountStatementEntry{saldo_state});
            }
          }
          else {
            logger::cout_proxy << "to_account_statement: Sorry, Failed to create tagged amount from field_row " << std::quoted(to_string(field_row));
          }
        }            
        logger::development_trace("to_account_statement: returned {} entries",entries.size());
        return AccountStatement{entries};
      }
      else {
        logger::development_trace("to_account_statement: NULL maybe_csv_table");
      }
      return std::unexpected("Unsupported (not viable?) account statement file");
    }
  }
}
