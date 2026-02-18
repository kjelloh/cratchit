#include "csv_to_account_statement.hpp"
#include "fiscal/amount/TaggedAmountFramework.hpp"
#include <set>

namespace account {

  namespace statement {

    namespace maybe {

      namespace table {

        RowMap to_row_map(CSV::Table::Row const& row) {
          RowMap result{};

          for (unsigned i=0;i<row.size();++i) {
            result.ixs[to_field_type(row[i])].push_back(i);
          }

          return result;
        }

        RowsMap to_rows_map(CSV::Table::Rows const& rows) {
          RowsMap result{};

          for (auto const& row : rows) {
            result.push_back(to_row_map(row));
          }

          return result;
        }

        void log_the_rows_map(CSV::Table::Rows const& rows,RowsMap const& rows_map) {
          for (size_t i=0;i<rows.size();++i) {
            logger::development_trace(
              "\n\trow:{} \n\tmap:{}"
              ,rows[i].to_string()
              ,to_string(rows_map[i]));
          } // for rows

        } // log_the_rows_map

        std::optional<StatementMapping> generic_like_to_statement_mapping(CSV::Table const& table) {

          logger::scope_logger log_raii(logger::development_trace,"generic_like_to_statement_mapping",logger::LogToConsole::ON);

          StatementMapping candidate{};

          auto const& rows = table.rows;
          auto rows_map = to_rows_map(rows);
          if (true) log_the_rows_map(table.rows,rows_map);

          if (true) logger::development_trace("rows_map size {}",rows_map.size());

          if (rows_map.size() == 0) return {}; // Empty table

          candidate.m_row_0_map = rows_map[0];

          // Has heading?
          if (rows_map[0].ixs.contains(FieldType::Text)) {
            auto text_count = rows_map[0].ixs.at(FieldType::Text).size(); 
            auto empty_count = rows_map[0].ixs.contains(FieldType::Empty)?rows_map[0].ixs.at(FieldType::Empty).size():0;
            if (true) logger::development_trace(
               "text_count:{} empty_count:{} rows[0].size():{}"
              ,text_count
              ,empty_count
              ,table.rows[0].size());

            candidate.has_heading = (text_count+empty_count == table.rows[0].size());
          }
          if (true) logger::development_trace("candidate.has_heading {}",candidate.has_heading);

          auto to_in_out_saldos = [](CSV::Table const& table, RowsMap rows_map) -> std::optional<FoundSaldos> {

            auto const& [heading,rows] = table;

            {
              // SKV statement table?
              auto is_saldo_candidate = [](RowMap const& row_map){
                // Empty: 0 2 Amount: 3 Text: 1
                static auto const SKV_SALDO_ENTRY_MAP_1 = RowMap{
                  .ixs = {
                    {FieldType::Empty,{0,2}} 
                    ,{FieldType::Amount,{3}} 
                    ,{FieldType::Text,{1}}}
                };

                // Empty: 0 4 Amount: 2 3 Text: 1
                static auto const SKV_SALDO_ENTRY_MAP_0 = RowMap{
                  .ixs = {
                    {FieldType::Empty,{0,4}} 
                    ,{FieldType::Amount,{2,3}} 
                    ,{FieldType::Text,{1}}}
                };
                return (
                     (row_map == SKV_SALDO_ENTRY_MAP_0)
                  or (row_map == SKV_SALDO_ENTRY_MAP_1));
              };

              auto in_saldo_candidate_iter = std::ranges::find_if(
                  rows_map
                ,is_saldo_candidate
              );

              auto out_saldo_candidate_iter = in_saldo_candidate_iter;
              std::ptrdiff_t in_out_saldo_enveloped_size{};
              if (in_saldo_candidate_iter != rows_map.end()) {
                out_saldo_candidate_iter = std::find_if(
                    in_saldo_candidate_iter+1
                  ,rows_map.end()
                  ,is_saldo_candidate
                );
                if (out_saldo_candidate_iter != rows_map.end()) {
                  in_out_saldo_enveloped_size = std::distance(in_saldo_candidate_iter,out_saldo_candidate_iter);
                }
              }
              if (true) logger::development_trace("in_out_saldo_enveloped_size:{}",in_out_saldo_enveloped_size);

              if (in_out_saldo_enveloped_size==0) return {}; // to_in_out_saldos

              auto in_saldo_rix = std::distance(rows_map.begin(),in_saldo_candidate_iter);
              auto out_saldo_rix = std::distance(rows_map.begin(),out_saldo_candidate_iter);

              auto to_skv_saldo_date = [](std::string const& skv_saldo_text) -> std::optional<Date> {
                auto tokens = tokenize::splits(skv_saldo_text,' ');
                if (true) logger::development_trace("tokens:{}",tokens);
                if (tokens.size()==0) return {};
                return to_date(tokens.back());
              }; // to_skv_saldo_date

              auto maybe_in_saldo_date = to_skv_saldo_date(rows[in_saldo_rix][1]);
              auto maybe_out_saldo_date = to_skv_saldo_date(rows[out_saldo_rix][1]);

              if (!maybe_in_saldo_date or !maybe_out_saldo_date) return {};

              auto amount_cix = rows_map[in_saldo_rix].ixs.at(FieldType::Amount).front();
              auto maybe_in_saldo_amount = to_amount(rows[in_saldo_rix][amount_cix]);
              auto maybe_out_saldo_amount = to_amount(rows[out_saldo_rix][amount_cix]);

              if (!maybe_in_saldo_amount or !maybe_out_saldo_amount) return {}; // to_skv_saldo_date

              auto found_in_saldo = FoundSaldo{
                  in_saldo_rix
                ,*maybe_in_saldo_date
                ,*maybe_in_saldo_amount
              };
              auto found_out_saldo = FoundSaldo{
                  out_saldo_rix
                ,*maybe_out_saldo_date
                ,*maybe_out_saldo_amount
              };
              return FoundSaldos{found_in_saldo,found_out_saldo};
            }

            return {};
            
          }; // to_in_out_saldos

          auto maybe_in_out_saldos = to_in_out_saldos(table,rows_map);
          candidate.m_maybe_in_out_saldos = maybe_in_out_saldos;
          if (maybe_in_out_saldos) {
            if (true) logger::development_trace(
              "maybe_in_out_saldos: IN:[{},{}] OUT[{},{}]"
              ,maybe_in_out_saldos->m_in_saldo.m_value.first
              ,::to_string(maybe_in_out_saldos->m_in_saldo.m_value.second)
              ,maybe_in_out_saldos->m_out_saldo.m_value.first
              ,::to_string(maybe_in_out_saldos->m_out_saldo.m_value.second)
              );
          }
          
          auto is_trans_entry_candidate = [](auto const& row_map) {
            if (!row_map.ixs.contains(FieldType::Date) or row_map.ixs.at(FieldType::Date).size()!=1) return false;
            if (     !row_map.ixs.contains(FieldType::Amount) 
                  or (row_map.ixs.at(FieldType::Amount).size()==0)
                  or (row_map.ixs.at(FieldType::Amount).size()>2)) return false;
            if (!row_map.ixs.contains(FieldType::Text) or row_map.ixs.at(FieldType::Text).size()==0) return false;
            return true;
          };

          auto first_trans_iter_candidate = std::ranges::find_if(
             rows_map
            ,is_trans_entry_candidate);

          if (first_trans_iter_candidate == rows_map.end()) {
            logger::development_trace("No is_trans_entry_candidate match");
            if (maybe_in_out_saldos) {
              logger::development_trace("Has in/out saldos - returns candidate");
              return candidate;
            }
            return {};
          }

          auto trans_iter_candidates_end = std::find_if_not(
             first_trans_iter_candidate
            ,rows_map.end()
            ,is_trans_entry_candidate
          );

          auto trans_candidates_count = std::distance(first_trans_iter_candidate,trans_iter_candidates_end);
          if (true) logger::development_trace("trans_candidates_count:{}",trans_candidates_count);

          // Fold all candidate row maps using 'common' to get the common row map
          auto iter = first_trans_iter_candidate +1;
          auto iter_end = trans_iter_candidates_end;
          auto common = *first_trans_iter_candidate;
          while (iter != iter_end) {
            auto const& rhs = *iter;
            RowMap next{};
            for (auto const& [lhs_type,lhs_v] : common.ixs) {
              if (!rhs.ixs.contains(lhs_type)) continue;
              auto const& rhs_v = rhs.ixs.at(lhs_type);
              std::vector<FieldIx> intersection_v{};
              std::ranges::set_intersection(
                 lhs_v
                ,rhs_v
                ,std::back_inserter(intersection_v)
              );
              if (intersection_v.size()>0) next.ixs[lhs_type] = intersection_v;
            }
            common = next;
            ++iter;
          }

          if (true) logger::development_trace("common :{}",to_string(common));

          bool is_single_amount_trans_candidates = common.ixs.at(FieldType::Amount).size() == 1;
          if (true) logger::development_trace("is_single_amount_trans_candidates:{}",is_single_amount_trans_candidates);

          candidate.maybe_common = common;

          auto begin_rix = std::distance(rows_map.begin(),first_trans_iter_candidate);
          auto end_rix = std::distance(rows_map.begin(),trans_iter_candidates_end);

          if (is_single_amount_trans_candidates) {
            candidate.entry_amounts_type = EntryAmountsType::TransOnly;
            return candidate;
          } // if is_single_amount_trans_candidates

          if (trans_candidates_count == 1) {
            // Inferr (bet on) it being trans- and then saldo amount entry?
            if (candidate.has_heading) {
              auto const& lhs_amount_heading = rows[0][common.ixs.at(FieldType::Amount).front()];
              auto const& rhs_amount_heading = rows[0][common.ixs.at(FieldType::Amount).back()];
              if (true) logger::development_trace(
                "lhs_amount_heading:{} rhs_amount_heading:{}"
                ,lhs_amount_heading
                ,rhs_amount_heading);
              std::set<std::string> trans_heading_candidates{
                "Belopp"
              };
              std::set<std::string> saldo_heading_candidates{
                "Saldo"
              };
              if (trans_heading_candidates.contains(lhs_amount_heading) and saldo_heading_candidates.contains(rhs_amount_heading)) {
                candidate.entry_amounts_type = EntryAmountsType::TransThenSaldo;
                return candidate; // SUCCESS

              }
            }
            else {
              // No heading
            }
          }

          std::vector<std::pair<Amount,Amount>> amounts{};
          for (auto rix=begin_rix;rix<end_rix;++rix) {
            auto const& row = rows[rix];
            auto first_cix = common.ixs.at(FieldType::Amount).front();
            auto second_cix = common.ixs.at(FieldType::Amount).back();
            auto maybe_first_amount = to_amount(row[first_cix]);
            auto maybe_second_amount = to_amount(row[second_cix]);
            if (maybe_first_amount and maybe_second_amount) {
              amounts.push_back({*maybe_first_amount,*maybe_second_amount});
            }
            else {
              if (true) logger::design_insufficiency("Expected two valid amount after successfull rows mapping");
              return {};
            }
          }

          if (amounts.size()<2) {
            if (true) logger::design_insufficiency("Failed to determine saldo amount column for less than two entry candidates");
            return {};
          }

          // Detect trans- vs saldo-amount columns
          unsigned raising_date_first_trans_second_saldo_count{};
          unsigned falling_date_first_trans_second_saldo_count{};
          unsigned undetermined_amounts_count{};
          for (size_t pred_six=0;pred_six+1 < amounts.size();++pred_six) {
            auto succ_six = pred_six+1;
            auto [pred_first,pred_second] = amounts[pred_six];
            auto [succ_first,succ_second] = amounts[succ_six];

            logger::development_trace(
               "pred_first:{} pred_second:{} succ_first:{} succ_second:{}"
              ,::to_string(pred_first)
              ,::to_string(pred_second)
              ,::to_string(succ_first)
              ,::to_string(succ_second));
            
            logger::development_trace(
               "pred_second:{} + succ_first:{} = {} ==?== {} : succ_second"
               ,::to_string(pred_second)
               ,::to_string(succ_first)
               ,::to_string(pred_second+succ_first)
               ,::to_string(succ_second));

            if (pred_second+succ_first == succ_second) {
              ++raising_date_first_trans_second_saldo_count;
              logger::development_trace("raising_date_first_trans_second_saldo_count:{}",raising_date_first_trans_second_saldo_count);
            }
            else if (succ_second+pred_first == pred_second) {
              ++falling_date_first_trans_second_saldo_count;
              logger::development_trace("falling_date_first_trans_second_saldo_count:{}",falling_date_first_trans_second_saldo_count);
            }
            else {
              ++undetermined_amounts_count;
              logger::development_trace("undetermined_amounts_count:{}",undetermined_amounts_count);
            }

          }

          // Log
          if (true) logger::development_trace(
             "raising_date_first_trans_second_saldo_count:{} falling_date_first_trans_second_saldo_count:{}"
            ,raising_date_first_trans_second_saldo_count
            ,falling_date_first_trans_second_saldo_count);

          if (true) logger::development_trace(
             "undetermined_amounts_count:{}"
            ,undetermined_amounts_count);

          // Bail out on fails
          if (undetermined_amounts_count > 0) {
            if (true) logger::development_trace(
               "undetermined_amounts_count:{} should be 0 for amounts.size()-1:{}"
              ,undetermined_amounts_count
              ,amounts.size()-1);
            return {};
          }
          if (     (falling_date_first_trans_second_saldo_count > 0) 
               and (falling_date_first_trans_second_saldo_count != amounts.size()-1)) {
            if (true) logger::development_trace(
               "falling_date_first_trans_second_saldo_count:{} don't match amounts.size()-1:{}"
              ,falling_date_first_trans_second_saldo_count
              ,amounts.size()-1);
            return {};
          }
          if (     (raising_date_first_trans_second_saldo_count > 0) 
               and (raising_date_first_trans_second_saldo_count != amounts.size()-1)) {
            if (true) logger::development_trace(
               "raising_date_first_trans_second_saldo_count:{} don't match amounts.size()-1:{}"
              ,falling_date_first_trans_second_saldo_count
              ,amounts.size()-1);
            return {};
          }

          bool is_new_to_old_order = (falling_date_first_trans_second_saldo_count>0);
          if (true) logger::development_trace("is_new_to_old_order:{}",is_new_to_old_order);

          if (falling_date_first_trans_second_saldo_count>0 or raising_date_first_trans_second_saldo_count) {
            candidate.entry_amounts_type = EntryAmountsType::TransThenSaldo;
          }
          else {
            if (true) logger::development_trace("Failed to identify candidate.entry_amounts_type");
            return {};
          }
          
          if (true) logger::development_trace(
            "returns candidate");

          return candidate;

        } // generic_like_to_statement_mapping

