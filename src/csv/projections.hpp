#pragma once

#include "csv.hpp"
#include "fiscal/amount/AccountStatement.hpp"
#include <functional> // std::function

namespace CSV {
  namespace project {

    namespace deprecated {

      enum class HeadingId {
        Undefined
        ,NORDEA
        ,SKV
        ,unknown
      };

      // 'Older' CSV -> Table -> Account Statement mechanism
      //         Based on a kind-of 'heading' type detection...
      // TODO: Refactor away in faviour of 'newer' monadic composition
      HeadingId to_csv_heading_id(CSV::FieldRow const& field_row);
      using ToHeadingProjection = std::function<CSV::OptionalTableHeading(CSV::FieldRow const& field_row)>;
      ToHeadingProjection make_heading_projection(HeadingId const& heading_id);

      // 'Older' ExpectedAccountStatement returning mechanism.
      // DEPRECATED: Use csv::monadic::import_table_to_account_statement() instead
      // TODO: Remove after all call sites have been refactored / 20251124
      [[deprecated("Use csv::monadic::import_table_to_account_statement() instead")]]
      ExpectedAccountStatement to_account_statement(CSV::project::deprecated::HeadingId const& csv_heading_id, CSV::OptionalTable const& maybe_csv_table);

    } // deprecated


  } // project
} // CSV