//
// Created by kjell-olovhogdahl on 8/1/2016.
//

#include "SIE.h"
#include "../tris/BackEnd.h"
#include <iterator>
#include <regex>
#include <cctype> // std::toupper
#include <sstream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cmath>

namespace sisyphos {

	namespace detail {
		template <typename Tag>
		struct is_forward_iterator_tag : public std::false_type {};
		template <>
		struct is_forward_iterator_tag<std::forward_iterator_tag> : public std::true_type {};
	}

	template <typename I, typename F1, typename F2, typename F3>
	void for_first_middle_last(I _first, I _end, F1 first, F2 middle, F3 last) {
		// Always apply last to last element if at least one entry in the proved range.
		// Apply first and last if two elemnts in provded range.
		// Otherwise apply first to the first element, middle to intermediate elements and last to last element.
		static_assert(!detail::is_forward_iterator_tag<typename I::iterator_category>::value, "for_first_middle_last must be called with Forward Iteratable range");
		switch (std::distance(_first, _end)) {
		case 0: break;
		case 1: {last(*_first); } break;
		case 2: {first(*_first); last(*(++_first)); } break;
		default: {
			auto iter_pair = std::make_pair(_first, ++I(_first));
			first(*iter_pair.first);
			++iter_pair.first; ++iter_pair.second;
			while (iter_pair.second != _end) {
				middle(*iter_pair.first); // apply middle
				++iter_pair.first; ++iter_pair.second;
			}
			last(*iter_pair.first);
		} break;
		}
	}

}

namespace vine {

	template <typename Text>
	void design_insufficiency_log(const Text& entry) { 
		// Stub - Not yet implemented 
	};

}

namespace babel {


	namespace detail {
		using Script = std::string; // babel Script type to handle text

		Script case_fold(const Script& text) {
			Script result;
			// asume non-escpaded characters (E.g. not UTF-8 ‘Ä’ as it is escaped as {195/0xC3, 132/0x84} (http://www.utf8-chartable.de))
      std::transform(std::begin(text), std::end(text), std::back_inserter(result), [](char ch){ return std::tolower(ch);});
			return result;
		};
	}

	// Constructs from text convertable to babel::Script
	// Matches with any text that accepts babel::Script as parameter to Text::find
	struct keyword {
		template<typename Text>
		keyword(const Text& keyword) : m_keyword(detail::case_fold(keyword)) {}		// babel::Script must be constructable from provided Text

		template <typename Text>
		bool matches(const Text& text) {
			// TODO: Adapt to some babel solution for case-folding of Unicode text (http://unicode.org/faq/casemap_charprop.html#3)
			detail::Script case_folded_text = detail::case_fold(text);
			auto is_match = (case_folded_text.find(m_keyword) != Text::npos);		// babel::Script m_keyword must be compatible with Text::find parameter
			return is_match;
		}
	private:
		detail::Script m_keyword;
	};

}

namespace ibi {
	template <typename T>
	class Path {

	};
}

namespace sie {

	namespace parse {

		enum e_SIEElement {
			eSIEElement_Undefined
			, eSIEElement_Unknown
		};

		using Key = e_SIEElement;

		using Path = std::vector<Key>;

		using Source = std::stringstream;

		using Cache = std::vector<Source::char_type>;

		using Range = std::pair<Cache::iterator, Cache::iterator>;

		struct ElementMap : public std::pair<Path, Range> {
			ElementMap(Path p = Path(), Range r = Range()) : std::pair<Path, Range>(p,r) {}
		};

		struct Target : public SIE_Statements {
			void onParsedElement(const ElementMap& em) {
				// Create and store SIE_Element from provided element Path and range
				// em.first == Path;
				// em.second.first = begin();
				// em.second.second = end();
			}
		};


		// sie::parse::iter is iterative parsing
		namespace iter {


			struct Parser {
				// We need a parser to keep the cache alive (so that the iterator has something to reference.
				// Note: The iteartor can't carry a cache itself as the end-iterator should not aggregate any cache.

				class ElementMapIterator : public std::iterator<std::forward_iterator_tag, ElementMap> {
					ElementMap m_current;
				public:
					ElementMapIterator(Cache* p) : m_current(Path(), Range(p->begin(), p->begin())) { ++(*this); }

					ElementMapIterator(const ElementMapIterator&) = default;

					ElementMapIterator& operator++() {
						// TODO: Parse from m_p until we have the next element in range (or end)
						return *this; 
					}
					bool operator==(const ElementMapIterator& rhs) { return m_current == rhs.m_current; }
					bool operator!=(const ElementMapIterator& rhs) { return m_current != rhs.m_current; }
					ElementMap& operator*() { return m_current; }

				};

				ElementMapIterator begin() { return ElementMapIterator(&m_cache); }
				ElementMapIterator end() { return ElementMapIterator(nullptr); }