        std::optional<ColumnMapping> generic_like_to_column_mapping(CSV::MDTable<StatementMapping> const& mapped_table) {
          logger::scope_logger log_raii(logger::development_trace,"generic_like_to_column_mapping",logger::LogToConsole::ON);
          ColumnMapping candidate{};

          auto const& [statement_mapping,table] = mapped_table;

          try {

            if (!statement_mapping.maybe_common) return {};

            candidate.date_column = statement_mapping.maybe_common->ixs.at(FieldType::Date).front();

            switch (statement_mapping.entry_amounts_type) {
              case EntryAmountsType::TransOnly: {
                candidate.transaction_amount_column = statement_mapping.maybe_common->ixs.at(FieldType::Amount).front();
              } break;
              case EntryAmountsType::TransThenSaldo: {
                candidate.transaction_amount_column = statement_mapping.maybe_common->ixs.at(FieldType::Amount).front();
                candidate.saldo_amount_column = statement_mapping.maybe_common->ixs.at(FieldType::Amount).back();
              } break;
              default: {
                logger::design_insufficiency(
                  "generic_like_to_column_mapping failed. Unknwon entry_amounts_type:{}"
                  ,static_cast<int>(statement_mapping.entry_amounts_type));
              }
            }

            if (statement_mapping.maybe_common->ixs.at(FieldType::Text).size()==1) {
                candidate.description_column = statement_mapping.maybe_common->ixs.at(FieldType::Text).front();
            }
            else if (statement_mapping.has_heading) {
              std::map<FieldIx,std::string> text_columns_map{};
              for (auto ix : statement_mapping.maybe_common->ixs.at(FieldType::Text)) {
                auto const& text = table.rows[0][ix];
                text_columns_map[ix] = text;
              }
              if (true) logger::development_trace("text_columns_map:{}",text_columns_map);

              const auto NORDEA_TEXT_MAP = std::map<FieldIx,std::string>{
                {4, "Namn"}
                ,{5, "Ytterligare detaljer"}
                ,{9, "Valuta"}};

              if (text_columns_map == NORDEA_TEXT_MAP) {
                candidate.description_column = 4;
                candidate.additional_description_columns.push_back(5);
              }
            }
          }
          catch (std::exception const& e) {
            logger::design_insufficiency(
               "generic_like_to_column_mapping failed. Exception: {}"
              ,e.what());
          }

          if (candidate.is_valid()) return candidate;

          return {};

        }

