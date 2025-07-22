#include "csv.hpp"

namespace CSV {
namespace NORDEA {

OptionalTaggedAmount to_tagged_amount(FieldRow const& field_row, Table::Heading const& heading) {
	OptionalTaggedAmount result{};
	if (field_row.size() >= 10) {
		auto sDate = field_row[element::Bokforingsdag];
		if (auto date = to_date(sDate)) {
			auto sAmount = field_row[element::Belopp];
			if (auto amount = to_amount(sAmount)) {
				auto cents_amount = to_cents_amount(*amount);
				TaggedAmount ta{*date,cents_amount};
				ta.tags()["Account"] = "NORDEA";
				if (field_row[element::Namn].size() > 0) {
					ta.tags()["Text"] = field_row[element::Namn];
				}
				if (field_row[element::Avsandare].size() > 0) {
					ta.tags()["From"] = field_row[element::Avsandare];
				}
				if (field_row[element::Mottagare].size() > 0) {
					ta.tags()["To"] = field_row[element::Mottagare];
				}
				if (auto saldo = to_amount(field_row[element::Saldo])) {
					ta.tags()["Saldo"] = to_string(to_cents_amount(*saldo));
				}
				if (field_row[element::Meddelande].size() > 0) {
					ta.tags()["Message"] = field_row[element::Meddelande];
				}
				if (field_row[element::Egna_anteckningar].size() > 0) {
					ta.tags()["Notes"] = field_row[element::Egna_anteckningar];
				}
        // Tag with column names as defined by heading
        if (heading.size() > 0 and heading.size() == field_row.size()) {
          for (int i=0;i<heading.size();++i) {
            auto key = heading[i];
            auto value = field_row[i];
            if (ta.tags().contains(key) == false) {
              if (value.size() > 0) ta.tags()[key] = field_row[i]; // add column value tagged with column name
            }
            else {
              std::cout << "\nWarning, to_tagged_amount - Skipped conflicting column name:" << std::quoted(key) << ". Already tagged with value:" << std::quoted(ta.tags()[key]);
            }
          }
        }
        else {
          std::cout << "\nError, to_tagged_amount failed to tag amount with heading defined tags and values. No secure mapping from heading column count:" << heading.size() << " to entry column count:" << field_row.size();
        }

				result = ta;
			}
			else {
				std::cout << "\nNot a valid NORDEA amount: " << std::quoted(sAmount); 
			}
		}
		else {
			if (sDate.size() > 0) std::cout << "\nNot a valid date: " << std::quoted(sDate);
		}
	}
	return result;
}

} // namespace NORDEA

namespace SKV {

OptionalTaggedAmount to_tagged_amount(FieldRow const& field_row, Table::Heading const& heading) {
	OptionalTaggedAmount result{};
	if (field_row.size() == 5) {
		// NOTE: The SKV comma separated file is in fact a end-with-';' field file (so we get five separations where the file only contains four fields...)
		//       I.e., field index 0..3 contains values
    // NOTE! skv-files seems to be ISO_8859_1 encoded! (E.g., 'å' is ASCII 229 etc...)
    // Assume the client calling this function has already transcoded the text into internal character set and encoding (on macOS UNICODE in UTF-8)
		auto sDate = field_row[element::OptionalDate];
		if (auto date = to_date(sDate)) {
			auto sAmount = field_row[element::Belopp];
			if (auto amount = to_amount(sAmount)) {
				auto cents_amount = to_cents_amount(*amount);
				TaggedAmount ta{*date,cents_amount};
				ta.tags()["Account"] = "SKV";
				ta.tags()["Text"] = field_row[element::Text];

        // Tag with column names as defined by heading
        if (heading.size() > 0 and heading.size() == field_row.size()) {
          for (int i=0;i<heading.size();++i) {
            auto key = heading[i];
            if (ta.tags().contains(key) == false) {
              // Note: The SKV file may contain both CR (0x0D) and LF (0x0A) even when downloaded to macOS that expects only CR.
              // We can trim on the value to get rid of any residue LF
              // TODO: Make reading of text from file robust against control characters that does not match the expectation of the runtime
              auto value = tokenize::trim(field_row[i]);
              if (value.size() > 0) ta.tags()[key] = field_row[i]; // add column value tagged with column name
            }
            else {
              std::cout << "\nWarning, to_tagged_amount - Skipped conflicting column name:" << std::quoted(key) << ". Already tagged with value:" << std::quoted(ta.tags()[key]);
            }
          }
        }
        else {
          std::cout << "\nError, to_tagged_amount failed to tag amount with heading defined tags and values. No secure mapping from heading column count:" << heading.size() << " to entry column count:" << field_row.size();
        }

				result = ta;
			}
			else {
				std::cout << "\nNot a valid amount: " << std::quoted(sAmount); 
			}
		}
		else {
			// It may be a saldo entry
			/*
					skv-file entry	";Ingående saldo 2022-06-05;625;0;"
					field_row				""  "Ingående saldo 2022-06-05" "625" "0" ""
					index:           0                           1     2   3   4
			*/
			auto sOptionalZero = field_row[element::OptionalZero]; // index 3
			if (auto zero_amount = to_amount(sOptionalZero)) {
				// No date and the optional zero is present ==> Assume a Saldo entry
				// Pick the date from Text
				auto words = tokenize::splits(field_row[element::Text],' ');
				if (auto saldo_date = to_date(words.back())) {
					// Success
					auto sSaldo = field_row[element::Saldo];
					if (auto saldo = to_amount(sSaldo)) {
						auto cents_saldo = to_cents_amount(*saldo);
						TaggedAmount ta{*saldo_date,cents_saldo};
						ta.tags()["Account"] = "SKV";
						ta.tags()["Text"] = field_row[element::Text];
						ta.tags()["type"] = "saldo";
						// NOTE! skv-files seems to be ISO_8859_1 encoded! (E.g., 'å' is ASCII 229 etc...)
						// TODO: Re-encode into UTF-8 if/when we add parsing of text into tagged amount (See namespace charset::ISO_8859_1 ...)
						result = ta;
					}
					else {
						std::cout << "\nNot a valid SKV Saldo: " << std::quoted(sSaldo); 
					}
				}
				else {
						std::cout << "\nNot a valid SKV Saldo Date in entry: " << std::quoted(field_row[element::Text]); 
				}
			}
			if (sDate.size() > 0) std::cout << "\nNot a valid SKV date: " << std::quoted(sDate);
		}
	}
	return result;
}

} // namespace SKV

HeadingId to_csv_heading_id(FieldRow const& field_row) {
  HeadingId result{HeadingId::Undefined};
  if (true) {
    std::cout << "\nfield_row.size() = " << field_row.size();
    std::cout << "\nto_csv_heading_id(field_row:" << std::quoted(to_string(field_row)) << ")";
    for (auto const& field : field_row) std::cout << "\n\t[" << "]" << std::quoted(field);
  }
  if (field_row.size() >= 10) {
    if (true) {
      std::cout << "\nNORDEA File candidate ok";
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
      std::cout << "\nSKV File candidate ok";
    }
    result = HeadingId::SKV;
  }
  return result;
}

} // namespace CSV