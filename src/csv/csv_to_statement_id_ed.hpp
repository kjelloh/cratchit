#pragma once

#include "csv/csv.hpp"                          // CSV::Table
#include "functional/maybe.hpp"                 // AnnotatedMaybe<T>...
#include "fiscal/amount/AccountStatement.hpp"   // AccountID, DomainPrefixedId
#include "text/functional.hpp"                  // contains_any_keyword,...
#include "logger/log.hpp"                       // logger::...
#include <optional>
#include <string>
#include <string_view>
#include <algorithm>
#include <ranges>
#include <regex>
#include <set>
#include <cctype> // std::isalnul,...

namespace account {
  namespace statement {

    namespace maybe {

      std::optional<CSV::MDTable<maybe::table::TableMeta>> to_statement_id_ed_step(CSV::Table const& table);

    } // maybe

    namespace monadic {
      AnnotatedMaybe<CSV::MDTable<maybe::table::TableMeta>> to_statement_id_ed_step(CSV::Table const& table);

    } // monadic

  } // statement
} // account