        std::optional<AccountID> generic_like_to_account_id(CSV::MDTable<StatementMapping> const& mapped_table) {
          logger::scope_logger log_raii(logger::development_trace,"generic_like_to_account_id",logger::LogToConsole::ON);

          AccountID candidate{};
          auto const& [statement_mapping,table] = mapped_table;

          {
            // map: Empty: 0 4 Amount: 2 3 Text: 1
            auto const SKV_ROW_0_MAP_0 = RowMap{
              .ixs = {
                {FieldType::Empty,{0,4}}
              ,{FieldType::Amount,{2,3}}
              ,{FieldType::Text,{1}}
            }};

            if (statement_mapping.m_row_0_map == SKV_ROW_0_MAP_0) {
              if (true) logger::development_trace(
                  "Matches SKV_ROW_0_MAP_0:{}"
                ,to_string(SKV_ROW_0_MAP_0));
                candidate = AccountID{"SKV","??"};
                return candidate; // SUCCESS
            }

          } // SKV 'older'

          {

            // Empty: 2 3 OrgNo: 1 Text: 0
            auto const SKV_ROW_0_MAP_1 = RowMap{
              .ixs = {
                {FieldType::Empty,{2,3}}
              ,{FieldType::OrgNo,{1}}
              ,{FieldType::Text,{0}}
            }};

            if (statement_mapping.m_row_0_map == SKV_ROW_0_MAP_1) {
              if (true) logger::development_trace(
                  "Matches SKV_ROW_0_MAP_1:{}"
                ,to_string(SKV_ROW_0_MAP_1));
              // "THE ITFIED AB";"556782-8172";"";""
              auto org_name_cix = statement_mapping.m_row_0_map.ixs.at(FieldType::Text).front();
              auto org_no_cix = statement_mapping.m_row_0_map.ixs.at(FieldType::OrgNo).front();
              auto const& org_name_candidate = table.rows[0][org_name_cix];
              auto const& org_nr_candidate = table.rows[0][org_no_cix];

              if (auto maybe_org_no = SKV::to_org_no(org_nr_candidate)) {
                if (true) logger::development_trace("maybe_org_no:{}",maybe_org_no->with_hyphen());            
                candidate = AccountID{"SKV",maybe_org_no->with_hyphen()};
                return candidate; // SUCCESS
              }
            }

          } // 'newer' SKV

          if (!statement_mapping.maybe_common) return {};

          if (statement_mapping.has_heading) {
            std::map<FieldIx,std::string> column_headings_map{};
            for (auto const& [id,idxs] : statement_mapping.maybe_common->ixs) {
              if ( !(    (id == FieldType::Date) 
                      or (id == FieldType::Amount) 
                      or ( id == FieldType::Text))) continue;
              for (auto ix : idxs) {
                auto const& text = table.rows[0][ix];
                column_headings_map[ix] = text;
              }
            }

            if (true) logger::development_trace("column_headings_map:{}",column_headings_map);

            // {0: "Bokföringsdag", 1: "Belopp", 4: "Namn", 5: "Ytterligare detaljer", 8: "Saldo", 9: "Valuta"}
            const auto NORDEA_HEADING_MAP = std::map<FieldIx,std::string>{
              {0,"Bokföringsdag"}
              ,{1,"Belopp"}
              ,{4,"Namn"}
              ,{5,"Ytterligare detaljer"}
              ,{8,"Saldo"}
              ,{9,"Valuta"}
            };

            if (column_headings_map == NORDEA_HEADING_MAP) {
              candidate = AccountID{"NORDEA",""};
              return candidate; // SUCCESS
            }

          }
          else {
            if (true) logger::development_trace("No Heading");
          } // has heading

          if (true) logger::development_trace("returns nullopt");
          return {};
        }