				void parse(Source& source, Target& target) {
					// Compose parsers "on stack" by call to appropriate parse-function for current element.
					// Thus the parsed structure is modelled by where in the execution whe are.
					std::copy(std::istream_iterator<char>(source), std::istream_iterator<char>(), std::back_inserter(m_cache)); // clone source to cache to ensure it is available
					std::for_each(begin(), end(), [&target](const ElementMap& em) {target.onParsedElement(em); });
				}

				Source m_src;
				Cache m_cache; // Actual cache during parse (iterator refernces this cache)
			};


			void iter_example() {
				Source source;
				Target target;
				Parser parser;
				parser.parse(source, target);
			}

		}

		// sie::parse::gen is generating parsing
		namespace gen {

			struct Parser {

				void parse(Source& source, Target& target) {
					// Compose parsers "on stack" by call to appropriate parse-function for current element.
					// Thus the parsed structure is modelled by where in the execution whe are.
					std::copy(std::istream_iterator<char>(source), std::istream_iterator<char>(), std::back_inserter(m_cache)); // clone source to cache to ensure it is available
					ElementMap em;
					target.onParsedElement(em);
				}

				Cache m_cache;
			};

			void gen_example() {
				Source source("Hello");
				Target target;
				Parser parser;
				parser.parse(source, target);
			}

		}

	}

	// The SIE-format is piblsihed at http://www.sie.se/

	using SIE_Amount = int; // Integer amount in cents

	// TODO: Templetaize on SIE_Amount to use std::stof or std::stod
	SIE_Amount to_SIE_Amount(const SIE_Element& amount_element) { return static_cast<SIE_Amount>(std::round(std::stof(amount_element) * 100)); } // Return amount in cents

	using SIE_Account_Id = unsigned int;

	enum e_SIEAccountType {
		eSIEAccountType_Undefined
		, eSIEAccountType_Asset
		, eSIEAccountType_Debt
		, eSIEAccountType_Cost
		, eSIEAccountType_Income
		, eSIEAccountType_Unknown
	};

	namespace bas {
		using BAS_Account_Id = unsigned int;

		e_SIEAccountType account_type(const SIE_Account_Id& account_id) {
			// TODO: Secure all account types (as of now only based ob class and seelcted excpetions)
			switch (account_id / 1000) {
			case 0: return eSIEAccountType_Undefined;
			case 1: return eSIEAccountType_Asset;
			case 2: return (account_id != 2640) ? eSIEAccountType_Debt : eSIEAccountType_Asset;
			case 3: return eSIEAccountType_Income;
			case 4: 
			case 5:
			case 6:
			case 7: return eSIEAccountType_Cost;
			case 8: return (account_id != 8300)? eSIEAccountType_Cost: eSIEAccountType_Income;
			case 9: return eSIEAccountType_Undefined;
			default: return eSIEAccountType_Unknown;
			}
		}

	}

	namespace experimental {

		e_SIEAccountType account_type(const SIE_Account_Id& account_id) {
			return bas::account_type(account_id);
		}

		namespace cratchit {

			namespace event {

				struct Cratchit_Event {
					Cratchit_Event(const SIE_Seed& sie_seed) {
						sisyphos::for_first_middle_last(std::begin(sie_seed), std::end(sie_seed),
							[&](const SIE_Element& e)->void {event_date = e; }	// Date is first
						, [&](const SIE_Element& e)->void {seed_label += e + " "; } // Concatinate middle parts
						, [&](const SIE_Element& e)->void {event_amount = to_SIE_Amount(e); } // Amount is last
						);
					}
					SIE_Element event_date;
					SIE_Element seed_label;
					SIE_Amount event_amount;
				};

			}
		}

		SIE_Entry VER_Statement(const SIE_Element& event_date, const SIE_Element& seed_label) {
			// SIE_Entry({ "#VER","?","??",cratchit_event.event_date,cratchit_event.seed_label,"_?today?_" }));
			SIE_Entry result({ "#VER","?","??",event_date,seed_label,"_?today?_" });
			return result;
		};

		namespace detail {
			template <typename T>
			struct is_not_instantiated : public std::false_type {};
			
			// https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Resource_Acquisition_Is_Initialization

			template <typename T>
			struct raii {
				static_assert(is_not_instantiated<T>::value,"No RAII wrapper defined for this type");
			};

			template <>
			struct raii<std::ifstream> : public std::ifstream {
				raii(const std::filesystem::path& p) : std::ifstream() {
					this->exceptions(std::ifstream::failbit | std::ifstream::badbit);
					this->open(p);
					this->exceptions(std::ifstream::goodbit); // no excpetions for client access
				}
				~raii() { this->close(); }
			};

		}

		SIE_Statements get_template_statements() {
			SIE_Statements result;
			std::filesystem::path sie_path(R"(src\sie\test\sie1.se)");
			detail::raii<std::ifstream> sie_file(sie_path);
			return result;
		}

