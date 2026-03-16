#include "statement_table_meta.hpp"
#include "text/functional.hpp" // text::functional::count_alpha
#include "logger/log.hpp"
#include <set>

namespace SKV {
  std::optional<OrgNo> to_org_no(std::string_view sv) {
    auto len = sv.size();
    int digits_count{};
    int hyphen_count{};
    for (size_t i=0;i<sv.size();++i) {
      auto ch = sv[i];
      if (std::isdigit(ch)) ++digits_count;
      if (ch == '-') {
        if (i!=len-5) return {};
        ++hyphen_count;
      }
    }

    if (true) logger::development_trace(
      "sv:{}:{} digits_count:{} hyphen_count:{}"
      ,sv
      ,len
      ,digits_count
      ,hyphen_count);

    if (hyphen_count>1) return {};
    if (digits_count + hyphen_count != len) return {};
    if (!(digits_count == 10 or digits_count == 12)) return {};

    if (true) logger::development_trace("org-no, candidate has acceptable counts");

    return OrgNo{
        .value = std::string(sv.substr(0,len-4-hyphen_count))
      ,.control = std::string(sv.substr(len-4,4))
    };
  }; // to_org_no

} // SKV

namespace account {
  namespace statement {

    namespace maybe {

      namespace table {

        // BEGIN FieldType

        std::string to_string(FieldType field_type) {
          std::string result{"?FieldType?"};
          switch (field_type) {
            case FieldType::Unknown: result = "Unknown"; break;
            case FieldType::Empty: result = "Empty"; break;
            case FieldType::Date: result = "Date"; break;
            case FieldType::Amount: result = "Amount"; break;
            case FieldType::OrgNo: result = "OrgNo"; break;
            case FieldType::Text: result = "Text"; break;
            case FieldType::Undefined: result = "Undefined"; break;
          }
          return result;
        }

        FieldType to_field_type(std::string const& s) {
          if (s.size()==0) {
            return FieldType::Empty;
          }
          else if (auto maybe_date = to_date(s)) {
            return FieldType::Date;
          }
          else if (auto maybe_amount = to_amount(s)) {
            return FieldType::Amount;
          }
          else if (auto maybe_org_no = SKV::to_org_no(s)) {
            return FieldType::OrgNo;
          }
          else if (text::functional::count_alpha(s) > 0) {
            return FieldType::Text;
          }
          return FieldType::Unknown;
        }

        // END FieldType

        std::string to_string(RowMap const& row_map) {
          std::string result{};

          for (auto const& entry : row_map.ixs) {
            result += std::format(" {}",to_string(entry.first));
            result += ":";
            for (auto ix : entry.second) {
              result += std::format(" {}",ix);
            }
          }
          return result;
        }

        // BEGIN FoundSaldo

        FoundSaldo::FoundSaldo(std::ptrdiff_t rix,Date date,Amount ta)
          : m_value(rix,TaggedAmount(date,to_cents_amount(ta))) {}

        // END FoundSaldo

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

        std::size_t StatementMapping::transaction_candidates_count() const {
          return tix_end-tix_begin;
        }

        std::optional<StatementMapping> generic_like_to_statement_mapping(CSV::Table const& table) {

          logger::scope_logger log_raii(logger::development_trace,"generic_like_to_statement_mapping",logger::LogToConsole::OFF);

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
            if (!row_map.ixs.contains(FieldType::Amount)) return false;
            if (!row_map.ixs.contains(FieldType::Text)) return false;
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

          auto tix_begin = std::distance(rows_map.begin(),first_trans_iter_candidate);
          auto tix_end = std::distance(rows_map.begin(),trans_iter_candidates_end);

          candidate.tix_begin = tix_begin;
          candidate.tix_end = tix_end;


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
          for (auto rix=tix_begin;rix<tix_end;++rix) {
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
          logger::scope_logger log_raii(logger::development_trace,"generic_like_to_column_mapping",logger::LogToConsole::OFF);
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

              if (!candidate.is_valid()) {
                const auto NORDEA_TEXT_MAP = std::map<FieldIx,std::string>{
                  {4, "Namn"}
                  ,{5, "Ytterligare detaljer"}
                  ,{9, "Valuta"}};

                if (text_columns_map == NORDEA_TEXT_MAP) {
                  candidate.description_column = 4;
                  candidate.additional_description_columns.push_back(5);
                }
              }

              if (!candidate.is_valid()) {
                // Final call - generic assign of columns left to right (fingers crossed)
                // TODO: Consider to filter on heading names like 'Namn', "Valuta"?
                int state{0};
                for (auto const& [cix,text] : text_columns_map) {
                  switch (state) {
                    case 0: candidate.description_column = cix; break;
                    default:
                      candidate.additional_description_columns.push_back(cix);
                      break;
                  }
                  ++state;
                }
                
              }

            }
            else if (statement_mapping.maybe_common->ixs.at(FieldType::Text).size()>1) {
              // fall back - fingers crossed the left-most text column is description
              // TODO: Consider to filter on actual text field that are currency like 'SEK'?
              int state{};
              for (auto cix : statement_mapping.maybe_common->ixs.at(FieldType::Text)) {
                switch (state) {
                  case 0: candidate.description_column = cix; break;
                  default:
                      candidate.additional_description_columns.push_back(cix);
                      break;
                }
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
          logger::scope_logger log_raii(logger::development_trace,"generic_like_to_account_id",logger::LogToConsole::OFF);

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
                candidate = AccountID{"SKV","Anonymous"};
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
                if (true) logger::development_trace("maybe_org_no:{}",maybe_org_no->without_hyphen());            
                candidate = AccountID{"SKV",maybe_org_no->without_hyphen()};
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
            const auto NORDEA_HEADING_MAP_1 = std::map<FieldIx,std::string>{
              {0,"Bokföringsdag"}
              ,{1,"Belopp"}
              ,{4,"Namn"}
              ,{5,"Ytterligare detaljer"}
              ,{8,"Saldo"}
              ,{9,"Valuta"}
            };

            // {0: "Datum", 1: "Belopp", 5: "Ytterligare detaljer", 8: "Saldo", 9: "Valuta"}
            const auto NORDEA_HEADING_MAP_2 = std::map<FieldIx,std::string>{
               {0,"Datum"}
              ,{1,"Belopp"}
              ,{5,"Ytterligare detaljer"}
              ,{8,"Saldo"}
              ,{9,"Valuta"}
            };

            // {0: "Datum", 1: "Belopp", 5: "Ytterligare detaljer", 6: "Meddelande", 8: "Saldo", 9: "Valuta"}
            const auto NORDEA_HEADING_MAP_3 = std::map<FieldIx,std::string>{
               {0,"Datum"}
              ,{1,"Belopp"}
              ,{5,"Ytterligare detaljer"}
              ,{6,"Meddelande"}
              ,{8,"Saldo"}
              ,{9,"Valuta"}
            };

            if (     (column_headings_map == NORDEA_HEADING_MAP_1)
                  or (column_headings_map == NORDEA_HEADING_MAP_2)
                  or (column_headings_map == NORDEA_HEADING_MAP_3)) {
              candidate = AccountID{"NORDEA","Anonymous"};
              return candidate; // SUCCESS
            }

          }
          else {
            if (true) logger::development_trace("No Heading");
          } // has heading

          if (true) logger::development_trace("returns nullopt");
          return {};
        } // generic_like_to_account_id

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
            ,.account_id = maybe_account_id.value_or(AccountID{"Generisk","??"})
          };
          return result;
        } // generic_like_to_statement_table_meta

      } // table
    } // maybe

  } // statement
} // acount