        TableMeta generic_like_to_statement_table_meta(CSV::Table const& table) {
          std::optional<StatementMapping> maybe_statement_mapping;
          std::optional<ColumnMapping> maybe_column_mapping;
          std::optional<AccountID> maybe_account_id;
          maybe_statement_mapping = generic_like_to_statement_mapping(table);
          if (maybe_statement_mapping) {
            CSV::MDTable<StatementMapping> mapped_table{*maybe_statement_mapping,table};
            maybe_column_mapping = generic_like_to_column_mapping(mapped_table);
            maybe_account_id = generic_like_to_account_id(mapped_table);
          }
          TableMeta result{
             .statement_mapping = maybe_statement_mapping.value_or(StatementMapping{})
            ,.column_mapping = maybe_column_mapping.value_or(ColumnMapping{})
            ,.account_id = maybe_account_id.value_or(AccountID{})
          };
          return result;
        }

        ColumnMapping skv_like_to_column_mapping(CSV::Table const& table) {
          logger::scope_logger log_raii(logger::development_trace,"skv_like_to_column_mapping",logger::LogToConsole::ON);

          ColumnMapping result{};
          auto rows_map = to_rows_map(table.rows);
          // Try for SKV account statement file

          if (true) log_the_rows_map(table.rows,rows_map);

          // row:^Ingående saldo 2025-07-01^^658 
          // map: Empty: 0 2 Amount: 3 Text: 1
          // row:^Utgående saldo 2025-09-30^^660 
          // map: Empty: 0 2 Amount: 3 Text: 1
          auto is_skv_saldo_entry_candidate = [](auto const& row_map) -> bool {    
            static const auto SKV_SALDO_ROW_MAP = RowMap{
              .ixs = {
                {FieldType::Empty,{0,2}}
                ,{FieldType::Amount,{3}}
                ,{FieldType::Text,{1}}
              }
            };
            return row_map == SKV_SALDO_ROW_MAP;      
          }; // is_skv_saldo_entry_candidate
          
          auto in_saldo_candidate_iter = std::ranges::find_if(
            rows_map
            ,is_skv_saldo_entry_candidate
          );

          if (true) logger::development_trace("in_saldo_candidate_iter != rows_map.end():{}",in_saldo_candidate_iter != rows_map.end());

          if (in_saldo_candidate_iter != rows_map.end()) {
            auto trans_span_begin = in_saldo_candidate_iter+1;
            auto out_saldo_candidate_iter = std::find_if(
              trans_span_begin
              ,rows_map.end()
              ,is_skv_saldo_entry_candidate
            );
  
  
  
            if (out_saldo_candidate_iter != rows_map.end()) {
  
              auto trans_candidates_count = std::distance(in_saldo_candidate_iter,out_saldo_candidate_iter) -1;
              logger::development_trace("trans_candidates_count:{}",trans_candidates_count);

              if (trans_candidates_count==0) return {};

              auto trans_span_end = out_saldo_candidate_iter;
              // We have a span to check
              auto key_map = *(in_saldo_candidate_iter+1);
              auto is_skv_trans_entry_candidate = [&key_map](auto const& row_map){
                if (!row_map.ixs.contains(FieldType::Date) or row_map.ixs.at(FieldType::Date).size()!=1) return false;
                if (!row_map.ixs.contains(FieldType::Text) or row_map.ixs.at(FieldType::Text).size()!=1) return false;
                if (!row_map.ixs.contains(FieldType::Amount) or row_map.ixs.at(FieldType::Amount).size()!=2) return false;
                return row_map.ixs == key_map.ixs;
              };

              if (std::all_of(
                trans_span_begin
                ,trans_span_end
                ,is_skv_trans_entry_candidate
              )) {
                // Good enough - This is probably an SKV account statement csv file table
                result.date_column = key_map.ixs.at(FieldType::Date).front();
                result.description_column = key_map.ixs.at(FieldType::Text).front();
                result.transaction_amount_column = key_map.ixs.at(FieldType::Amount)[0];
                result.saldo_amount_column = key_map.ixs.at(FieldType::Amount)[1];
              }
            }

          }

          if (true) logger::development_trace("returns result.is_valid:{}",result.is_valid());

          return result;
        } // skv_like_to_column_mapping