		SIE_Statements parse_financial_events() {
			SIE_Statements result;
			auto sie_path = std::filesystem::path("src") / "sie" / "test" / "events1.txt";
			try {
				if (std::filesystem::exists(sie_path)==false) throw std::runtime_error("SIE File Does not exist");
				detail::raii<std::ifstream> sie_file(sie_path);
				SIE_Element line;
				while (std::getline(sie_file, line)) {
					std::regex whitespace_regexp("\\s+"); // tokenize on whitespace
					sie::SIE_Seed sie_seed(std::sregex_token_iterator(std::begin(line), std::end(line), whitespace_regexp, -1), std::sregex_token_iterator());
					result.push_back(create_statement_for(sie_seed));
				}			
			}
			catch (std::exception const& e) {
				SIE_Entry entry{"#ERROR",e.what()};
				SIE_Statement statement{entry};
				result.push_back(statement);
			}
			return result;
		}

		SIE_Amount account_decerase_amount(SIE_Account_Id sie_account_id, const SIE_Amount& sie_amount) {
			switch (bas::account_type(sie_account_id)) {
			case eSIEAccountType_Asset: return sie_amount * -1; // Decerase asset is negative (Credit)
			case eSIEAccountType_Debt: return sie_amount; // Decerase debt is positive (Debit)
			case eSIEAccountType_Cost: return sie_amount*-1; // Decerase cost is negative (Credit)
			case eSIEAccountType_Income: return sie_amount; // Decerase income is positive (Debit)
			default: throw std::runtime_error("account_decerase_amount failed. No Amount available for unknown account type");
			}
		}

		SIE_Amount account_increase_amount(SIE_Account_Id sie_account_id, const SIE_Amount& sie_amount) {
			switch (bas::account_type(sie_account_id)) {
			case eSIEAccountType_Asset: return sie_amount ; // Increase assets is positive (debit)
			case eSIEAccountType_Debt: return sie_amount*-1; // Increase debt is negative (credit)
			case eSIEAccountType_Cost: return sie_amount; // Increase costs is positive (Debit)
			case eSIEAccountType_Income: return sie_amount*-1; // Increase income is negative (Credit)
			default: throw std::runtime_error("account_increase_amount failed. No Amount available for unknown account type");
			}
		}

		struct VAT_Record {
			SIE_Amount amount_with_VAT;
			SIE_Amount amount_without_VAT;
			SIE_Amount VAT_Amount;
			unsigned int cent_recidue;
		};

		VAT_Record Amount_With_VAT_View(SIE_Amount amount_with_VAT) {
			// TODO: Assure this algorithm is according to Swedish rounding rules (for now it is mathematicly correct?)
			VAT_Record result;
			result.amount_with_VAT = amount_with_VAT;
			result.VAT_Amount = static_cast<decltype(result.VAT_Amount)>(std::round(0.20 * amount_with_VAT)); // https://www4.skatteverket.se/rattsligvagledning/edition/2014.3/321578.html
			result.amount_without_VAT = amount_with_VAT - result.VAT_Amount;
			result.cent_recidue = result.amount_with_VAT - result.amount_without_VAT - result.VAT_Amount;
			return result;
		}

		SIE_Element to_SIE_Element(SIE_Amount sie_amount) {
			std::stringstream ssResult;
			ssResult.setf(std::ios::fixed);
			ssResult.precision(2);
			ssResult << (sie_amount / 100.00);
			return ssResult.str();
		}

		SIE_Element to_SIE_Element(unsigned int value) {
			return std::to_string(value);
		}

		SIE_Entry TRANS_increase_account_entry(SIE_Account_Id account_id, const SIE_Amount& sie_amount, const SIE_Element& sie_comment) {
			SIE_Entry result({ "#TRANS",to_SIE_Element(account_id),"{}",to_SIE_Element(account_increase_amount(account_id,sie_amount)),"",sie_comment });
			return result;
		}

		SIE_Entry TRANS_decrease_account_entry(SIE_Account_Id account_id, const SIE_Amount& sie_amount, const SIE_Element& sie_comment) {
			SIE_Entry result({ "#TRANS",to_SIE_Element(account_id),"{}",to_SIE_Element(account_decerase_amount(account_id,sie_amount)),"",sie_comment });
			return result;
		}

		SIE_Entry TRANS_increase_cost_entry(SIE_Account_Id account_id, const SIE_Amount& sie_amount, const SIE_Element& sie_comment) {
			return TRANS_increase_account_entry(account_id,sie_amount,sie_comment);
		}

		SIE_Entry TRANS_increase_debt_entry(SIE_Account_Id account_id, const SIE_Amount& sie_amount, const SIE_Element& sie_comment) {
			return TRANS_increase_account_entry(account_id, sie_amount, sie_comment);
		}

		SIE_Entry TRANS_decrease_asset_entry(SIE_Account_Id account_id, const SIE_Amount& sie_amount,const SIE_Element& sie_comment) {
			return TRANS_decrease_account_entry(account_id, sie_amount, sie_comment);
		}

		SIE_Entry TRANS_decrease_debt_entry(SIE_Account_Id account_id, const SIE_Amount& sie_amount, const SIE_Element& sie_comment) {
			return TRANS_decrease_account_entry(account_id, sie_amount, sie_comment);
		}

