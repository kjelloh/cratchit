#pragma once

#include "Key.hpp" // Key::Path,
#include "text/encoding.hpp"
#include "fiscal/amount/TaggedAmountFramework.hpp"
#include "tokenize.hpp"
#include <iostream> // std::cout,

namespace CSV {
	using FieldRow = Key::Path;
	using FieldRows = std::vector<FieldRow>;
	using OptionalFieldRows = std::optional<FieldRows>;

	OptionalFieldRows to_field_rows(auto& in,char delim=';') {
    if (false) {
      std::cout << "\nto_field_rows(auto& in...";
    }
		OptionalFieldRows result{};
		try {
			FieldRows field_rows{};
			std::string raw_entry{};
      while (auto entry = in.getline(encoding::unicode::to_utf8{})) {
				field_rows.push_back({*entry,delim});
      }
			result = field_rows;
		}
		catch (std::exception const& e) {
			std::cout << "\nDESIGN INSUFFICIENCY: to_field_rows failed. Exception=" << std::quoted(e.what());
		}
		return result;
	}

  using TableHeading = FieldRow;
  using OptionalTableHeading = std::optional<TableHeading>;

  // Helper to identify a valid function to_heading(row_0) : FieldRow -> std::optional<TableHeading> 
  template<typename ToHeading, typename FieldRow>
  concept ToHeadingConcept = requires(ToHeading to_heading, FieldRow field_row) {
      { to_heading(field_row) } -> std::convertible_to<std::optional<TableHeading>>;
  };  

  struct Table {
    using Heading = FieldRow;
    using Row = FieldRow;
    using Rows = std::vector<Row>;
    Heading heading;
    Rows rows;
  };
  using OptionalTable = std::optional<Table>;

  OptionalTable to_table(OptionalFieldRows const& field_rows,ToHeadingConcept<FieldRow> auto to_heading) {
    OptionalTable result{};
    if (field_rows and field_rows->size()>0) {
      Table table{};
      if (auto heading = to_heading(field_rows->at(0))) {
        table.heading = *heading;
        table.rows = *field_rows; // Keep all rows inclduing heading row if there is one (TODO: Remove when mechanism to differ between rows with and without a heading row 0 implemented)
        result = table;
      }
    }
    return result;
  }

} // namespace CSV

namespace CSV {
	// For CSV-files
	namespace NORDEA {
		// For NORDEA CSV-files (bank account statements)

		// Assume Finland located bank Nordea swedish web csv format of transactions to/from an account
		/*
		Bokföringsdag;Belopp;Avsändare;Mottagare;Namn;Rubrik;Meddelande;Egna anteckningar;Saldo;Valuta
		2022/06/27;-720,00;51 86 87-9;5343-2795;LOOPIA AB (WEBSUPPORT);LOOPIA AB (WEBSUPPORT);378587992;Webhotell Q2;49537,22;SEK
		2022/06/16;-880,00;51 86 87-9;824-3040;TELIA SVERIGE AB;TELIA SVERIGE AB;19990271223;Mobil Q2;50257,22;SEK
		2022/06/13;-625,00;51 86 87-9;5050-1055;SKATTEVERKET;SKATTEVERKET;16556782817244;Förs Avg;51137,22;SEK
		2022/06/13;-153,71;51 86 87-9;;KORT             BEANSTALK APP   26;KORT             BEANSTALK APP   26;BEANSTALK APP   2656;;;SEK
		2022/06/10;-184,00;51 86 87-9;;KORT             POSTNORD SE     26;KORT             POSTNORD SE     26;POSTNORD SE     2656;;51915,93;SEK
		2022/06/09;-399,90;51 86 87-9;;KORT             KJELL O CO 100  26;KORT             KJELL O CO 100  26;KJELL O CO 100  2656;;52099,93;SEK
		2022/06/03;-1,70;51 86 87-9;;PRIS ENL SPEC;PRIS ENL SPEC;;;52499,83;SEK
		2022/05/31;-446,00;51 86 87-9;5020-7042;Fortnox Finans AB;Fortnox Finans AB;52804292974641;Bokf pr jun/jul/aug;52501,53;SEK
		2022/05/24;786,99;;51 86 87-9;BG KONTOINS;BG KONTOINS;0817-9780 GOOGLE IRELA;;52947,53;SEK
		2022/05/19;179,00;;51 86 87-9;BG KONTOINS;BG KONTOINS;5050-1030 SK5567828172;;52160,54;SEK
		2022/05/18;-610,33;51 86 87-9;;KORT             PAYPAL *HKSITES 26;KORT             PAYPAL *HKSITES 26;PAYPAL *HKSITES 2656;;51981,54;SEK
		2022/05/12;-154,91;51 86 87-9;;KORT             BEANSTALK APP   26;KORT             BEANSTALK APP   26;BEANSTALK APP   2656;;52591,87;SEK
		2022/05/04;-1,70;51 86 87-9;;PRIS ENL SPEC;PRIS ENL SPEC;;;52746,78;SEK
		2022/04/19;-186,25;51 86 87-9;5343-2795;LOOPIA AB (WEBSUPPORT);LOOPIA AB (WEBSUPPORT);375508199;sharedplanet se;52748,48;SEK
		2022/04/12;-145,41;51 86 87-9;;KORT             BEANSTALK APP   26;KORT             BEANSTALK APP   26;BEANSTALK APP   2656;;52934,73;SEK
		2022/04/04;-6,80;51 86 87-9;;PRIS ENL SPEC;PRIS ENL SPEC;;;53080,14;SEK
		*/

		enum element: std::size_t {
			undefined
			,Bokforingsdag = 0
			,Belopp = 1
			,Avsandare = 2
			,Mottagare = 3
			,Namn = 4
			,Rubrik = 5
			,Meddelande = 6
			,Egna_anteckningar = 7
			,Saldo = 8
			,Valuta = 9
			,unknown
		};

		OptionalTaggedAmount to_tagged_amount(FieldRow const& field_row, Table::Heading const& heading = Table::Heading{}); 
	} // namespace NORDEA 

	namespace SKV {
		// For SKV CSV-files (so called skv-files with tax account statements)

		/*
			;Ing�ende saldo 2022-06-05;625;0;
			2022-06-13;F�rs.avgift moms/arbetsgivardeklaration 220314;-625;;
			2022-06-20;Inbetalning bokf�rd 220613;625;;
			;Utg�ende saldo 2022-07-02;625;0;
		*/

		enum element: std::size_t {
			undefined
			,OptionalDate=0
			,Text=1
			,Belopp=2
			,Saldo=2
			,OptionalZero=3
			,unknown
		};

		OptionalTaggedAmount to_tagged_amount(FieldRow const& field_row, Table::Heading const& heading = Table::Heading{});
	} // namespace SKV
  enum class HeadingId {
    Undefined
    ,NORDEA
    ,SKV
    ,unknown
  };

  HeadingId to_csv_heading_id(FieldRow const& field_row);
} // namespace CSV