        ColumnMapping nordea_like_to_column_mapping(CSV::Table const& table) {
          logger::scope_logger log_raii(logger::development_trace,"nordea_like_to_column_mapping",logger::LogToConsole::ON);

          ColumnMapping result{};

          auto const& rows = table.rows;
          auto rows_map = to_rows_map(rows);
          if (true) log_the_rows_map(table.rows,rows_map);

          // Try for NORDEA like account statement table
          logger::development_trace("rows_map size {}",rows_map.size());

          if (rows_map.size() == 0) return {}; // Empty table

          auto is_trans_entry_candidate = [](auto const& row_map){
              if (!row_map.ixs.contains(FieldType::Date) or row_map.ixs.at(FieldType::Date).size()!=1) return false;
              if (!row_map.ixs.contains(FieldType::Amount) or row_map.ixs.at(FieldType::Amount).size()<2) return false;
              if (!row_map.ixs.contains(FieldType::Text) or row_map.ixs.at(FieldType::Text).size()==0) return false;
              return true;
            };

          auto first_trans_iter_candidate = std::ranges::find_if(
             rows_map
            ,is_trans_entry_candidate);

          if (first_trans_iter_candidate == rows_map.end()) {
            logger::development_trace("No is_trans_entry_candidate match");
            return {};
          }

          auto trans_iter_candidates_end = std::find_if_not(
             first_trans_iter_candidate
            ,rows_map.end()
            ,is_trans_entry_candidate
          );

          auto trans_candidates_count = std::distance(first_trans_iter_candidate,trans_iter_candidates_end);
          if (true) logger::development_trace("trans_candidates_count:{}",trans_candidates_count);

          // Fold all candidate row maps using 'common' to get the common row map
          auto iter = first_trans_iter_candidate +1;
          auto iter_end = trans_iter_candidates_end;
          auto common = *first_trans_iter_candidate;
          while (iter != iter_end) {
            auto const& rhs = *iter;
            RowMap next{};
            for (auto const& [lhs_type,lhs_v] : common.ixs) {
              if (!rhs.ixs.contains(lhs_type)) continue;
              auto const& rhs_v = rhs.ixs.at(lhs_type);
              std::vector<FieldIx> intersection_v{};
              std::ranges::set_intersection(
                 lhs_v
                ,rhs_v
                ,std::back_inserter(intersection_v)
              );
              if (intersection_v.size()>0) next.ixs[lhs_type] = intersection_v;
            }
            common = next;
            ++iter;
          }

          if (true) logger::development_trace("common :{}",to_string(common));

          if (!common.ixs.contains(FieldType::Date) or common.ixs.at(FieldType::Date).size() != 1) {
            if (true) logger::development_trace("Expected common Date column");
            return {};
          }
          if (!common.ixs.contains(FieldType::Amount) or common.ixs.at(FieldType::Amount).size() != 2) {
            if (true) logger::development_trace("Expected two common amount columns");
            return {};
          }
          if (!common.ixs.contains(FieldType::Text) or common.ixs.at(FieldType::Text).size() == 0) {
            if (true) logger::development_trace("Expected at least one common text column");
            return {};
          }

          auto begin_rix = std::distance(rows_map.begin(),first_trans_iter_candidate);
          auto end_rix = std::distance(rows_map.begin(),trans_iter_candidates_end);

          std::vector<std::pair<Amount,Amount>> amounts{};
          for (auto rix=begin_rix;rix<end_rix;++rix) {
            auto const& row = rows[rix];
            auto first_cix = common.ixs.at(FieldType::Amount).front();
            auto second_cix = common.ixs.at(FieldType::Amount).back();
            auto maybe_first_amount = to_amount(row[first_cix]);
            auto maybe_second_amount = to_amount(row[second_cix]);
            if (maybe_first_amount and maybe_second_amount) {
              amounts.push_back({*maybe_first_amount,*maybe_second_amount});
            }
            else {
              if (true) logger::design_insufficiency("Expected two valid amount after successfull rows mapping");
              return {};
            }
          }

          if (amounts.size()<2) {
            if (true) logger::design_insufficiency("Failed to determine saldo amount column for less than two entry candidates");
            return {};
          }

          // Detect trans- vs saldo-amount columns
          unsigned raising_date_first_trans_second_saldo_count{};
          unsigned falling_date_first_trans_second_saldo_count{};
          unsigned undetermined_amounts_count{};
          for (size_t pred_six=0;pred_six+1 < amounts.size();++pred_six) {
            auto succ_six = pred_six+1;
            auto [pred_first,pred_second] = amounts[pred_six];
            auto [succ_first,succ_second] = amounts[succ_six];

            logger::development_trace(
               "pred_first:{} pred_second:{} succ_first:{} succ_second:{}"
              ,::to_string(pred_first)
              ,::to_string(pred_second)
              ,::to_string(succ_first)
              ,::to_string(succ_second));
            
            logger::development_trace(
               "pred_second:{} + succ_first:{} = {} ==?== {} : succ_second"
               ,::to_string(pred_second)
               ,::to_string(succ_first)
               ,::to_string(pred_second+succ_first)
               ,::to_string(succ_second));

            if (pred_second+succ_first == succ_second) {
              ++raising_date_first_trans_second_saldo_count;
              logger::development_trace("raising_date_first_trans_second_saldo_count:{}",raising_date_first_trans_second_saldo_count);
            }
            else if (succ_second+pred_first == pred_second) {
              ++falling_date_first_trans_second_saldo_count;
              logger::development_trace("falling_date_first_trans_second_saldo_count:{}",falling_date_first_trans_second_saldo_count);
            }
            else {
              ++undetermined_amounts_count;
              logger::development_trace("undetermined_amounts_count:{}",undetermined_amounts_count);
            }

          }

          // Log
          if (true) logger::development_trace(
             "raising_date_first_trans_second_saldo_count:{} falling_date_first_trans_second_saldo_count:{}"
            ,raising_date_first_trans_second_saldo_count
            ,falling_date_first_trans_second_saldo_count);

          if (true) logger::development_trace(
             "undetermined_amounts_count:{}"
            ,undetermined_amounts_count);

          // Bail out on fails
          if (undetermined_amounts_count > 0) {
            if (true) logger::development_trace(
               "undetermined_amounts_count:{} should be 0 for amounts.size()-1:{}"
              ,undetermined_amounts_count
              ,amounts.size()-1);
            return {};
          }
          if (     (falling_date_first_trans_second_saldo_count > 0) 
               and (falling_date_first_trans_second_saldo_count != amounts.size()-1)) {
            if (true) logger::development_trace(
               "falling_date_first_trans_second_saldo_count:{} don't match amounts.size()-1:{}"
              ,falling_date_first_trans_second_saldo_count
              ,amounts.size()-1);
            return {};
          }
          if (     (raising_date_first_trans_second_saldo_count > 0) 
               and (raising_date_first_trans_second_saldo_count != amounts.size()-1)) {
            if (true) logger::development_trace(
               "raising_date_first_trans_second_saldo_count:{} don't match amounts.size()-1:{}"
              ,falling_date_first_trans_second_saldo_count
              ,amounts.size()-1);
            return {};
          }

          bool is_new_to_old_order = (falling_date_first_trans_second_saldo_count>0);
          if (true) logger::development_trace("is_new_to_old_order:{}",is_new_to_old_order);

          // Register validated Date and amount columns
          result.date_column = common.ixs[FieldType::Date].front();
          result.transaction_amount_column = common.ixs[FieldType::Amount].front();
          result.saldo_amount_column = common.ixs[FieldType::Amount].back();

          std::vector<std::string> text_coumn_values{};
          std::map<FieldIx,std::string> text_columns_map{};
          for (auto ix : common.ixs.at(FieldType::Text)) {
            auto const& text = rows[0][ix];
            text_columns_map[ix] = text;
            text_coumn_values.push_back(text);
          }
          if (true) logger::development_trace("text_coumn_values:{}",text_coumn_values);
          if (true) logger::development_trace("text_columns_map:{}",text_columns_map);

          const auto NORDEA_TEXT_MAP = std::map<FieldIx,std::string>{
             {4, "Namn"}
            ,{5, "Ytterligare detaljer"}
            ,{9, "Valuta"}};

          if (text_columns_map == NORDEA_TEXT_MAP) {
            result.description_column = 4;
            result.additional_description_columns.push_back(5);
          }
          
          if (true) logger::development_trace("returns result.is_valid:{}",result.is_valid());

          return result;
        } // nordea_like_to_column_mapping