		SIE_Statement create_statement_for(const SIE_Seed& sie_seed)
		{
			SIE_Statement result;
			if (sie_seed.size() >= 3) {

				SIE_Element event_date;
				SIE_Element seed_label;
				SIE_Element event_amount;

				sisyphos::for_first_middle_last(std::begin(sie_seed),std::end(sie_seed),
					 [&](const SIE_Element& e)->void {event_date = e; }	// Date is first
					,[&](const SIE_Element& e)->void {seed_label += e + " "; } // Concatinate middle parts
					,[&](const SIE_Element& e)->void {event_amount = e; } // Amount is last
				);

				SIE_Statements statements;

				if (babel::keyword("Beanstalk").matches(seed_label)) {
					/*
					#VER	1	106	20160331	"Beanstalk"	20160805
					{
					#TRANS	1920	{}	-125,63	""	"Beanstalk"
					#TRANS	6230	{}	125,63	""	"Beanstalk"
					}
					*/
					{
						SIE_Statement statement;
						cratchit::event::Cratchit_Event cratchit_event(sie_seed);

						statement.push_back(VER_Statement(cratchit_event.event_date,cratchit_event.seed_label));// SIE_Entry({ "#VER","?","??",cratchit_event.event_date,cratchit_event.seed_label,"_?today?_" }));
						statement.push_back(SIE_Entry({ "{" }));

						// Direct Payment (Increase Cost, Decerase Asset)
						statement.push_back(TRANS_increase_cost_entry(6230, cratchit_event.event_amount, seed_label));// SIE_Entry({ "#TRANS","6230","{}",cratchit_event.event_amount,"","Beanstalk" }));
						statement.push_back(TRANS_decrease_asset_entry(1920, cratchit_event.event_amount,seed_label));// SIE_Entry({ "#TRANS","1920","{}",cratchit_event.event_amount,"","Beanstalk" }));

						statement.push_back(SIE_Entry({ "}" }));

						statements.push_back(statement);
					}
				}
				else if (babel::keyword("Telia").matches(seed_label)) {
					/*
					#VER	4	42	20151116	"FA407/Telia"	20160711
					{
					#TRANS	2440	{}	-1330,00	""	"FA407/Telia"
					#TRANS	2640	{}	265,94	""	"FA407/Telia"
					#TRANS	6212	{}	1064,06	""	"FA407/Telia"
					}
					*/

					/*
					#VER	5	41	20151216	"BE"	20160701
					{
					#TRANS	1920	{}	-1330,00	""	"BE"
					#TRANS	2440	{}	1330,00	""	"BE/FA407/Telia"
					}
					*/
					{
						//SIE_Statement statement;
						//
						//statement.push_back(SIE_Entry({ "#VER","4","42","20160517","FA407/Telia","20160711" }));
						//statement.push_back(SIE_Entry({ "{" }));
						//statement.push_back(SIE_Entry({ "#TRANS","2440","{}",event_amount,"","FA407/Telia" }));
						//statement.push_back(SIE_Entry({ "#TRANS","2640","{}","_?Event_VAT_amount?_","","FA407/Telia" }));
						//statement.push_back(SIE_Entry({ "#TRANS","6212","{}","_?Event_No_VAT_amount?_","","FA407/Telia" }));
						//statement.push_back(SIE_Entry({ "}" }));

						//statements.push_back(statement);
					}
					{
						SIE_Statement statement;
						cratchit::event::Cratchit_Event cratchit_event(sie_seed);
						VAT_Record vat_record = Amount_With_VAT_View(cratchit_event.event_amount);

						statement.push_back(VER_Statement(cratchit_event.event_date, cratchit_event.seed_label));
						statement.push_back(SIE_Entry({ "{" }));

						// Generate Invoice Entry (Increase Cost, Increase Debt)
						statement.push_back(TRANS_increase_debt_entry(2440, vat_record.amount_with_VAT,cratchit_event.seed_label)); // { "#TRANS","2440","{}",event_amount,"","FA407/Telia" }));
						statement.push_back(TRANS_increase_debt_entry(2640, vat_record.VAT_Amount, cratchit_event.seed_label)); // { "#TRANS","2640","{}","_?Event_VAT_amount?_","","FA407/Telia" }));
						statement.push_back(TRANS_increase_cost_entry(6212, vat_record.amount_without_VAT,cratchit_event.seed_label)); // SIE_Entry({ "#TRANS","6212","{}","_?Event_No_VAT_amount?_","","FA407/Telia" }));

						statement.push_back(SIE_Entry({ "}" }));
						statements.push_back(statement);
					}
					{
						SIE_Statement statement;
						cratchit::event::Cratchit_Event cratchit_event(sie_seed);
						statement.push_back(VER_Statement(cratchit_event.event_date, cratchit_event.seed_label));
						statement.push_back(SIE_Entry({ "{" }));

						// Generate Payment Entry (Decerase Debt, Decrease Asset)
						statement.push_back(TRANS_decrease_debt_entry(2440, cratchit_event.event_amount, cratchit_event.seed_label));// #TRANS	2440	{}	1330, 00	""	"BE/FA407/Telia"
						statement.push_back(TRANS_decrease_asset_entry(1920, cratchit_event.event_amount, cratchit_event.seed_label)); // #TRANS	1920	{}	-1330, 00	""	"BE"


						statement.push_back(SIE_Entry({ "}" }));
						statements.push_back(statement);

					}
				}
				else {
					// Unknown event
				}

				std::for_each(std::begin(statements), std::end(statements), [&result](const SIE_Statement& statement) {
					{
						// Header
						result.push_back(SIE_Entry({ "#FNR","ITFIED" }));
						result.push_back(SIE_Entry({ "#FNAMN","The ITfied AB" }));
						result.push_back(SIE_Entry({ "#ORGNR","556782-8172" }));
						result.push_back(SIE_Entry({ "#RAR","0","20160501","20170430" }));
						result.push_back(SIE_Entry({ "" }));
					}
					// statemenet
					std::copy(std::begin(statement), std::end(statement), std::back_inserter(result));
				});

				//if (statement.size() > 0) {
				//	// Create result with Header
				//	{
				//		// Header
				//		result.push_back(SIE_Entry({ "#FNR","ITFIED" }));
				//		result.push_back(SIE_Entry({ "#FNAMN","The ITfied AB" }));
				//		result.push_back(SIE_Entry({ "#ORGNR","556782-8172" }));
				//		result.push_back(SIE_Entry({ "#RAR","0","20160501","20170430" }));
				//		result.push_back(SIE_Entry({ "" }));
				//	}
				//	// statemenet
				//	std::copy(std::begin(statement), std::end(statement), std::back_inserter(result));
				//}
			}
			return result;
		}
	}
}

