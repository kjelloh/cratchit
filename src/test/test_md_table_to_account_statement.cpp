#include "test_atomics.hpp"
#include "test_fixtures.hpp"
#include "logger/log.hpp"
#include "fiscal/amount/AccountStatement.hpp"
#include "domain/csv_to_account_statement.hpp"
#include "csv/csv_to_statement_id_ed.hpp"
#include "csv/parse_csv.hpp"
#include "test/data/account_statements_csv.hpp"
#include <gtest/gtest.h>
#include <optional>
#include <string>