        namespace to_deprecate {
          ColumnMapping detect_columns_from_header(CSV::Table::Heading const& heading) {

            logger::scope_logger log_raii(logger::development_trace,"detect_columns_from_header");

            ColumnMapping mapping;

            // Helper to check if a string contains a keyword (case-insensitive)
            auto contains_keyword = [](std::string_view text, std::vector<std::string_view> const& keywords) {
              std::string lower_text;
              lower_text.reserve(text.size());
              std::ranges::transform(text, std::back_inserter(lower_text),
                [](char c) { return std::tolower(c); });

              return std::ranges::any_of(keywords, [&lower_text](std::string_view keyword) {
                return lower_text.find(keyword) != std::string::npos;
              });
            };

            // Date keywords
            std::vector<std::string_view> date_keywords = {
              "date", "datum", "bokföringsdag", "dag", "bokforingsdag"
            };

            // Amount keywords
            std::vector<std::string_view> amount_keywords = {
              "amount", "belopp", "suma", "summa"
            };

            // Description keywords (prioritized)
            std::vector<std::string_view> primary_description_keywords = {
              "namn", "name", "description", "rubrik", "titel", "title"
            };

            std::vector<std::string_view> secondary_description_keywords = {
              "meddelande", "message", "text", "detaljer", "details"
            };

            // Scan header columns
            for (size_t i = 0; i < heading.size(); ++i) {
              std::string col_name = heading[i];

              // Check for date column
              if (mapping.date_column == -1 && contains_keyword(col_name, date_keywords)) {
                mapping.date_column = static_cast<int>(i);
              }

              // Check for transaction amount column
              if (mapping.transaction_amount_column == -1 && contains_keyword(col_name, amount_keywords)) {
                mapping.transaction_amount_column = static_cast<int>(i);
              }

              // Check for primary description column
              if (mapping.description_column == -1 && contains_keyword(col_name, primary_description_keywords)) {
                mapping.description_column = static_cast<int>(i);
              }

              // Check for additional description columns
              if (contains_keyword(col_name, secondary_description_keywords)) {
                mapping.additional_description_columns.push_back(static_cast<int>(i));
              }
            }

            logger::development_trace("returns mapping.is_valid:{}",mapping.is_valid());

            return mapping;
          } // detect_columns_from_header