/*
==> Consider to propose an SIE-structure-key-paths for an SIE-file Composite (http://www.sie.se/?page_id=250)?

// Note: Keys spelled in capital letters corresponds to SIE-encoding of #-element (e.g., sie.ADRESS corresponds to SIE file member #ADRESS)
// Note: In key-path the key name is the SIE Element name with spaces replaced with �_� (e.g., SIE Element �distribution address� becomes key �distribution_address�
// Note: Elements that implicitly form a list is assigned a list key [*] (the �*� indicates a possible index 0�n)
// Note: Member element with sub-elements {dimension_no object_no} has been assigned the key �object� (to correspond to referred OBJECT element)
// Note: Specified #RES member �year�s� has bee assigned the key �year_no�
// Note: Specified TRANS member element �object list� has been assigned the list key �objects[]� each conforming to key object (see specification 8.21)

sie.ADRESS.contact
sie.ADRESS.distribution_address
sie.ADRESS.postal_address
sie.ADRESS.tel

sie.BKOD.SNI_code

sie.DIM[*].dimension_no
sie.DIM[*].name

sie.ENHET[*].account_no
sie.ENHET[*].unit

sie.FLAGGA.x

sie.FNAMN.company_name

sie.FNR.company_id

sie.FORMAT
sie.FTYP

sie.GEN.date
sie.GEN.sign

sie.IB.year_no
sie.IB.account
sie.IB.balance
sie.IB.quantity	// optional

sie.KONTO[*].account_no
sie.KONTO[*].account_name

sie.KPTYP

sie.KSUMMA[0�1]

sie.KTYP.account_no
sie.KTYP.account_type

sie.OBJEKT[*].dimension_no
sie.OBJEKT[*].object_no
sie.OBJEKT[*].object_name

sie.OIB[*].year_no
sie.OIB[*].account
sie.OIB[*].object.dimension_no
sie.OIB[*].object.object_no
sie.OIB[*].balance
sie.OIB[*].quantity // optional

sie.OMFATTN

sie.ORGNR.CIN
sie.ORGNR.acq_no
sie.ORGNR.act_no

sie.OUB[*].year_no
sie.OUB[*].account
sie.OUB[*].object.dimension_no
sie.OUB[*].object.object_no
sie.OUB[*].balance
sie.OUB[*].quantity // optional

sie.PBUDGET[*].year_no
sie.PBUDGET[*].period
sie.PBUDGET[*].account
sie.PBUDGET[*].object.dimension_no
sie.PBUDGET[*].object.object_no
sie.PBUDGET[*].balance
sie.PBUDGET[*].quantity

sie.PROGRAM.program_name
sie.PROGRAM.version

sie.PROSA

sie.PSALDO[*].year_no
sie.PSALDO[*].period
sie.PSALDO[*].account
sie.PSALDO[*].object.dimension_no
sie.PSALDO[*].object.object_no
sie.PSALDO[*].balance

sie.RAR[*].year_no
sie.RAR[*].start
sie.RAR[*].end

sie.RES[*].year_no
sie.RES[*].account
sie.RES[*].balance
sie.RES[*].quantity

sie.SIETYP

sie.SRU[*].account
sie.SRU[*].SRU_code

sie.TAXAR

sie.UB[*].year_no
sie.UB[*].account
sie.UB[*].balance
sie.UB[*].quantity

sie.UNDERDIM[*].dimension_no
sie.UNDERDIM[*].name
sie.UNDERDIM[*].superdimension

sie.VALUTA

sie.VER[*].series
sie.VER[*].verno
sie.VER[*].verdate
sie.VER[*].vertext // optional presence
sie.VER[*].regdate
sie.VER[*].sign

sie.VER[*].RTRANS[*].account_no
sie.VER[*].RTRANS[*].objects[*].dimension_no
sie.VER[*].RTRANS[*].objects[*].object_no
sie.VER[*].RTRANS[*].amount
sie.VER[*].RTRANS[*].transdate // optional
sie.VER[*].RTRANS[*].transtext // optional
sie.VER[*].RTRANS[*].quantity // optional presence
sie.VER[*].RTRANS[*].sign

sie.VER[*].TRANS[*].account_no
sie.VER[*].TRANS[*].objects[*].dimension_no
sie.VER[*].TRANS[*].objects[*].object_no
sie.VER[*].TRANS[*].amount
sie.VER[*].TRANS[*].transdate // optional
sie.VER[*].TRANS[*].transtext // optional
sie.VER[*].TRANS[*].quantity // optional presence
sie.VER[*].TRANS[*].sign

sie.VER[*].BTRANS[*].account_no
sie.VER[*].BTRANS[*].objects[*].dimension_no
sie.VER[*].BTRANS[*].objects[*].object_no
sie.VER[*].BTRANS[*].amount
sie.VER[*].BTRANS[*].transdate // optional
sie.VER[*].BTRANS[*].transtext // optional
sie.VER[*].BTRANS[*].quantity // optional presence
sie.VER[*].BTRANS[*].sign

*/

