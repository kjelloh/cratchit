#pragma once

#include "csv.hpp"
#include <functional> // std::function

namespace CSV {
  namespace project {
    enum class HeadingId {
      Undefined
      ,NORDEA
      ,SKV
      ,unknown
    };

    HeadingId to_csv_heading_id(CSV::FieldRow const& field_row);

    using ToHeadingProjection = std::function<CSV::OptionalTableHeading(CSV::FieldRow const& field_row)>;

    ToHeadingProjection make_heading_projection(HeadingId const& heading_id);


  } // project
} // CSV