          ColumnMapping detect_columns_from_data(CSV::Table::Rows const& rows) {
            ColumnMapping mapping;

            logger::scope_logger log_raii(logger::development_trace,"detect_columns_from_data");

            if (rows.empty()) {
              return mapping;
            }

            // Determine number of columns
            size_t num_columns = 0;
            for (auto const& row : rows) {
              num_columns = std::max(num_columns, row.size());
            }

            if (num_columns == 0) {
              return mapping;
            }

            // Sample rows for analysis (skip empty first columns - balance rows)
            auto sample_rows = 
                rows
              | std::views::filter([](auto const& row) {
                  return row.size() > 0 && row[0].size() > 0;
                })
              | std::views::take(10);

            std::vector<CSV::Table::Row> sampled;
            std::ranges::copy(sample_rows, std::back_inserter(sampled));

            if (sampled.empty()) {
              logger::development_trace("if (sampled.empty()) -> returns mapping.is_valid:{}",mapping.is_valid());
              return mapping;
            }

            // Check each column
            for (size_t col = 0; col < num_columns; ++col) {

              int valid_dates = 0;
              int valid_amounts = 0;

              for (auto const& row : sampled) {
                if (col >= row.size() || row[col].size() == 0) {
                  continue;
                }

                if (to_date(row[col]).has_value()) {
                  valid_dates++;
                }
                else if (to_amount(row[col]).has_value()) {
                  valid_amounts++;
                }
                else {
                  // Nor date or amount
                }
              } // for row

              if (valid_dates == sampled.size()) {
                // Column is Date if ALL values are valid dates
                mapping.date_column = static_cast<int>(col);
              }
              else if (valid_amounts == sampled.size()) {
                // First amount column found becomes transaction amount
                // Second amount column found becomes saldo amount
                if (mapping.transaction_amount_column == -1) {
                  mapping.transaction_amount_column = static_cast<int>(col);
                }
                else if (mapping.saldo_amount_column == -1) {
                  mapping.saldo_amount_column = static_cast<int>(col);
                }
              }
              else {
                // Not a date nor an amount column
              }
            } // for column

            // Description is typically the first text column that's not date or amount
            for (size_t col = 0; col < num_columns; ++col) {
              if (static_cast<int>(col) != mapping.date_column &&
                  static_cast<int>(col) != mapping.transaction_amount_column &&
                  static_cast<int>(col) != mapping.saldo_amount_column &&
                  mapping.description_column == -1) {

                // Check if column has substantial text content
                bool has_text = false;
                for (auto const& row : sampled) {
                  if (col < row.size() && row[col].size() > 2) {
                    has_text = true;
                    break;
                  }
                }

                if (has_text) {
                  mapping.description_column = static_cast<int>(col);
                }
              }
            }

            logger::development_trace("returns mapping.is_valid:{}",mapping.is_valid());

            return mapping;
          } // detect_columns_from_data