/*
-2) �verv�g att ta reda p� hur jag skall bokf�ra de verifikationer jag �nnu inte skickat till SE-konsult?

==> Faktura 30871 SE-konsult

==> Tidigare faktura har bokf�rts s� h�r (Verifikat D:57 kodas som #VER	4	57)
==> Kollade flera stycken och alla verkar se ut s� h�r?

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	4	57	20160229	"FA422/SE Konsult"	20160711
{
#TRANS	2440	{}	-3325,00	""	"FA422/SE Konsult"
#TRANS	2640	{}	665,00	""	"FA422/SE Konsult"
#TRANS	6530	{}	2660,00	""	"FA422/SE Konsult"
}

==> Konton

6530	Redovisningstj�nster
2440	Leverant�rsskulder
2640	Ing�ende moms

==> Och betalning s� h�r (Verifikat E:54 kodas som #VER	5	54)

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	5	54	20160318	"BE"	20160701
{
#TRANS	1920	{}	-3325,00	""	"BE"
#TRANS	2440	{}	3325,00	""	"BE/FA422/SE Konsult"
}

==> Konton

1920	PlusGiro
2440	Leverant�rsskulder

==> Skattekonto Maj 2016 (inklusive periodmoms)

==> Q: Hur bokf�ra EU-moms som ing�r just denna g�ng?
==> A: Bokf�r betalning som vanlig SKV inbet med moms (momskonto 2650 mot inbet skattekonto 1630)?
Ex:
#TRANS	2650	{}	-9418,00	""	"Moms okt-dec"
#TRANS	1630	{}	9418,00	""	"Moms okt-dec"

==> A: EU-ink�pet och momsen verkar vara bokf�rd s� h�r (Verifikat A 91 kodat som #VER	1	91)

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	1	91	20160219	"Digicert"	20160711
{
#TRANS	1920	{}	-3956,13	""	"Digicert"
#TRANS	6230	{}	3956,13	""	"Digicert"
#TRANS	2645	{}	989,00	""	"Digicert"
#TRANS	2615	{}	-989,00	""	"Digicert"
}

==> Konton
1920	PlusGiro
6230	Datakommunikation
2645	Ber ing moms p� f�rv�rv fr�n u
2615	Ber utg moms varuf�rv�rv EU-la
==> Notera att EU-momsen bokf�rts i balans b�de som ing�ende (2645) och utg�ende (2615)?

==> Denna moms verkar ha �t�mts� vid periodslutet jan-mars och sammanst�llts p� 2650 (Verifikat A 108 kodat som #VER	1	108)

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	1	108	20160401	"Momst�mning jan - mars"	20160711
{
#TRANS	2610	{}	28875,00	""	"Momst�mning jan - mars"
#TRANS	2615	{}	989,00	""	"Momst�mning jan - mars"
#TRANS	2640	{}	-6562,04	""	"Momst�mning jan - mars"
#TRANS	2645	{}	-989,00	""	"Momst�mning jan - mars"
#TRANS	2650	{}	-22313,00	""	"Momst�mning jan - mars"
#TRANS	3740	{}	0,04	""	"Momst�mning jan - mars"
}

==> Konton:

2640	Ing�ende moms
2610	Utg�ende moms 25%
2650	Redovisningskonto f�r moms

2615	Ber utg moms varuf�rv�rv EU-la
2645	Ber ing moms p� f�rv�rv fr�n u
3740	�res- och kronutj�mning

==> Not: 2650 inkluderar EU-momsen vilket indikerar att vid betalning beh�ver inte EU-moms-konton p�verkas?

==> Tidigare momst�mning med EU-moms har bokf�rts 140701 apr-jun (Verifikat A:42 kodat som #VER	1	42)

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20140501	20150430

#VER	1	42	20140701	"Momsomf�ring april-juni"	20160711
{
#TRANS	2610	{}	130499,00	""	"Momsomf�ring april-juni"
#TRANS	2615	{}	-1135,74	""	"Momsomf�ring april-juni"
#TRANS	2640	{}	-11120,00	""	"Momsomf�ring april-juni"
#TRANS	2645	{}	1135,74	""	"Momsomf�ring april-juni"
#TRANS	2650	{}	-119379,00	""	"Momsomf�ring april-juni"
}

==> Och inbetalning inklusive periodmoms och EU-moms bokf�rdes (F�RSENAD) s� h�r

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20140501	20150430

#VER	1	48	20141013	"Inbet. SKV"	20160711
{
#TRANS	1920	{}	-32000,00	""	"Inbet. SKV"
#TRANS	1630	{}	32000,00	""	"Inbet. SKV"
#TRANS	1630	{}	-500,00	""	"F�rseningsavgift"
#TRANS	6992	{}	500,00	""	"F�rseningsavgift"
#TRANS	2710	{}	10269,00	""	"Pers.skatt"
#TRANS	1630	{}	-10269,00	""	"Pers.skatt"
#TRANS	2730	{}	12253,00	""	"AGA sept"
#TRANS	1630	{}	-12253,00	""	"AGA sept"
#TRANS	2510	{}	8786,00	""	"Prel.skatt"
#TRANS	1630	{}	-8786,00	""	"Prel.skatt"
#TRANS	1630	{}	4,00	""	"R�nta"
#TRANS	8314	{}	-4,00	""	"R�nta"
}

==> Not: Verkar INTE ber�ra n�gra EU-konton?
==> Not: Hittar inga EU-momskonton f�r tidigare �r.

==> Tidigare momst�mning okt-dec har bokf�rts s� h�r (Verifikat A 82 kodat som #VER	1	82)

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	1	82	20160104	"�rspris kort"	20160711
{
#TRANS	1920	{}	-144,00	""	"�rspris kort"
#TRANS	6570	{}	144,00	""	"�rspris kort"
#TRANS	2640	{}	-12568,00	""	"momssaldo okt-dec15"
#TRANS	2610	{}	3150,00	""	"momssaldo okt-dec15"
#TRANS	2650	{}	9418,00	""	"momssaldo okt-dec15"
}

==> Konton, exklusive "�rspris kort" (j�mf�r momst�mning med EU-moms A:108 ovan):

2640	Ing�ende moms
2610	Utg�ende moms 25%
2650	Redovisningskonto f�r moms

==> Tidigare redovisad inbet. SKV inklusive periodmoms okt-dec har bokf�rts s� h�r (Verifikat A 89 kodat som #VER	1	89)

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	1	89	20160212	"Inbet t SKV"	20160701
{
#TRANS	1920	{}	-23838,00	""	"Inbet t SKV"
#TRANS	1630	{}	23838,00	""	"Inbet t SKV"
#TRANS	2650	{}	-9418,00	""	"Moms okt-dec"
#TRANS	1630	{}	9418,00	""	"Moms okt-dec"
#TRANS	2730	{}	12568,00	""	"AGA jan"
#TRANS	1630	{}	-12568,00	""	"AGA jan"
#TRANS	2710	{}	11270,00	""	"Avdr.skatt jan"
#TRANS	1630	{}	-11270,00	""	"Avdr.skatt jan"
}

==> Konton

1920	PlusGiro
1630	Skattekonto
2730	Lagstadgade sociala avgifter/l
2710	Personalskatt

2650	Redovisningskonto f�r moms

==> Telia Faktura 160517

==> Tidigare Telia-faktura verkar vara bokf�rd s� h�r (Verifikat D:42 kodad som #VER	4	42)

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	4	42	20151116	"FA407/Telia"	20160711
{
#TRANS	2440	{}	-1330,00	""	"FA407/Telia"
#TRANS	2640	{}	265,94	""	"FA407/Telia"
#TRANS	6212	{}	1064,06	""	"FA407/Telia"
}

==> Konton

2440	Leverant�rsskulder
2640	Ing�ende moms
6212	Mobiltelefon

==> Och betalning verkar vara bokf�rd s� h�r (Verifikat E:41 kodat som #VER	5	41)

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	5	41	20151216	"BE"	20160701
{
#TRANS	1920	{}	-1330,00	""	"BE"
#TRANS	2440	{}	1330,00	""	"BE/FA407/Telia"
}

==> Konton

1920	PlusGiro
2440	Leverant�rsskulder

==> Privat kostnad iTunes 17 Maj

==> Detta verkar vara en tidigare iTunes som bokf�rts privat (Verifikat A:2 kodat som #VER	1	2)

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	1	2	20150506	"ITunes"	20160711
{
#TRANS	1920	{}	-49,00	""	"ITunes"
#TRANS	2893	{}	49,00	""	"ITunes"
}

==> Konton

1920	PlusGiro
2893	Skulder n�rst�ende personer, k

==> Skattekonto Juni 2016 (ej periodmoms)

==> Tidigare redovisad inbet SKV har bokf�rts s� h�r (Verifikat A 111 kodat som #VER	1	111)
==> Kollade n�gra andra SKV-verifikat och alla �vanliga� (L�neskatt och avg) verkar se ut s� h�r.

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	1	111	20160414	"Inbet. till SKV"	20160711
{
#TRANS	1920	{}	-23838,00	""	"Inbet. till SKV"
#TRANS	1630	{}	23838,00	""	"Inbet. till SKV"
#TRANS	2730	{}	12568,00	""	"AGA mars"
#TRANS	1630	{}	-12568,00	""	"AGA mars"
#TRANS	2710	{}	11270,00	""	"Avdr. skatt mars"
#TRANS	1630	{}	-11270,00	""	"Avdr. skatt mars"
}

==> Konton

1920	PlusGiro
1630	Skattekonto
2730	Lagstadgade sociala avgifter/l
2710	Personalskatt

==> Verkar som vi skall g�ra momst�mning f�r apr-jun den 1 Juli?

==> Tidigare bokf�ring verkar g�ra en momst�mning f�r varje avslutat momsperiod (t.ex okt-dec 2015)


#TRANS	2640	{}	-12568,00	""	"momssaldo okt-dec15"
#TRANS	2610	{}	3150,00	""	"momssaldo okt-dec15"
#TRANS	2650	{}	9418,00	""	"momssaldo okt-dec15"

2640	Ing�ende moms
2610	Utg�ende moms 25%
2650	Redovisningskonto f�r moms


==> Faktura Binero 160529

==> Tidigare Bindero-faktura p� 702 SEK �r bokf�rd s� h�r (Verifikat D:16 kodat som #VER	4	16)

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	4	16	20150828	"FA381/Binero"	20160701
{
#TRANS	2440	{}	-702,00	""	"FA381/Binero"
#TRANS	2640	{}	140,00	""	"FA381/Binero"
#TRANS	6230	{}	562,00	""	"FA381/Binero"
}

==> Konton

2440	Leverant�rsskulder
2640	Ing�ende moms
6230	Datakommunikation

==> Not: ALLA Binero-fakturor verkar bokas som datakommunikation (�ven dom�ner)?
==> Not: Jag borde egentligen boka dom�ner p� n�got annat?

==> Transaktioner PG-konto Maj 2016

==> Betalt Telia-faktura
==> Betalt Beanstalk
==> Utbet L�n
==> iTunes (redan bokf�rd ovan)
==> Betalt SE-faktura
==> SKV inbet (redan bokf�rd ovan)?
==> Pris enligt spec.
==> Betalt Beanstalk

==> Transaktioner PG-konto Juni 2016

==> Se pdf.

==> Faktura SE-konsult 160630

==> Se ovan

Verkar vara alla f�r nu? ...


-1) �verv�g att dokumentera de konton som dokumenterade verifikationer anv�nder?

==> Se bifogad excel-file �ITFIED - Kontoplan - 160711 190841� som Edison rapporterar som den aktuella kontoplanen?

2893 "Skulder n�rst�ende personer, k"
4010 �Ink�p varor och material� verkar anv�ndas till alla m�jliga ink�p?

1) �verv�g att dokumentera vanliga verifikationer i den �l�pande bokf�ringen� (Serie A)

==> Se bifogad excel-fil �ITFIED - Verifikatlista - 160711 185592.xlsx� f�r huvudbok Serie A �L�pande bokf�ring�




==> Detta verkar vara skickad kund-faktura till Swedbank

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	1	113	20160430	"Kundfaktura 83 3/11"	20160711
{
#TRANS	1790	{}	57273,00	""	"Kundfaktura 83 3/11"
#TRANS	3590	{}	-57273,00	""	"Kundfaktura 83 3/11"
}

==> Detta verkar vara en betalning som �r privat

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	1	2	20150506	"ITunes"	20160711
{
#TRANS	1920	{}	-49,00	""	"ITunes"
#TRANS	2893	{}	49,00	""	"ITunes"
}

==> Detta verkar vara ett kvitto p� Kjell&Compandy betalt med kort

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	1	104	20160323	"Kjell & Company"	20160711
{
#TRANS	1920	{}	-648,90	""	"Kjell & Company"
#TRANS	2640	{}	130,00	""	"Kjell & Company"
#TRANS	5410	{}	519,00	""	"Kjell & Company"
#TRANS	3740	{}	-0,10	""	"Kjell & Company"
}

*/
