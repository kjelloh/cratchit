#include "projections.hpp"
#include <sstream> // std::ostringstream,
#include "logger/log.hpp"

namespace CSV {
  namespace project {
    HeadingId to_csv_heading_id(FieldRow const& field_row) {
      HeadingId result{HeadingId::Undefined};
      if (true) {
        logger::cout_proxy << "\nfield_row.size() = " << field_row.size();
        logger::cout_proxy << "\nto_csv_heading_id(field_row:" << std::quoted(to_string(field_row)) << ")";
        for (auto const& field : field_row) logger::cout_proxy << "\n\t[" << "]" << std::quoted(field);
      }
      if (field_row.size() >= 10) {
        if (true) {
          logger::cout_proxy << "\nNORDEA File candidate ok";
        }
        // Bokföringsdag;Belopp;Avsändare;Mottagare;Namn;Rubrik;Meddelande;Egna anteckningar;Saldo;Valuta
        // Bokföringsdag;Belopp;Avsändare;Mottagare;Namn;Ytterligare detaljer;Meddelande;Egna anteckningar;Saldo;Valuta;
        // Note: NORDEA web csv format has changed to incorporate and ending ';' (in effect changing ';' from being a separator to being a terminator)
        // TODO 240221: Find a way to handle NORDEA file coming in UTF-8 with a BOM prefix and the SKV-file being ISO-8859-1 encoded (a mess!)
        if (
            true
            and field_row[0].find(R"(Bokföringsdag)") != std::string::npos // avoid matching to the UTF-8 BOM prefix in a NORDEA file
            and field_row[1] == "Belopp" 
            and field_row[2] == "Avsändare"
            and field_row[3] == "Mottagare" 
            and field_row[4] == "Namn" 
            and (field_row[5] == "Rubrik" or field_row[5] == "Ytterligare detaljer")
            and field_row[6] == "Meddelande" 
            and field_row[7] == "Egna anteckningar" 
            and field_row[8] == "Saldo" 
            and field_row[9] == "Valuta") {
          result = HeadingId::NORDEA;
        }
      }
      else if (field_row.size() == 5) {
        if (true) {
          logger::cout_proxy << "\nSKV File candidate ok";
        }
        result = HeadingId::SKV;
      }
      return result;
    }

    ToHeadingProjection make_heading_projection(HeadingId const& csv_heading_id) {
      switch (csv_heading_id) {
        case HeadingId::Undefined: {
          return [](CSV::FieldRow const& field_row) -> CSV::OptionalTableHeading {
            return std::nullopt;
          };
        } break;
        case HeadingId::NORDEA: {
          return [](CSV::FieldRow const& field_row) -> CSV::OptionalTableHeading {
            if (field_row.size() > 3) {
              // Note: Requiring at least 3 'columns' is a heuristic to somewhat ensure we have at least a date, an amount and some description to work with
              // E.g. NORDEA bank CSV-file row 0: "Bokföringsdag;Belopp;Avsändare;Mottagare;Namn;Rubrik;Meddelande;Egna anteckningar;Saldo;Valuta"
              return field_row; // Assume csv-file has row 0 as the one naming the columns (as of 2024-06-09 NORDEA web bank csv-file does)
              // NOTE: This approach makes Cratchit dependent on the naming chosen by Nordea in its web bank generated CSV-file...
            }
            else {
              logger::cout_proxy << "\nDESIGN_INSUFFICIENCY: Failed to use provied field_row " << field_row << " to return a table heading. Insufficient field_row.size()=" << field_row.size();
              return std::nullopt;
            }
          };
        } break;
        case HeadingId::SKV: {
          return [](CSV::FieldRow const& field_row) -> CSV::OptionalTableHeading {
            // Assume this is row 0 of an SKV-file which in turn we assume is from SKV, the swedish tax agency, with transactions on 'our' tax account
            // This mean we have no column names in the file and need to hard code them.
            if (field_row.size() == 5) {
              return CSV::TableHeading{{"Bokföringsdag","Rubrik","Belopp","Kolumn_4","Kolumn_5"}};
            }
            else {
              return std::nullopt;
            }
          };
        } break;
        case HeadingId::unknown: {
          return [](CSV::FieldRow const& field_row) -> CSV::OptionalTableHeading {
            return std::nullopt;
          };
        } break;
      }
    }

    ToTaggedAmountProjection make_tagged_amount_projection(
       HeadingId const& csv_heading_id
      ,CSV::TableHeading const& table_heading) {
      switch (csv_heading_id) {
        case HeadingId::Undefined: {
          return [table_heading](CSV::FieldRow const& field_row) -> OptionalTaggedAmount {
            return std::nullopt;
          };
        } break;
        case HeadingId::NORDEA: {
          return [table_heading](CSV::FieldRow const& field_row) -> OptionalTaggedAmount {
            return CSV::NORDEA::to_tagged_amount(field_row,table_heading);
          };
        } break;
        case HeadingId::SKV: {
          return [table_heading](CSV::FieldRow const& field_row) -> OptionalTaggedAmount {
            return CSV::SKV::to_tagged_amount(field_row,table_heading);
          };
        } break;
        case HeadingId::unknown: {
          return [table_heading](CSV::FieldRow const& field_row) -> OptionalTaggedAmount {
            return std::nullopt;
          };
        } break;
      }
    }

  } // project
} // CSV