          ColumnMapping to_column_mapping(CSV::Table const& table) {
            ColumnMapping result{};
            result = table::to_deprecate::detect_columns_from_header(table.heading);
            if (!result.is_valid()) {
              result = table::to_deprecate::detect_columns_from_data(table.rows);
            }
            return result;
          } // to_column_mapping

        } // to_deprecate

        namespace detail {

          using ColumnMappingFn = ColumnMapping(*)(CSV::Table const&);

          constexpr std::array<ColumnMappingFn, 3> column_mapping_projectors = {
             &table::skv_like_to_column_mapping
            ,&table::nordea_like_to_column_mapping
            ,&table::to_deprecate::to_column_mapping
          }; 


        } // detail

        ColumnMapping to_column_mapping(CSV::Table const& table) {
            for (auto project : detail::column_mapping_projectors) {
                ColumnMapping mapping = project(table);
                if (mapping.is_valid()) {
                    return mapping;
                }
            }
            return {};
        } // to_column_mapping

        // TableMeta to_account_statement_table_meta(CSV::Table const& table) {
        //   TableMeta result{};
        //   result.statement_mapping.column_mapping = to_column_mapping(table);
        //   return result;
        // } // to_account_statement_table_meta


        bool is_ignorable_row(CSV::Table::Row const& row, ColumnMapping const& mapping) {
          // Empty row is ignorable
          if (row.size() == 0) {
            return true;
          }

          // Row with empty first column (common pattern for balance rows)
          if (row[0].size() == 0) {
            return true;
          }

          // Check description column for balance keywords
          if (mapping.description_column >= 0 &&
              static_cast<size_t>(mapping.description_column) < row.size()) {

            std::string desc = row[mapping.description_column];
            std::ranges::transform(desc, desc.begin(),
              [](char c) { return std::tolower(c); });

            if (desc.find("saldo") != std::string::npos ||
                desc.find("ingående") != std::string::npos ||
                desc.find("ingaende") != std::string::npos ||
                desc.find("utgående") != std::string::npos ||
                desc.find("utgaende") != std::string::npos) {
              return true;
            }
          }

          std::print("\nis_ignorable_row: TODO - ISSUE20260114_SKV_CSV Ignore row if mapping fails");

          return false;
        } // is_ignorable_row

      } // table

      OptionalAccountStatementEntries csv_table_to_account_statement_entries(CSV::Table const& table) {
        logger::scope_logger log_raii{logger::development_trace, "csv_table_to_account_statement_entries(table)"};
        
        auto mapping = table::to_column_mapping(table);

        if (!mapping.is_valid()) {
          return std::nullopt;
        }

        // Extract entries from rows
        AccountStatementEntries entries;

        for (auto const& row : table.rows) {
          auto maybe_entry = table::extract_entry_from_row(row, mapping);
          if (maybe_entry) {
            entries.push_back(*maybe_entry);
          }
        }

        logger::development_trace("Returns entries with size:{}",entries.size());

        // Return the entries (may be empty if all rows were ignorable)
        return entries;
      } // csv_table_to_account_statement_entries

      std::optional<AccountStatement> csv_table_to_account_statement_step(
          CSV::Table const& table,
          AccountID const& account_id) {
        logger::scope_logger log_raii{logger::development_trace, "csv_table_to_account_statement_step(table, account_id)"};

        // Extract entries from the CSV table
        auto maybe_entries = csv_table_to_account_statement_entries(table);

        if (!maybe_entries) {
          logger::development_trace("Failed to extract entries from CSV table");
          return std::nullopt;
        }

        // Create AccountStatement with entries and account ID metadata
        AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};

        logger::development_trace("Creating AccountStatement with {} entries and account_id: {}",
          maybe_entries->size(), account_id.to_string());

        return AccountStatement(*maybe_entries, meta);
      } // csv_table_to_account_statement_step

      std::optional<AccountStatement> statement_id_ed_to_account_statement_step(CSV::MDTable<account::statement::TableMeta> const& statement_id_ed) {
        logger::scope_logger log_raii{logger::development_trace,
          "statement_id_ed_to_account_statement_step(statement_id_ed)"};

        auto const& [table_meta,table] = statement_id_ed;

        AccountID const& account_id = table_meta.account_id;

        logger::development_trace("Processing MDTable with AccountID: '{}'", account_id.to_string());

        // Extract entries from the CSV table
        auto maybe_entries = csv_table_to_account_statement_entries(table);

        if (!maybe_entries) {
          logger::development_trace("Failed to extract entries from CSV table in MDTable");
          return std::nullopt;
        }

        // Create AccountStatement with entries and account ID metadata
        AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};

        logger::development_trace("Creating AccountStatement with {} entries and account_id: {}",
          maybe_entries->size(), account_id.to_string());

        return AccountStatement(*maybe_entries, meta);
      } // statement_id_ed_to_account_statement_step


    } // maybe

    namespace monadic {

      AnnotatedMaybe<AccountStatement> statement_id_ed_to_account_statement_step(CSV::MDTable<account::statement::TableMeta> const& statement_id_ed) {

        auto f = cratchit::functional::to_annotated_maybe_f(
           account::statement::maybe::statement_id_ed_to_account_statement_step
          ,"Account ID.ed table -> account statement failed");

        return f(statement_id_ed);

      }


    } // monadic
  } // statement
} // account