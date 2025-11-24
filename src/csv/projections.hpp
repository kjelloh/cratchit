#pragma once

#include "csv.hpp"
#include "fiscal/amount/AccountStatement.hpp"
#include <functional> // std::function

namespace CSV {
  namespace project {
    enum class HeadingId {
      Undefined
      ,NORDEA
      ,SKV
      ,unknown
    };

    // 'Older' CSV -> Table -> Account Statement mechanism
    //         Based on a kind-of 'heading' type detection...
    // TODO: Refactor away in faviour of 
    HeadingId to_csv_heading_id(CSV::FieldRow const& field_row);
    using ToHeadingProjection = std::function<CSV::OptionalTableHeading(CSV::FieldRow const& field_row)>;
    ToHeadingProjection make_heading_projection(HeadingId const& heading_id);

    // 'Older' ExpectedAccountStatement returning mechanism.
    // DEPRECATED: Use cratchit::csv::import_table_to_account_statement() instead
    // TODO: Remove after all call sites have been refactored / 20251124
    [[deprecated("Use cratchit::csv::import_table_to_account_statement() instead")]]
    ExpectedAccountStatement to_account_statement(CSV::project::HeadingId const& csv_heading_id, CSV::OptionalTable const& maybe_csv_table);

    // Now in TaggedAmountFramework (upstream projections)
    // using ToTaggedAmountProjection = std::function<OptionalTaggedAmount(CSV::FieldRow const& field_row)>;
    // ToTaggedAmountProjection make_tagged_amount_projection(
    //    HeadingId const& csv_heading_id
    //   ,CSV::TableHeading const& table_heading);

  } // project
} // CSV