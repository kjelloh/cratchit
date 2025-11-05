#include "zeroth/main.hpp"

// Cpp file to isolate this 'zeroth' variant of cratchin until refactored to 'next' variant
// (This whole file conatins the 'zeroth' version of cratchit)
// NOTE: Yes, originally zeroth cratchit was one sinle cpp-file :o ... :) 

Amount get_INK1_Income(Model const& model) {
	Amount result{};
	if (auto amount = model->sru["0"].at(1000)) {
		std::istringstream is{*amount};
		is >> result;
	}
	return result;
}
Amount get_K10_Dividend(Model const& model) {
	Amount result{};
	if (auto amount = model->sru["0"].at(4504)) {
		// use amount assigned to SRU 4504
		std::istringstream is{*amount};
		is >> result;
	}
	else if (auto amount = account_sum(model->sie_env_map["current"],2898)) {
		// Use any amount accounted for in 2898
		result = *amount;
	}
	return result;
}

struct IBPeriodUB {
  CentsAmount ib{};
  CentsAmount period{};
  CentsAmount ub{};
};

using IBPeriodUBMap = std::map<BAS::AccountNo,IBPeriodUB>;

IBPeriodUBMap to_ib_period_ub(Model const& model,std::string relative_year_key) {
  IBPeriodUBMap result{};
  if (model->sie_env_map.contains(relative_year_key)) {
    auto financial_year_date_range = model->to_financial_year_date_range(relative_year_key);
    if (financial_year_date_range) {
      std::map<BAS::AccountNo,Amount> opening_balances = model->sie_env_map[relative_year_key].opening_balances();
      auto financial_year_tagged_amounts = model->all_dotas.date_range_tagged_amounts(*financial_year_date_range);
      auto bas_account_accs = tas::to_bas_omslutning(financial_year_tagged_amounts);
      for (auto const& ta : bas_account_accs) {
        IBPeriodUB entry{};
        std::string bas_account_string = ta.tags().at("BAS");
        auto bas_account_no = *BAS::to_account_no(bas_account_string);
        if (opening_balances.contains(bas_account_no)) {
          entry.ib = to_cents_amount(opening_balances.at(bas_account_no));
        }
        entry.period = ta.cents_amount();
        entry.ub = entry.ib + entry.period;
        result[bas_account_no] = entry;
        opening_balances.erase(bas_account_no); // consumed
      }
      for (auto const& [bas_account_no,opening_balance] : opening_balances) {
        IBPeriodUB entry{};
        std::string bas_account_string = std::to_string(bas_account_no);
        entry.ib = to_cents_amount(opening_balance);
        entry.ub = entry.ib;
        result[bas_account_no] = entry;
      }
    }
    else {
      std::cerr << "\nto_omslutning failed to create financial_year_date_range for relative_year_key:" << std::quoted(relative_year_key);
    }
  }
  else {
    std::cerr << "\nto_omslutning failed. No SIE environment for relative_year_key:" << std::quoted(relative_year_key);
  }
  return result;
}

namespace SKV {
	namespace SRU {

		OptionalSRUValueMap to_sru_value_map(Model const& model,std::string year_index,::CSV::FieldRows const& field_rows) {
			OptionalSRUValueMap result{};
      std::map<SKV::SRU::AccountNo,int> sru_value_aggregate_sign{};
			try {
        if (!model->sie_env_map.contains(year_index)) {
          logger::cout_proxy << "\nSorry, to_sru_value_map failed. No SIE environment found for year index:" << year_index;
          return result;
        }
        logger::cout_proxy << "\nto_sru_value_map";

				std::map<SKV::SRU::AccountNo,BAS::OptionalAccountNos> sru_to_bas_accounts{};
				for (int i=0;i<field_rows.size();++i) {
					auto const& field_row = field_rows[i];
          logger::cout_proxy << "\n\t" << static_cast<std::string>(field_row);
					if (field_row.size() > 1) {
						auto const& field_1 = field_row[1];
            logger::cout_proxy << "\n\t\t[1]=" << std::quoted(field_1);
						if (auto sru_code = to_account_no(field_1)) {
              logger::cout_proxy << " ok! ";
							if (field_row.size() > 4) {
								auto mandatory = (field_row[3].find("J") != std::string::npos);
								if (mandatory) {
                  logger::cout_proxy << " Mandatory.";
								}
								else {
                  logger::cout_proxy << " optional ;)";
								}

                // Experimental: It seems this specification just defines how to aggregate this value.
                //               A '+' means add it, and '-' means subtract it?
                //               The value itself should be positive if we kept the books in order
                //               and flips the sign according to BAS account category (See below)?
                //               A '*' seems to mean it does not take part in any expressions (so dont care)?
                sru_value_aggregate_sign[*sru_code] = (field_row[4].find("-") != std::string::npos)?-1:1;

                sru_to_bas_accounts[*sru_code] = model->sie_env_map[year_index].to_bas_accounts(*sru_code);
                logger::cout_proxy << "\nSRU:" << *sru_code << " BAS count: " << sru_to_bas_accounts[*sru_code]->size();

							}
							else {
                logger::cout_proxy << " NO [3]";
							}
						}
						else {
              logger::cout_proxy << " NOT SRU";
						}
					}
					else {
            logger::cout_proxy << " null (does not exist)";
					}
				}

        auto ib_period_ub = to_ib_period_ub(model,year_index);

				// Now retreive the sru values from bas accounts as mapped
				SRUValueMap sru_value_map{};
				for (auto const& [sru_code,bas_account_nos] : sru_to_bas_accounts) {
          logger::cout_proxy << "\nSRU:" << sru_code;
					if (bas_account_nos) {

            CentsAmount acc{};
            int sign_adjust_factor{1};

            for (auto const& bas_account_no : *bas_account_nos) {

              // Experimental: We need to adjust the sign of values of SRU to match the form.
              // After some thinking I have this theory.
              // The Assets and Liabilities are entered into the form with signes flipped to
              // make BAS Debit Assets AND BAS Credit liabilities is positive in 'SRU form'.
              // This means we flip the sign on BAS liabilites to adjust for this.
              // The same goes for Revenues that we flip the sign on to get positive values for SRU
              // Expenses are positive in BAS so no sign flipping there.
              // And
              switch (bas_account_no / 1000) {
                case 1: sign_adjust_factor = 1; break; // Assets are positive in BAS ok
                case 2: sign_adjust_factor = -1; break; // Liabilities are negative in BAS = flip sign
                case 3: sign_adjust_factor = -1; break; // Revenues are negative in BAS = flip sign
                case 4:
                case 5:
                case 6:
                case 7: sign_adjust_factor = 1; break; // Expenses are positive in BAS ok.
                case 8: {

                  // TODO: Consider to solve the problem with signs on 8xxx-accounts?
                  //       1) Hard code all SRU values on INK2R result form so they are all positive (abs())?
                  //       2) hard code / identify what 8xxx accounts that are cost accounts and flip the signs on those?
                  //       3) Other option?

                  // SRU values shall be positive for Revenuse (so flip BAS sign)
                  // Except for 'årets resultat' 8990..8999 where negative is a loss.
                  //        So we must not flip the sign of 8999 as we need to detect if we made a loss to
                  //        assign to SRU 7450 (gain) or 7550 (loss)
                  if (bas_account_no < 8990) sign_adjust_factor = -1;

                  // Dirty fix for BAS 84xx (SRU: 7522)
                  // The form aggregates this value with -. But a cost on 8xxx is + in BAS! To make this work we
                  // need to NOT flip the sign on this cost value to make SRU value psotive and form subtract it correctly!
                  // Note: BAS:8310 UB:-199807 for a revenue!
                  // Note: BAS:
                  // *sigh* - what a MESS!
                  if (bas_account_no >= 8400 and bas_account_no <= 8499) sign_adjust_factor = 1; // SRU:7522 positive for a cost
                }
                break;
              }


              logger::cout_proxy << "\n\tBAS:" << bas_account_no;
              if (ib_period_ub.contains(bas_account_no)) {
                acc += ib_period_ub[bas_account_no].ub;
                logger::cout_proxy << " UB:" << ib_period_ub[bas_account_no].ub;
                logger::cout_proxy << " Acc:" << acc;
              }
            }
            logger::cout_proxy << "\n\tsign_adjust_factor : " << sign_adjust_factor;
            logger::cout_proxy << "\n\t" << acc << " *= " << sign_adjust_factor << " = ";
            acc *= sign_adjust_factor; // Expect all amounts to SKV to be without sign
            logger::cout_proxy << acc;

            sru_value_map[sru_code] = to_string(acc);

            logger::cout_proxy << "\n\t------------------";
            logger::cout_proxy << "\n\tSUM:" << sru_code << " = ";
            if (sru_value_map[sru_code]) logger::cout_proxy << *sru_value_map[sru_code];
            else logger::cout_proxy << " null";

					}
					else {
            logger::cout_proxy << "\n\tNO BAS Accounts map to SRU:" << sru_code;
						if (auto const& stored_value = model->sru[year_index].at(sru_code)) {
							sru_value_map[sru_code] = stored_value;
              logger::cout_proxy << "\n\tstored:" << *stored_value;
						}
            else {
              // Mark as unknown
              // TODO: Skip if not mandatory
              // sru_value_map[sru_code] = std::format("?{}?",sru_code);
            }

						// // K10
						// SRU:4531	"Antal ägda andelar vid årets ingång"
						// SRU:4532	"Totala antalet andelar i hela företaget vid årets ingång"
						// SRU:4511	"Årets gränsbelopp enligt förenklingsregeln"				(=183 700 * ägar_andel)
						// SRU:4501	"Sparat utdelningsutrymme från föregående år * 103%)	(103% av förra årets sparade utrymme, förra årets SRU:4724)
						// SRU:4502	"Gränsbelopp enligt förenklingsregeln"					(SRU:4511 + SRU:4501
						// SRU:4503	"Gränsbelopp att utnyttja vid p. 1.7 nedan"				(SRU:4502)

						// // Utdelning som beskattas I TJÄNST"
						// SRU:4504	"Utdelning"										(Från BAS:2898, SRU:7369)
						// SRU:4721	"Gränsbelopp SRU:4503"
						// SRU:4722	"Sparat utdelningsutrymme"							(SRU:4504 - SRU:4721)
						// SRU:4724	"Sparat utdelningsutrymme till nästa år"					(SRU:4722)

						// // Utdelning som beskattas i KAPITAL
						// SRU:4506	"Utdelning"										(SRU:4504)
						// SRU:4507	"Utdelning i Kapital"
						// SRU:4508	Det minsta av (2/3 av gränsbelopp vs 2/3 av utdelning Kapital)
						// SRU:4509	Resterande utdelning (utdelning kapital - gränsbelopp om positivt annars 0)
						// SRU:4515	"Utdelning som tas upp i kapital"						(Till INK1 p. 7.7. SRU:1100)

						// // INK1
						// SRU:1000 	"1.1 Lön Förmåner, Sjukpenning mm"	(Från förtryckta uppgifter)
						// SRU:1100	"7.2 Ränteinkomster, utdelningar..."	(från K10 SRU:4515	"Utdelning som tas upp i kapital")

					}
				}
				result = sru_value_map;
			}
			catch (std::exception const& e) {
				logger::cout_proxy << "\nto_sru_value_map failed. Excpetion=" << std::quoted(e.what());
			}
			return result;
		}
	}
} // namespace SKV {


using Command = std::string;
struct Quit {};
struct Nop {};

using Msg = std::variant<Nop,Quit,Command>;
struct Cmd {std::optional<Msg> msg{};};
using Ux = std::vector<std::string>;

Cmd to_cmd(std::string const& user_input) {
	Cmd result{Nop{}};
	if (user_input == "quit" or user_input=="q") result.msg = Quit{};
	else result.msg = Command{user_input};
	return result;
}

class FilteredSIEEnvironment {
public:
	FilteredSIEEnvironment(SIEEnvironment const& sie_environment,BAS::MatchesMetaEntry matches_meta_entry)
		:  m_sie_environment{sie_environment}
			,m_matches_meta_entry{matches_meta_entry} {}

	void for_each(auto const& f) const {
		auto f_if_match = [this,&f](BAS::MDJournalEntry const& mdje){
			if (this->m_matches_meta_entry(mdje)) f(mdje);
		};
		for_each_md_journal_entry(m_sie_environment,f_if_match);
	}
private:
	SIEEnvironment const& m_sie_environment;
	BAS::MatchesMetaEntry m_matches_meta_entry{};
};

std::ostream& operator<<(std::ostream& os,FilteredSIEEnvironment const& filtered_sie_environment) {
	struct stream_entry_to {
		std::ostream& os;
		void operator()(BAS::MDJournalEntry const& me) const {
			os << '\n' << me;
		}
	};
	os << "\n*Filter BEGIN*";
	filtered_sie_environment.for_each(stream_entry_to{os});
	os << "\n*Filter end*";
	return os;
}

std::ostream& operator<<(std::ostream& os,SIEEnvironment const& sie_environment) {
  os << "\n" << "Financial Period:";
  if (auto maybe_year_range = sie_environment.financial_year_date_range()) {
    os << maybe_year_range.value();
  }
  else {
    os << "*anonymous*";
  }

	for (auto const& je : sie_environment.journals()) {
		auto& [series,journal] = je;
		for (auto const& [verno,entry] : journal) {
			BAS::MDJournalEntry mdje {
				.meta = {
					 .series = series
					,.verno = verno
					,.unposted_flag = sie_environment.is_unposted(series,verno)
				}
				,.defacto = entry
			};
			os << '\n' << mdje;
		}
	}
	return os;
}

// Nof in SIEEnvironmentFramework unit / 20251028
// BAS::MetaEntry to_entry(SIE::Ver const& ver) {

// Now in SIEEnvirnmentFramework unit / 20251028
// OptionalSIEEnvironment sie_from_sie_file(std::filesystem::path const& sie_file_path) {

void unposted_to_sie_file(SIEEnvironment const& sie,std::filesystem::path const& p) {
  logger::cout_proxy << "\nunposted_to_sie_file " << p;
	std::ofstream os{p};
	SIE::io::OStream sieos{os};
	auto now = std::chrono::system_clock::now();
	auto now_timet = std::chrono::system_clock::to_time_t(now);
	auto now_local = localtime(&now_timet);
	sieos.os << "#GEN " << std::put_time(now_local, "%Y%m%d");
  if (auto maybe_year_range = sie.financial_year_date_range()) {
    auto const& year_range = maybe_year_range.value();
    sieos.os << "\n#RAR" << " 0 " << year_range.begin() << " " << year_range.end();
  }
	for (auto const& entry : sie.unposted()) {
    logger::cout_proxy << "\nUnposted:" << entry;
		sieos << to_sie_t(entry);
	}
}

void unposted_to_sie_files(SIEEnvironmentsMap const& sie_env_map) {
  logger::development_trace("unposted_to_sie_files");
  for (auto const& [year_id,sie] : sie_env_map) {
    unposted_to_sie_file(sie,sie.staged_sie_file_path());
  }
}

std::vector<std::string> quoted_tokens(std::string const& cli) {
	// std::cout << "quoted_tokens count:" << cli.size();
	// for (char ch : cli) {
	// 	std::cout << " " << ch << ":" << static_cast<unsigned>(ch);
	// }
	std::vector<std::string> result{};
	std::istringstream in{cli};
	std::string token{};
	while (in >> std::quoted(token)) {
		// std::cout << "\nquoted token:" << token;
		result.push_back(token);
	}
	return result;
}

std::string prompt_line(PromptState const& prompt_state) {
	std::ostringstream prompt{};
	// prompt << options_list_of_prompt_state(prompt_state);
	prompt << "cratchit";
	switch (prompt_state) {
		case PromptState::Root: {
			prompt << ":";
		} break;
    case PromptState::LUARepl: {
			prompt << ":lua";
    } break;
		case PromptState::TAIndex: {
			prompt << ":tas";
		} break;
		case PromptState::AcceptNewTAs: {
			prompt << ":tas:accept";
		} break;
		case PromptState::HADIndex: {
			prompt << ":had";
		} break;
		case PromptState::VATReturnsFormIndex: {
			prompt << ":had:vat";
		} break;
		case PromptState::JEIndex: {
			prompt << ":had:je";
		} break;
		case PromptState::GrossDebitorCreditOption: {
			prompt << "had:aggregate:gross 0+or-:";
		} break;
		case PromptState::CounterTransactionsAggregateOption: {
			prompt << "had:aggregate:counter:";
		} break;
		case PromptState::GrossAccountInput: {
			prompt << "had:aggregate:counter:gross";
		} break;
		case PromptState::NetVATAccountInput: {
			prompt << "had:aggregate:counter:net+vat";
		} break;
		case PromptState::JEAggregateOptionIndex: {
			prompt << ":had:je:1*at";
		} break;
		case PromptState::EnterHA: {
			prompt << ":had:je:ha";
		} break;
		case PromptState::ATIndex: {
			prompt << ":had:je:at";
		} break;
		case PromptState::EditAT: {
			prompt << ":had:je:at:edit";
		} break;
		case PromptState::CounterAccountsEntry: {
			prompt << "had:je:cat";
		} break;
		case PromptState::SKVEntryIndex: {
			prompt << ":skv";
		} break;
		case PromptState::QuarterOptionIndex: {
			prompt << ":skv:tax_return:period";
		} break;
		case PromptState::SKVTaxReturnEntryIndex: {
			prompt << ":skv:tax_return";
		} break;
		case PromptState::K10INK1EditOptions: {
			prompt << ":skv:to_tax";
		} break;
		case PromptState::INK2AndAppendixEditOptions: {
			prompt << ":skv:ink2";
		} break;
		case PromptState::EnterIncome: {
			prompt << ":skv:income?";
		} break;
		case PromptState::EnterDividend: {
			prompt << ":skv:dividend?";
		} break;
		case PromptState::EnterContact: {
			prompt << ":contact";
		} break;
	  case PromptState::EnterEmployeeID: {
			prompt << ":employee";
		} break;
		default: {
			prompt << ":??";
		}
	}
	prompt << ">";
	return prompt.str();
}

std::string to_had_listing_prompt(HeadingAmountDateTransEntries const& hads) {
		// Prepare to Expose hads (Heading Amount Date transaction entries) to the user
		std::stringstream prompt{};
		unsigned int index{0};
		std::vector<std::string> sHads{};
		std::transform(hads.begin(),hads.end(),std::back_inserter(sHads),[&index](auto const& had){
			std::stringstream os{};
			os << index++ << " " << had;
			return os.str();
		});
		prompt << "\n" << std::accumulate(sHads.begin(),sHads.end(),std::string{"Please select:"},[](auto acc,std::string const& entry) {
			acc += "\n  " + entry;
			return acc;
		});
		return prompt.str();
}

std::optional<int> to_signed_ix(std::string const& s) {
	std::optional<int> result{};
	try {
		const std::regex signed_ix_regex("^\\s*[+-]?\\d+$"); // +/-ddd...
		if (std::regex_match(s,signed_ix_regex)) result = std::stoi(s);
	}
	catch (...) {}
	return result;
}

// cratchit lua-to-cratchit interface
namespace lua_faced_ifc {
  // "hidden" implementation details
  namespace detail {
    OptionalHeadingAmountDateTransEntry to_had(lua_State *L)
    {
      // Check if the argument is a table
      if (!lua_istable(L, -1))
      {
        luaL_error(L, "Invalid argument. Expected a table representing a heading amount date object.");
        return {};
      }

      HeadingAmountDateTransEntry had;

      // Get the 'heading' field
      lua_getfield(L, -1, "heading");
      if (!lua_isstring(L, -1))
      {
        luaL_error(L, "Invalid table. Expected a string for 'heading'.");
        lua_pop(L, 1); // remove value from stack
        return {};
      }
      had.heading = lua_tostring(L, -1);
      lua_pop(L, 1); // remove value from stack

      // Get the 'amount' field
      lua_getfield(L, -1, "amount");
      if (!lua_isnumber(L, -1))
      {
        luaL_error(L, "Invalid table. Expected a number for 'amount'.");
        lua_pop(L, 1); // remove value from stack
        return {};
      }
      had.amount = lua_tonumber(L, -1);
      lua_pop(L, 1); // remove value from stack

      // Get the 'date' field
      lua_getfield(L, -1, "date");
      if (!lua_isstring(L, -1))
      {
        luaL_error(L, "Invalid table. Expected a string for 'date'.");
        lua_pop(L, 1); // remove value from stack
        return {};
      }
      had.date = *to_date(lua_tostring(L, -1)); // assume success
      lua_pop(L, 1); // remove value from stack

      // ... (repeat for other fields)

      return had;
    }
    static int had_to_string(lua_State *L)
    {
      // expect a lua table on the stack representing a HeadingAmountDateTransEntry
      if (auto had = detail::to_had(L)) {
        std::stringstream os{};
        os << *had;
        lua_pushstring(L, os.str().c_str());
        return 1;
      }
      else {
        return luaL_error(L, "Invalid argument. Expected a table on the form {heading = '', amount = 0, date = ''}");
      }
    };
  }
  // Expect a single string on the lua stack on the form "<heading> <amount> <date>"
  static int to_had(lua_State *L) {
      // Check if the argument is a string
      if (!lua_isstring(L, 1)) {
          // If not, throw an error
          return luaL_error(L, "Invalid argument. Expected a string.");
      }

      // Get the string from the Lua stack
      const char *str = lua_tostring(L, 1);

      // Create a table
      lua_newtable(L);

      /*
      // lua table to represent a HeadingAmountDateTransEntry
      headingAmountDateTransEntry = {
          heading = "",
          amount = 0,  -- Assuming Amount is a numeric type
          date = "",  -- Assuming Date is a string
          optional = {
              series = nil,  -- Assuming char can be represented as a single character string
              gross_account_no = nil,  -- Assuming AccountNo can be represented as a string or number
              current_candidate = nil,  -- Assuming OptionalMetaEntry can be represented as a string or number
              counter_ats_producer = nil,  -- Assuming ToNetVatAccountTransactions can be represented as a string or number
              vat_returns_form_box_map_candidate = nil  -- Assuming FormBoxMap can be represented as a string or number
          }
      }
      */

      // For simplicity, let's assume the string is "key=value" format
      std::string command(str);
      auto tokens = tokenize::splits(command,tokenize::SplitOn::TextAmountAndDate);
      if (auto had = ::to_had(tokens)) {
          // Push the key
          lua_pushstring(L, "heading");
          // Push the value
          lua_pushstring(L, had->heading.c_str());
          // Set the table value
          lua_settable(L, -3);

          // Push the key
          lua_pushstring(L, "amount");
          // Push the value
          lua_pushnumber(L, to_double(had->amount));
          // Set the table value
          lua_settable(L, -3);

          // Push the key
          lua_pushstring(L, "date");
          // Push the value
          lua_pushstring(L, to_string(had->date).c_str());
          // Set the table value
          lua_settable(L, -3);

          // Push the key
          lua_pushstring(L, "optional");
          // Push the value
          lua_newtable(L);

          // 20250726 - optional.series removed from HAD
          // // Push the key
          // lua_pushstring(L, "series");
          // // Push the value
          // if (had->optional.series) {
          //   std::string series_str(1, *had->optional.series);
          //   lua_pushstring(L, series_str.c_str());
          // }
          // else lua_pushnil(L);
          // // Set the table value
          // lua_settable(L, -3);

          // Push the key
          lua_pushstring(L, "gross_account_no");
          // Push the value
          if (had->optional.gross_account_no) lua_pushstring(L, std::to_string(*had->optional.gross_account_no).c_str());
          else lua_pushnil(L);
          // Set the table value
          lua_settable(L, -3);

          // Push the key
          lua_pushstring(L, "current_candidate");
          // Push the value
          lua_pushnil(L);
          // Set the table value
          lua_settable(L, -3);

          // Push the key
          lua_pushstring(L, "counter_ats_producer");
          // Push the value
          lua_pushnil(L);
          // Set the table value
          lua_settable(L, -3);

          // Push the key
          lua_pushstring(L, "vat_returns_form_box_map_candidate");
          // Push the value
          lua_pushnil(L);
          // Set the table value
          lua_settable(L, -3);

          // Set the table value
          lua_settable(L, -3);
      }
      else {
          return luaL_error(L, "Invalid argument. Expected a string on the form '<heading> <amount> <date>'.");
      }

      // Create a new table to be used as the metatable
      lua_newtable(L);

      // Push the __tostring function onto the stack
      lua_pushstring(L, "__tostring");
      lua_pushcfunction(L, detail::had_to_string);
      lua_settable(L, -3);

      // Set the new table as the metatable of the original table
      lua_setmetatable(L, -2);

      // The table is already on the stack, so just return the number of results
      return 1;
  }

  void register_cratchit_functions(lua_State *L) {
    lua_pushcfunction(L, lua_faced_ifc::to_had);
    lua_setglobal(L, "to_had");
  }

} // namespace lua

// ==================================================
// *** class Updater declaration ***
// ==================================================
class Updater {
public:
	Model model;
	Cmd operator()(Command const& command);
	Cmd operator()(Quit const& quit);
  Cmd operator()(Nop const& nop);
private:
	BAS::TypedMetaEntries all_years_template_candidates(auto const& matches);
  std::pair<std::string,PromptState> transition_prompt_state(PromptState const& from_state,PromptState const& to_state);
};

// ==================================================
// *** class Updater Definition ***
// ==================================================
Cmd Updater::operator()(Command const& command) {
  // std::cout << "\noperator(command=" << std::quoted(command) << ")";
  std::ostringstream prompt{};
  auto ast = quoted_tokens(command);
  if (false) {
    // DEBUG, Trace tokens
    std::cout << "\ntokens:";
    std::ranges::copy(ast,std::ostream_iterator<std::string>(std::cout, ";"));
  }
  if (ast.size() == 0) {
    // User hit <Enter> with no input
    if (model->prompt_state == PromptState::TAIndex) {
      prompt << options_list_of_prompt_state(model->prompt_state);
      // List current selection
      prompt << "\n<SELECTED>";
      // for (auto const& ta : model->all_dotas.tagged_amounts()) {
      int index = 0;
      for (auto const& ta : model->selected_dotas.ordered_tagged_amounts()) {
        prompt << "\n" << index++ << ". " << ta;
      }
    }
    else if (model->prompt_state == PromptState::AcceptNewTAs) {
      // Reject new tagged amounts
      model->prompt_state = PromptState::TAIndex;
      prompt << "\n*Rejected*";
    }
    else if (model->prompt_state == PromptState::VATReturnsFormIndex) {
      // Assume the user wants to accept current Journal Entry Candidate
      if (auto had_iter = model->selected_had()) {
        auto& had = *(*had_iter);
        if (had.optional.current_candidate) {
          // We have a journal entry candidate - reset any VAT Returns form candidate for current had
          had.optional.vat_returns_form_box_map_candidate = std::nullopt;
          prompt << "VAT Consilidation Candidate " << *had.optional.current_candidate;
          model->prompt_state = PromptState::JEAggregateOptionIndex;
        }
      }
    }
    else {
      prompt << options_list_of_prompt_state(model->prompt_state);
    }
  }
  else {
    // We have at least one token in user input
    int signed_ix{};
    std::istringstream is{ast[0]};
    bool do_assign = (command.find('=') != std::string::npos);
    /* ======================================================


              Act on on Index?


      ====================================================== */
    if (auto signed_ix = to_signed_ix(ast[0]);
              do_assign == false
          and signed_ix
          and model->prompt_state != PromptState::EditAT
          and model->prompt_state != PromptState::EnterIncome
          and model->prompt_state != PromptState::EnterDividend) {
      // std::cout << "\nAct on ix = " << *signed_ix << " in state:" << static_cast<int>(model->prompt_state);
      size_t ix = abs(*signed_ix);
      bool do_remove = (ast[0][0] == '-');
      // Act on prompt state index input
      switch (model->prompt_state) {
        case PromptState::Root: {
        } break;
        case PromptState::LUARepl: {
          prompt << "\nSorry, a single number in LUA script state does nothing. Please enter a valid LUA expression";
        } break;
        case PromptState::TAIndex: {
          model->ta_index = ix;
          if (auto maybe_ta = model->selected_ta()) {
            auto& ta = maybe_ta.value();
            prompt << "\n" << ta;
          }
        } break;
        case PromptState::AcceptNewTAs: {
          switch (ix) {
            case 1: {
              // Accept the new tagged amounts created
              // model->all_dotas += model->new_dotas;
              // model->selected_dotas += model->new_dotas;
              model->all_dotas.dotas_insert_auto_ordered_container(model->new_dotas);
              model->selected_dotas.dotas_insert_auto_ordered_container(model->new_dotas);

              model->prompt_state = PromptState::TAIndex;
              prompt << "\n*Accepted*";
              prompt << "\n\n" << options_list_of_prompt_state(model->prompt_state);
            } break;
            default: {
              prompt << "\nPlease enter a valid option";
              prompt << "\n\n" << options_list_of_prompt_state(model->prompt_state);
            }
          }
        } break;
        case PromptState::HADIndex: {
          model->had_index = ix;
          // Note: Side effect = model->selected_had() uses model->had_index to return the corresponing had ref.
          if (auto had_iter = model->selected_had()) {
            auto& had = *(*had_iter);
            prompt << "\n" << had;
            bool do_prepend = (ast.size() == 4) and (ast[1] == "<--"); // Command <Index> "<--" <Heading> <Date>
            // TODO In state PromptState::HADIndex (-hads) allow command <Index> <Heading> | <Amount> | <Date> to easilly modify single property of selected had
            // Command: 4 "Betalning Faktura" modifies the heading of had with index 4
            // Command: 4 20240123 modifies the date of had with index 4
            // Command: 4 173,50 modifies the amount of had with index 4
            OptionalDate new_date = (ast.size() == 2) ? to_date(ast[1]) : std::nullopt;
            OptionalAmount new_amount = (ast.size() == 2 and not new_date) ? to_amount(ast[1]) : std::nullopt;
            std::optional<std::string> new_heading = (ast.size() == 2 and not new_amount and not new_date) ? std::optional<std::string>(ast[1]) : std::nullopt;
            if (do_remove) {
              model->heading_amount_date_entries.erase(*had_iter);
              prompt << " REMOVED";
              model->prompt_state = PromptState::Root;
            }
            else if (do_assign) {
              prompt << "\nSorry, ASSIGN not yet implemented for your input " << std::quoted(command);
            }
            else if (do_prepend) {
              std::cout << "\nprepend with heading:" << std::quoted(ast[2]) << " Date:" << ast[3];
              HeadingAmountDateTransEntry prepended_had {
                .heading = ast[2]
                ,.amount = abs(had.amount) // always an unsigned amount for prepended had
                ,.date = *to_date(ast[3]) // Assume success
              };
              model->heading_amount_date_entries.push_back(prepended_had);
              prompt << to_had_listing_prompt(model->refreshed_hads());
            }
            else if (new_heading) {
              had.heading = *new_heading;
              prompt << "\n  *new* --> " << had;
            }
            else if (new_amount) {
              had.amount = *new_amount;
              prompt << "\n  *new* --> " << had;
            }
            else if (new_date) {
              had.date = *new_date;
              prompt << "\n  *new* --> " << had;
            }
            else if (ast.size() == 4 and (ast[1] == "-initiated_as")) {
              // Command <Had Index> "-initiated_as" <Heading> <Date>
              if (auto init_date = to_date(ast[3])) {
                HeadingAmountDateTransEntry initiating_had {
                  .heading = ast[2]
                  ,.amount = abs(had.amount) // always an unsigned amount for initiating had
                  ,.date = *init_date
                };
                model->heading_amount_date_entries.push_back(initiating_had);
                prompt << to_had_listing_prompt(model->refreshed_hads());
              }
              else {
                prompt << "\nI failed to interpret " << std::quoted(ast[3]) << " as a date";
                prompt << "\nPlease enter a valid date for the event that intiated the indexed had (Syntax: <had Index> -initiated_as <Heading> <Date>)";
              }
            }
            else {
              // selected HAD and list template options
              if (had.optional.vat_returns_form_box_map_candidate) {
                // provide the user with the ability to edit the propsed VAT Returns form
                {
                  // Adjust the sum in box 49
                  had.optional.vat_returns_form_box_map_candidate->at(49).clear();
                  had.optional.vat_returns_form_box_map_candidate->at(49).push_back(SKV::XML::VATReturns::dummy_md_at(-SKV::XML::VATReturns::to_box_49_amount(*had.optional.vat_returns_form_box_map_candidate)));
                  for (auto const& [box_no,mats] : *had.optional.vat_returns_form_box_map_candidate)  {
                    prompt << "\n" << box_no << ": [" << box_no << "] = " << BAS::to_mdats_sum(mats);
                  }
                  BAS::MDJournalEntry mdje{
                    .meta = {
                      .series = 'M'
                    }
                    ,.defacto = {
                      .caption = had.heading
                      ,.date = had.date
                    }
                  };
                  std::map<BAS::AccountNo,Amount> account_amounts{};
                  for (auto const& [box_no,mats] : *had.optional.vat_returns_form_box_map_candidate)  {
                    for (auto const& mat : mats) {
                      account_amounts[mat.defacto.account_no] += mat.defacto.amount;
                      // std::cout << "\naccount_amounts[" << mat.defacto.account_no << "] += " << mat.defacto.amount;
                    }
                  }
                  for (auto const& [account_no,amount] : account_amounts) {
                    // account_amounts[0] = 4190.54
                    // account_amounts[2614] = -2364.4
                    // account_amounts[2640] = 2364.4
                    // account_amounts[2641] = 4190.54
                    // account_amounts[3308] = -888.1
                    // account_amounts[9021] = 11822

                    // std::cout << "\naccount_amounts[" << account_no << "] = " << amount;

                    // account_no == 0 is the dummy account for the VAT Returns form "sum" VAT
                    // Book this on BAS 1650 for now (1650 = "VAT to receive")
                    if (account_no==0) {
                      mdje.defacto.account_transactions.push_back({
                        .account_no = 1650
                        ,.amount = trunc(-amount)
                      });
                      mdje.defacto.account_transactions.push_back({
                        .account_no = 3740
                        ,.amount = BAS::to_cents_amount(-amount - trunc(-amount)) // to_tax(-amount) + diff = -amount
                      });
                    }
                    else {
                      mdje.defacto.account_transactions.push_back({
                        .account_no = account_no
                        ,.amount = BAS::to_cents_amount(-amount)
                      });
                    }
                    // Hard code reversal of VAT Returns report of EU Purchase (to have it not turn up on next report)
                    if (account_no == 9021) {
                      mdje.defacto.account_transactions.push_back({
                        .account_no = 9099
                        ,.transtext = "Avbokning (20) 9021"
                        ,.amount = BAS::to_cents_amount(amount)
                      });
                    }
                    // Hard code reversal of VAT Returns report of EU sales of services (to have it not turn up on next report)
                    if (account_no == 3308) {
                      mdje.defacto.account_transactions.push_back({
                        .account_no = 9099
                        ,.transtext = "Avbokning (39) 3308"
                        ,.amount = BAS::to_cents_amount(amount)
                      });
                    }
                  }
                  had.optional.current_candidate = mdje;

                  prompt << "\nCandidate: " << mdje;
                  model->prompt_state = PromptState::VATReturnsFormIndex;
                }
              }
              else if (had.optional.current_candidate) {
                prompt << "\n\t" << *had.optional.current_candidate;
                if (had.optional.counter_ats_producer) {
                  // We alreade have a "counter transactions" producer.
                  // Go directly to state for user to apply it to complete the candidate
                  model->prompt_state = PromptState::EnterHA;
                }
                else {
                  // The user has already selected a candidate
                  // But have not yet gone through assigning a "counter transaction" producer
                  model->prompt_state = PromptState::JEAggregateOptionIndex;
                }
              }
              else {
                // Selected HAD is "naked" (no candidate for book keeping assigned)
                model->had_index = ix;
                model->template_candidates = this->all_years_template_candidates([had](BAS::anonymous::JournalEntry const& aje){
                  return had_matches_trans(had,aje);
                });

                {
                  // hard code a template for Inventory gross + n x (ex_vat + vat) journal entry
                  // 0  *** "Amazon" 20210924
                  // 	"Leverantörsskuld":2440 "" -1489.66
                  // 	"Invenmtarier":1221 "Garmin Edge 130" 1185.93
                  // 	"Debiterad ingående moms":2641 "Garmin Edge 130" 303.73

                  // struct JournalEntry {
                  // 	std::string caption{};
                  // 	Date date{};
                  // 	AccountTransactions account_transactions;
                  // };

                  Amount amount{1000};
                  BAS::MDJournalEntry mdje{
                    .meta = {
                      .series = 'A'
                    }
                    ,.defacto = {
                      .caption = "Bestallning Inventarie"
                      ,.date = had.date
                    }
                  };
                  BAS::anonymous::AccountTransaction gross_at{
                    .account_no = 2440
                    ,.amount = -amount
                  };
                  BAS::anonymous::AccountTransaction net_at{
                    .account_no = 1221
                    ,.amount = static_cast<Amount>(amount*0.8f)
                  };
                  BAS::anonymous::AccountTransaction vat_at{
                    .account_no = 2641
                    ,.amount = static_cast<Amount>(amount*0.2f)
                  };
                  mdje.defacto.account_transactions.push_back(gross_at);
                  mdje.defacto.account_transactions.push_back(net_at);
                  mdje.defacto.account_transactions.push_back(vat_at);
                  model->template_candidates.push_back(to_typed_md_entry(mdje));
                }

                // List options for how the HAD may be registered into the books based on available candidates
                unsigned ix = 0;
                for (int i=0; i < model->template_candidates.size(); ++i) {
                  prompt << "\n    " << ix++ << " " << model->template_candidates[i];
                }
                model->prompt_state = PromptState::JEIndex;
              }
            }
          }
          else {
            prompt << "\nplease enter a valid index";
          }
        } break;
        case PromptState::VATReturnsFormIndex: {
          if (ast.size() > 0) {
            // Asume the user has selected an index for an entry on the proposed VAT Returns form to edit
            if (auto had_iter = model->selected_had()) {
              auto& had = *(*had_iter);
              if (had.optional.vat_returns_form_box_map_candidate) {
                if (had.optional.vat_returns_form_box_map_candidate->contains(ix)) {
                  auto box_no = ix;
                  auto& mats = had.optional.vat_returns_form_box_map_candidate->at(box_no);
                  if (auto amount = to_amount(ast[1]);amount and mats.size()>0) {
                    auto mats_sum = BAS::to_mdats_sum(mats);
                    auto sign = (mats_sum<0)?-1:1;
                    // mats_sum + diff = amount
                    auto diff = sign*(abs(*amount)) - mats_sum;
                    mats.push_back({
                      .defacto = {
                        .account_no = mats[0].defacto.account_no
                        ,.transtext = "diff"
                        ,.amount = diff
                      }
                    });
// std::cout << "\n[" << box_no << "]";
                    for (auto const& mat : mats) {
// std::cout << "\n\t" << mat;
                    }
// std::cout << "\n\t--------------------";
// std::cout << "\n\tsum " << BAS::mats_sum(mats);
                  }
                  else {
                    prompt << "\nPlease enter an entry index and a positive amount (will apply the sign required by the form)";
                  }
                }
                else {
                  prompt << "\nPlease enter a valid VAT Returns form entry index";
                  // provide the user with the ability to edit the propsed VAT Returns form
                }
                {
                  // Adjust the sum in box 49
                  had.optional.vat_returns_form_box_map_candidate->at(49).clear();
                  had.optional.vat_returns_form_box_map_candidate->at(49).push_back(SKV::XML::VATReturns::dummy_md_at(-SKV::XML::VATReturns::to_box_49_amount(*had.optional.vat_returns_form_box_map_candidate)));
                  for (auto const& [box_no,mats] : *had.optional.vat_returns_form_box_map_candidate)  {
                    prompt << "\n" << box_no << ": [" << box_no << "] = " << BAS::to_mdats_sum(mats);
                  }
                  BAS::MDJournalEntry mdje{
                    .meta = {
                      .series = 'M'
                    }
                    ,.defacto = {
                      .caption = had.heading
                      ,.date = had.date
                    }
                  };
                  std::map<BAS::AccountNo,Amount> account_amounts{};
                  for (auto const& [box_no,mats] : *had.optional.vat_returns_form_box_map_candidate)  {
                    for (auto const& mat : mats) {
                      account_amounts[mat.defacto.account_no] += mat.defacto.amount;
// std::cout << "\naccount_amounts[" << mat.defacto.account_no << "] += " << mat.defacto.amount;
                    }
                  }
                  for (auto const& [account_no,amount] : account_amounts) {
                    // account_amounts[0] = 4190.54
                    // account_amounts[2614] = -2364.4
                    // account_amounts[2640] = 2364.4
                    // account_amounts[2641] = 4190.54
                    // account_amounts[3308] = -888.1
                    // account_amounts[9021] = 11822
// std::cout << "\naccount_amounts[" << account_no << "] = " << amount;
                    // account_no == 0 is the dummy account for the VAT Returns form "sum" VAT
                    // Book this on BAS 2650
                    // NOTE: If "sum" is positive we could use 1650 (but 2650 is viable for both positive and negative VAT "debts")
                    if (account_no==0) {
                      mdje.defacto.account_transactions.push_back({
                        .account_no = 2650
                        ,.amount = trunc(-amount)
                      });
                      mdje.defacto.account_transactions.push_back({
                        .account_no = 3740
                        ,.amount = BAS::to_cents_amount(-amount - trunc(-amount)) // to_tax(-amount) + diff = -amount
                      });
                    }
                    else {
                      mdje.defacto.account_transactions.push_back({
                        .account_no = account_no
                        ,.amount = BAS::to_cents_amount(-amount)
                      });
                    }
                    // Hard code reversal of VAT Returns report of EU Purchase (to have it not turn up on next report)
                    if (account_no == 9021) {
                      mdje.defacto.account_transactions.push_back({
                        .account_no = 9099
                        ,.transtext = "Avbokning (20) 9021"
                        ,.amount = BAS::to_cents_amount(amount)
                      });
                    }
                    // Hard code reversal of VAT Returns report of EU sales of services (to have it not turn up on next report)
                    if (account_no == 3308) {
                      mdje.defacto.account_transactions.push_back({
                        .account_no = 9099
                        ,.transtext = "Avbokning (39) 3308"
                        ,.amount = BAS::to_cents_amount(amount)
                      });
                    }
                  }
                  had.optional.current_candidate = mdje;

                  prompt << "\nCandidate: " << mdje;
                  model->prompt_state = PromptState::VATReturnsFormIndex;
                }
              }
              else {
                prompt << "\nPlease re-enter a valid HAD and journal entry candidate (I seem to no longer have a valid VAT Returns form candidate to process)";
              }
            }
            else {
              prompt << "\nPlease re-enter a valid HAD index (It seems I have no recoprd of a selected HAD at the moment)";
            }
          }
        } break;
        case PromptState::JEIndex: {
          // Wait for user to choose how to create a journal entry from selected HAD
          if (auto had_iter = model->selected_had()) {
            // We have a selected HAD ok
            auto& had = *(*had_iter);
            if (auto account_no = BAS::to_account_no(command)) {
              // Assume user entered an account number for a Gross + 1..n <Ex vat, Vat> account entries
              BAS::MDJournalEntry mdje{
                .defacto = {
                    .caption = had.heading
                  ,.date = had.date
                }
              };
              mdje.defacto.account_transactions.emplace_back(BAS::anonymous::AccountTransaction{.account_no=*account_no,.amount=had.amount});
              had.optional.current_candidate = mdje;
              prompt << "\ncandidate:" << mdje;
              model->prompt_state = PromptState::GrossDebitorCreditOption;
            }
            else {
              // Assume user selected an entry index as base for a template
              auto tme_iter = model->template_candidates.begin();
              auto tme_end = model->template_candidates.end();
              auto at_iter = model->at_candidates.begin();
              auto at_end = model->at_candidates.end();
              if (ix < std::distance(tme_iter,tme_end)) {
                std::advance(tme_iter,ix);
                auto tme = *tme_iter;
                auto vat_type = to_vat_type(tme);
                std::cout << "\nvat_type = " << vat_type;
                switch (vat_type) {
                  case JournalEntryVATType::Undefined:
                    prompt << "\nSorry, I encountered Undefined VAT type for " << tme;
                    break; // *NOP*
                  case JournalEntryVATType::Unknown:
                    prompt << "\nSorry, I encountered Unknown VAT type for " << tme;
                    break; // *NOP*
                  case JournalEntryVATType::NoVAT: {
                    // No VAT in candidate.
                    // Continue with
                    // 1) Some proposed gross account transactions
                    // 2) a n x gross Counter aggregate

                    // NOTE 20240516 - A transfer to or from the SKV account is for now treated as a "No VAT" or "plain" transfer

                    auto tp = to_template(*tme_iter);
                    if (tp) {
                      auto mdje = to_md_journal_entry(had,*tp);
                      prompt << "\nPlain transfer " << mdje;
                      had.optional.current_candidate = mdje;
                      model->prompt_state = PromptState::JEAggregateOptionIndex;
                    }
                  } break;
                  case JournalEntryVATType::SwedishVAT: {
                    // Swedish VAT detcted in candidate.
                    // Continue with
                    // 2) a n x {net,vat} counter aggregate
                    auto tp = to_template(*tme_iter);
                    if (tp) {
                      auto mdje = to_md_journal_entry(had,*tp);
                      prompt << "\nSwedish VAT candidate " << mdje;
                      had.optional.current_candidate = mdje;
                      model->prompt_state = PromptState::JEAggregateOptionIndex;
                    }
                  } break;
                  case JournalEntryVATType::EUVAT: {
                    // EU VAT detected in candidate.
                    // Continue with a
                    // 2) n x gross counter aggregate + an EU VAT Returns "virtual" aggregate
                    // #1 hard code for EU VAT Candidate
                    BAS::MDJournalEntry mdje {
                      .defacto = {
                        .caption = had.heading
                        ,.date = had.date
                      }
                    };
                    for (auto const& [at,props] : tme_iter->defacto.account_transactions) {
                      if (props.contains("gross") or props.contains("eu_purchase")) {
                        int sign = (at.amount < 0)?-1:1; // 0 treated as +
                        BAS::anonymous::AccountTransaction new_at{
                          .account_no = at.account_no
                          ,.transtext = std::nullopt
                          ,.amount = sign*abs(had.amount)
                        };
                        mdje.defacto.account_transactions.push_back(new_at);
                      }
                      else if (props.contains("vat")) {
                        int sign = (at.amount < 0)?-1:1; // 0 treated as +
                        BAS::anonymous::AccountTransaction new_at{
                          .account_no = at.account_no
                          ,.transtext = std::nullopt
                          ,.amount = sign*abs(had.amount*0.2f)
                        };
                        mdje.defacto.account_transactions.push_back(new_at);
                        prompt << "\nNOTE: Assumed 25% VAT for " << new_at;
                      }
                    }
                    prompt << "\nEU VAT candidate " << mdje;
                    had.optional.current_candidate = mdje;
                    model->prompt_state = PromptState::JEAggregateOptionIndex;
                  } break;
                  case JournalEntryVATType::VATReturns: {
                    //  M2 "Momsrapport 2021-07-01 - 2021-09-30" 20210930
                    // 	 vat = sort_code: 0x6 : "Utgående moms, 25 %":2610 "" 83300
                    // 	 eu_vat vat = sort_code: 0x56 : "Utgående moms omvänd skattskyldighet, 25 %":2614 "" 1654.23
                    // 	 vat = sort_code: 0x6 : "Ingående moms":2640 "" -1690.21
                    // 	 vat = sort_code: 0x6 : "Debiterad ingående moms":2641 "" -849.52
                    // 	 vat = sort_code: 0x6 : "Redovisningskonto för moms":2650 "" -82415
                    // 	 cents = sort_code: 0x7 : "Öres- och kronutjämning":3740 "" 0.5

                    // TODO: Consider to iterate over all bas accounts defined for VAT Return form
                    //       and create a candidate that will zero out these for period given by date (end of VAT period)
                    BAS::MDJournalEntry mdje {
                      .defacto = {
                        .caption = had.heading
                        ,.date = had.date
                      }
                    };
                    mdje.defacto.account_transactions.push_back({
                      .account_no = 2610 // "Utgående moms, 25 %"
                      ,.transtext = std::nullopt
                      ,.amount = 0
                    });
                    mdje.defacto.account_transactions.push_back({
                      .account_no = 2614 // "Utgående moms omvänd skattskyldighet, 25 %"
                      ,.transtext = std::nullopt
                      ,.amount = 0
                    });
                    mdje.defacto.account_transactions.push_back({
                      .account_no = 2640 // "Ingående moms"
                      ,.transtext = std::nullopt
                      ,.amount = 0
                    });
                    mdje.defacto.account_transactions.push_back({
                      .account_no = 2641 // "Debiterad ingående moms"
                      ,.transtext = std::nullopt
                      ,.amount = 0
                    });
                    mdje.defacto.account_transactions.push_back({
                      .account_no = 2650 // "Redovisningskonto för moms"
                      ,.transtext = std::nullopt
                      ,.amount = had.amount
                    });
                    mdje.defacto.account_transactions.push_back({
                      .account_no = 3740 // "Öres- och kronutjämning"
                      ,.transtext = std::nullopt
                      ,.amount = 0
                    });
                    prompt << "\nVAT Consolidate candidate " << mdje;
                    had.optional.current_candidate = mdje;
                    model->prompt_state = PromptState::JEAggregateOptionIndex;
                  } break;
                  case JournalEntryVATType::VATClearing: {
                    auto tp = to_template(*tme_iter);
                    if (tp) {
                      auto mdje = to_md_journal_entry(had,*tp);
                      prompt << "\nVAT clearing candidate " << mdje;
                      had.optional.current_candidate = mdje;
                      model->prompt_state = PromptState::JEAggregateOptionIndex;
                    }
                  } break;
                  case JournalEntryVATType::SKVInterest: {
                    auto tp = to_template(*tme_iter);
                    if (tp) {
                      auto mdje = to_md_journal_entry(had,*tp);
                      prompt << "\nTax free SKV interest " << mdje;
                      had.optional.current_candidate = mdje;
                      model->prompt_state = PromptState::JEAggregateOptionIndex;
                    }
                  } break;
                  case JournalEntryVATType::SKVFee: {
                    if (false) {

                    }
                    else {
                      prompt << "\nSorry, I have yet to become capable to create an SKV Fee entry from template " << tme;
                    }
                  } break;
                  case JournalEntryVATType::VATTransfer: {
                    // 10  A2 "Utbetalning Moms från Skattekonto" 20210506
                    // transfer = sort_code: 0x1 : "Avräkning för skatter och avgifter (skattekonto)":1630 "Utbetalning" -802
                    // transfer = sort_code: 0x1 : "Avräkning för skatter och avgifter (skattekonto)":1630 "Momsbeslut" 802
                    // transfer vat = sort_code: 0x16 : "Momsfordran":1650 "" -802
                    // transfer = sort_code: 0x1 : "PlusGiro":1920 "" 802
                    if (tme.defacto.account_transactions.size()>0) {
                      bool first{true};
                      Amount amount{};
                      if (std::all_of(tme.defacto.account_transactions.begin(),tme.defacto.account_transactions.end(),[&amount,&first](auto const& entry){
                        // Lambda that returns true as long as all entries have the same absolute amount as the first entry
                        if (first) {
                          amount = abs(entry.first.amount);
                          first = false;
                          return true;
                        }
                        return (abs(entry.first.amount) == amount);
                      })) {
                        BAS::MDJournalEntry mdje {
                          .defacto = {
                            .caption = had.heading
                            ,.date = had.date
                          }
                        };
                        for (auto const& tat : tme.defacto.account_transactions) {
                          auto sign = (tat.first.amount < 0)?-1:+1;
                          mdje.defacto.account_transactions.push_back({
                            .account_no = tat.first.account_no
                            ,.transtext = std::nullopt
                            ,.amount = sign*abs(had.amount)
                          });
                        }
                        had.optional.current_candidate = mdje;
                        prompt << "\nVAT Settlement candidate " << mdje;
                        model->prompt_state = PromptState::JEAggregateOptionIndex;
                      }
                    }
                  } break;
                }
              }
              else if (auto at_ix = (ix - std::distance(tme_iter,tme_end));at_ix < std::distance(at_iter,at_end)) {
                prompt << "\nTODO: Implement acting on selected gross account transaction " << model->at_candidates[at_ix];
              }
              else {
                prompt << "\nPlease enter a valid index";
              }
            }
          }
          else {
            prompt << "\nPlease re-enter a valid HAD index (It seems I have no record of a selected HAD at the moment)";
          }
        } break;
        case PromptState::GrossDebitorCreditOption: {
          if (auto had_iter = model->selected_had()) {
            auto& had = *(*had_iter);
            if (had.optional.current_candidate) {
              if (had.optional.current_candidate->defacto.account_transactions.size()==1) {
                switch (ix) {
                  case 0: {
                    // As is
                    model->prompt_state = PromptState::CounterTransactionsAggregateOption;
                  } break;
                  case 1: {
                    // Force debit
                    had.optional.current_candidate->defacto.account_transactions[0].amount = abs(had.optional.current_candidate->defacto.account_transactions[0].amount);
                    model->prompt_state = PromptState::CounterTransactionsAggregateOption;
                  } break;
                  case 2: {
                      // Force credit
                    had.optional.current_candidate->defacto.account_transactions[0].amount = -1.0f * abs(had.optional.current_candidate->defacto.account_transactions[0].amount);
                    model->prompt_state = PromptState::CounterTransactionsAggregateOption;
                  } break;
                  default: {
                    prompt << "\nPlease enter a valid index. I don't know how to interpret option " << ix;
                  }
                }
              }
              else {
                prompt << "\nPlease re-enter a valid HAD and journal entry candidate (It seems current candidate have more than one transaction defined which confuses me)";
              }
              prompt << "\ncandidate:" << *had.optional.current_candidate;
            }
            else {
              prompt << "\nPlease re-enter a valid HAD and journal entry candidate (I seem to no longer have a valid journal entry candidate for current HAD to process)";
            }
          }
          else {
            prompt << "\nPlease re-enter a valid HAD index (It seems I have no recoprd of a selected HAD at the moment)";
          }
        } break;
        case PromptState::CounterTransactionsAggregateOption: {
          if (auto had_iter = model->selected_had()) {
            auto& had = *(*had_iter);
            if (had.optional.current_candidate) {
              if (had.optional.current_candidate->defacto.account_transactions.size()==1) {
                switch (ix) {
                  case 0: {
                    // Gross counter transaction aggregate
                    model->prompt_state = PromptState::GrossAccountInput;
                  } break;
                  case 1: {
                    // {net,VAT} counter transactions aggregate
                    model->prompt_state = PromptState::NetVATAccountInput;
                  } break;
                  default: {
                    prompt << "\nPlease enter a valid index. I don't know how to interpret option " << ix;
                  }
                }
              }
              else {
                prompt << "\nPlease re-enter a valid HAD and journal entry candidate (It seems current candidate have more than one transaction defined which confuses me)";
              }
              prompt << "\ncandidate:" << *had.optional.current_candidate;
            }
            else {
              prompt << "\nPlease re-enter a valid HAD and journal entry candidate (I seem to no longer have a valid journal entry candidate for current HAD to process)";
            }
          }
          else {
            prompt << "\nPlease re-enter a valid HAD index (It seems I have no recoprd of a selected HAD at the moment)";
          }
        } break;
        case PromptState::GrossAccountInput: {
          // Act on user gross account number input
          if (auto had_iter = model->selected_had()) {
            auto& had = *(*had_iter);
            if (had.optional.current_candidate) {
              if (had.optional.current_candidate->defacto.account_transactions.size()==1) {
                if (ast.size() == 1) {
                  auto gross_counter_account_no = BAS::to_account_no(ast[0]);
                  if (gross_counter_account_no) {
                    Amount gross_transaction_amount = had.optional.current_candidate->defacto.account_transactions[0].amount;
                    had.optional.current_candidate->defacto.account_transactions.push_back(BAS::anonymous::AccountTransaction{
                      .account_no = *gross_counter_account_no
                      ,.amount = -1.0f * gross_transaction_amount
                    }
                    );
                    prompt << "\nmutated candidate:" << *had.optional.current_candidate;
                    model->prompt_state = PromptState::JEAggregateOptionIndex;
                  }
                  else {
                    prompt << "\nPlease enter a a valid single gross counter amount account number (it seems I don't understand your input " << std::quoted(command) << ")";
                  }
                }
                else {
                  prompt << "\nPlease enter two single gross counter amount account number (it seems I interpret " << std::quoted(command) << " the wrong number of arguments";
                }
              }
              else {
                prompt << "\nPlease re-enter a valid HAD and journal entry candidate (It seems current candidate have more than one transaction defined which confuses me)";
              }
              prompt << "\ncandidate:" << *had.optional.current_candidate;
            }
            else {
              prompt << "\nPlease re-enter a valid HAD and journal entry candidate (I seem to no longer have a valid journal entry candidate for current HAD to process)";
            }
          }
          else {
            prompt << "\nPlease re-enter a valid HAD index (It seems I have no recoprd of a selected HAD at the moment)";
          }
        } break;
        case PromptState::NetVATAccountInput: {
          if (auto had_iter = model->selected_had()) {
            auto& had = *(*had_iter);
            if (had.optional.current_candidate) {
              if (had.optional.current_candidate->defacto.account_transactions.size()==1) {
                if (ast.size() == 2) {
                  auto net_counter_account_no = BAS::to_account_no(ast[0]);
                  auto vat_counter_account_no = BAS::to_account_no(ast[1]);
                  if (net_counter_account_no and vat_counter_account_no) {
                    Amount gross_transaction_amount = had.optional.current_candidate->defacto.account_transactions[0].amount;
                    had.optional.current_candidate->defacto.account_transactions.push_back(BAS::anonymous::AccountTransaction{
                      .account_no = *net_counter_account_no
                      ,.amount = -0.8f * gross_transaction_amount
                    }
                    );
                    had.optional.current_candidate->defacto.account_transactions.push_back(BAS::anonymous::AccountTransaction{
                      .account_no = *vat_counter_account_no
                      ,.amount = -0.2f * gross_transaction_amount
                    }
                    );
                    prompt << "\nNOTE: Cratchit currently assumes 25% VAT";
                    model->prompt_state = PromptState::JEAggregateOptionIndex;
                  }
                  else {
                    prompt << "\nPlease enter a a valid single gross counter amount account number (it seems I don't understand your input " << std::quoted(command) << ")";
                  }
                }
                else {
                  prompt << "\nPlease enter two single gross counter amount account number (it seems I interpret " << std::quoted(command) << " the wrong number of arguments";
                }
              }
              else {
                prompt << "\nPlease re-enter a valid HAD and journal entry candidate (It seems current candidate have more than one transaction defined which confuses me)";
              }
              prompt << "\ncandidate:" << *had.optional.current_candidate;
            }
            else {
              prompt << "\nPlease re-enter a valid HAD and journal entry candidate (I seem to no longer have a valid journal entry candidate for current HAD to process)";
            }
          }
          else {
            prompt << "\nPlease re-enter a valid HAD index (It seems I have no recoprd of a selected HAD at the moment)";
          }
        } break;
        case PromptState::JEAggregateOptionIndex: {
          // ":had:je:1or*";
          if (auto had_iter = model->selected_had()) {
            auto& had = *(*had_iter);
            if (had.optional.current_candidate) {
              // We need a typed entry to do some clever decisions
              auto tme = to_typed_md_entry(*had.optional.current_candidate);
              prompt << "\n" << tme;
              auto vat_type = to_vat_type(tme);
              switch (vat_type) {
                case JournalEntryVATType::NoVAT: {
                  // No VAT in candidate.
                  // Continue with
                  // 1) Some propose gross account transactions
                  // 2) a n x gross Counter aggregate
                } break;
                case JournalEntryVATType::SwedishVAT: {
                  // Swedish VAT detcted in candidate.
                  // Continue with
                  // 2) a n x {net,vat} counter aggregate
                } break;
                case JournalEntryVATType::EUVAT: {
                  // EU VAT detected in candidate.
                  // Continue with a
                  // 2) n x gross counter aggregate + an EU VAT Returns "virtual" aggregate
                } break;
                case JournalEntryVATType::VATReturns: {
                  // All VATS (VAT Report?)
                } break;
                case JournalEntryVATType::VATTransfer: {
                  // All VATS and one gross non-vat (assume bank transfer of VAT to/from tax agency?)
                } break;
                default: {std::cout << "\nDESIGN INSUFFICIENCY - Unknown JournalEntryVATType " << vat_type;}
              }
              // std::map<std::string,unsigned int> props_counter{};
              // for (auto const& [at,props] : tme.defacto.account_transactions) {
              // 	for (auto const& prop : props) props_counter[prop]++;
              // }
              // for (auto const& [prop,count] : props_counter) {
              // 	prompt << "\n" << std::quoted(prop) << " count:" << count;
              // }
              // auto props_sum = std::accumulate(props_counter.begin(),props_counter.end(),unsigned{0},[](auto acc,auto const& entry){
              // 	acc += entry.second;
              // 	return acc;
              // });
              // int vat_type{-1}; // unidentified VAT
              // // Identify what type of VAT the candidate defines
              // if ((props_counter.size() == 1) and props_counter.contains("gross")) {
              // 	vat_type = 0; // NO VAT (gross, counter gross)
              // 	prompt << "\nTemplate is an NO VAT transaction :)"; // gross,gross
              // }
              // else if ((props_counter.size() == 3) and props_counter.contains("gross") and props_counter.contains("net") and props_counter.contains("vat") and !props_counter.contains("eu_vat")) {
              // 	if (props_sum == 3) {
              // 		prompt << "\nTemplate is a SWEDISH PURCHASE/sale"; // (gross,net,vat);
              // 		vat_type = 1; // Swedish VAT
              // 	}
              // }
              // else if (
              // 	(     (props_counter.contains("gross"))
              // 		and (props_counter.contains("eu_purchase"))
              // 		and (props_counter.contains("eu_vat")))) {
              // 	vat_type = 2; // EU VAT
              // 	prompt << "\nTemplate is an EU PURCHASE :)"; // gross,gross,eu_vat,eu_vat,eu_purchase,eu_purchase
              // }
              // else {
              // 	prompt << "\nFailed to recognise the VAT type";
              // }

              switch (ix) {
                case 0: {
                  // Try to stage gross + single counter transactions aggregate
                  if (does_balance(had.optional.current_candidate->defacto) == false) {
                    // list at candidates from found entries with account transaction that counter the gross account
                    std::cout << "\nCurrent candidate does not balance";
                  }
                  else if (std::any_of(had.optional.current_candidate->defacto.account_transactions.begin(),had.optional.current_candidate->defacto.account_transactions.end(),[](BAS::anonymous::AccountTransaction const& at){
                    return abs(at.amount) < 1.0;
                  })) {
                    // Assume the user need to specify rounding by editing proposed account transactions
                    if (false) {
                      // 20240526 - new 'enter state' prompt mechanism
                      auto new_state = PromptState::ATIndex;
                      prompt << model->to_prompt_for_entering_state(new_state);
                      model->prompt_state = new_state;
                    }
                    else {
                      unsigned int i{};
                      std::for_each(had.optional.current_candidate->defacto.account_transactions.begin(),had.optional.current_candidate->defacto.account_transactions.end(),[&i,&prompt](auto const& at){
                        prompt << "\n  " << i++ << " " << at;
                      });
                      model->prompt_state = PromptState::ATIndex;
                    }
                  }
                  else {
                    // Stage as-is
                    if (auto stage_result = model->sie_env_map.stage(*had.optional.current_candidate)) {
                      prompt << "\n" << stage_result.md_entry() << " STAGED";
                      model->heading_amount_date_entries.erase(*had_iter);
                      model->prompt_state = PromptState::HADIndex;
                    }
                    else {
                      prompt << "\nSORRY - Failed to stage entry";
                      model->prompt_state = PromptState::Root;
                    }
                  }
                } break;
                case 1: {
                  // net + vat counter aggregate
                  BAS::anonymous::OptionalAccountTransaction net_at;
                  BAS::anonymous::OptionalAccountTransaction vat_at;
                  for (auto const& [at,props] : tme.defacto.account_transactions) {
                    if (props.contains("net")) net_at = at;
                    if (props.contains("vat")) vat_at = at;
                  }
                  if (!net_at) std::cout << "\nNo net_at";
                  if (!vat_at) std::cout << "\nNo vat_at";
                  if (net_at and vat_at) {
                    had.optional.counter_ats_producer = ToNetVatAccountTransactions{*net_at,*vat_at};

                    BAS::anonymous::AccountTransactions ats_to_keep{};
                    std::remove_copy_if(
                      had.optional.current_candidate->defacto.account_transactions.begin()
                      ,had.optional.current_candidate->defacto.account_transactions.end()
                      ,std::back_inserter(ats_to_keep)
                      ,[&net_at,&vat_at](auto const& at){
                        return ((at.account_no == net_at->account_no) or (at.account_no == vat_at->account_no));
                    });
                    had.optional.current_candidate->defacto.account_transactions = ats_to_keep;
                  }
                  prompt << "\ncadidate: " << *had.optional.current_candidate;
                  model->prompt_state = PromptState::EnterHA;
                } break;
                case 2: {
                  // Allow the user to edit individual account transactions
                  if (true) {
                    // 20240526 - new 'enter state' prompt mechanism
                    auto new_state = PromptState::ATIndex;
                    prompt << model->to_prompt_for_entering_state(new_state);
                    model->prompt_state = new_state;
                  }
                  else {
                    unsigned int i{};
                    std::for_each(had.optional.current_candidate->defacto.account_transactions.begin(),had.optional.current_candidate->defacto.account_transactions.end(),[&i,&prompt](auto const& at){
                      prompt << "\n  " << i++ << " " << at;
                    });
                    model->prompt_state = PromptState::ATIndex;
                  }
                } break;
                case 3: {
                  // Stage the candidate
                  if (auto stage_result = model->sie_env_map.stage(*had.optional.current_candidate)) {
                    prompt << "\n" << stage_result.md_entry() << " STAGED";
                    model->heading_amount_date_entries.erase(*had_iter);
                    model->prompt_state = PromptState::HADIndex;
                  }
                  else {
                    prompt << "\nSORRY - Failed to stage entry";
                    model->prompt_state = PromptState::Root;
                  }
                }
                default: {
                  prompt << "\nPlease enter a valid had index";
                } break;
              }
            }
            else {
              prompt << "\nPlease re-enter a valid HAD and journal entry candidate (I seem to no longer have a valid journal entry candidate for current HAD to process)";
            }
          }
          else {
            prompt << "\nPlease re-enter a valid had index";
          }

        } break;
        case PromptState::ATIndex: {
          if (auto had_iter = model->selected_had()) {
            auto& had = **had_iter;
            if (ast.size() == 2) {
			  // New (Account, Amount) entry input
              auto bas_account_no = BAS::to_account_no(ast[0]);
              auto amount = to_amount(ast[1]);
              if (bas_account_no and amount) {
                // push back a new account transaction with detected BAS account and amount
                BAS::anonymous::AccountTransaction at {
                  .account_no = *bas_account_no
                  ,.amount = *amount
                };
                had.optional.current_candidate->defacto.account_transactions.push_back(at);
                unsigned int i{};
                std::for_each(had.optional.current_candidate->defacto.account_transactions.begin(),had.optional.current_candidate->defacto.account_transactions.end(),[&i,&prompt](auto const& at){
                  prompt << "\n  " << i++ << " " << at;
                });
              }
              else {
                prompt << "\nSorry, I failed to understand your entry. Press <Enter> to get help" << std::quoted(command);
              }
            }
            else if (auto at_iter = model->to_at_iter(model->selected_had(),ix)) {
              model->at_index = ix;
              prompt << "\nAccount Transaction:" << **at_iter;
              if (do_remove) {
                had.optional.current_candidate->defacto.account_transactions.erase(*at_iter);
              }
              model->prompt_state = PromptState::EditAT;
            }
            else {
              prompt << "\nEntered index does not refer to an Account Transaction Entry in current Heading Amount Date entry";
              model->prompt_state = PromptState::JEAggregateOptionIndex;
            }
          }
          else {
            prompt << "\nSorry, I seems to have lost track of the HAD you selected. Please re-select a valid HAD";
            model->prompt_state = PromptState::HADIndex;
          }

        } break;

        case PromptState::CounterAccountsEntry: {
          prompt << "\nThe Counter Account Entry state is not yet active in this version of cratchit";
        } break;

        case PromptState::SKVEntryIndex: {
          // prompt << "\n1: Arbetsgivardeklaration (Employer’s contributions and PAYE tax return form)";
          // prompt << "\n2: Periodisk Sammanställning (EU sales list)"
          switch (ix) {
            case 0: {
              // Assume Employer’s contributions and PAYE tax return form

              // List Tax Return Form skv options (user data edit)
              auto const& [delta_prompt,prompt_state] = this->transition_prompt_state(model->prompt_state,PromptState::SKVTaxReturnEntryIndex);
              prompt << delta_prompt;
              model->prompt_state = prompt_state;
            } break;
            case 1: {
              // Create current quarter, previous quarter or two previous quarters option
              auto today = to_today();
              auto current_qr = zeroth::to_quarter_range(today);
              auto previous_qr = zeroth::to_three_months_earlier(current_qr);
              auto quarter_before_previous_qr = zeroth::to_three_months_earlier(previous_qr);
              auto two_previous_quarters = zeroth::DateRange{quarter_before_previous_qr.begin(),previous_qr.end()};

              prompt << "\n0: Track Current Quarter " << current_qr;
              prompt << "\n1: Report Previous Quarter " << previous_qr;
              prompt << "\n2: Check Quarter before previous " << quarter_before_previous_qr;
              prompt << "\n3: Check Previous two Quarters " << two_previous_quarters;
              model->prompt_state = PromptState::QuarterOptionIndex;
            } break;
            case 2: {
              // Create K10 form files
              if (auto fm = SKV::SRU::K10::to_files_mapping()) {
                std::filesystem::path info_file_path{"to_skv/K10/INFO.SRU"};
                std::filesystem::create_directories(info_file_path.parent_path());
                auto info_std_os = std::ofstream{info_file_path};
                SKV::SRU::OStream info_sru_os{info_std_os};
                SKV::SRU::InfoOStream info_os{info_sru_os};
                if (info_os << *fm) {
                  prompt << "\nCreated " << info_file_path;
                }
                else {
                  prompt << "\nSorry, FAILED to create " << info_file_path;
                }

                std::filesystem::path blanketter_file_path{"to_skv/K10/BLANKETTER.SRU"};
                std::filesystem::create_directories(blanketter_file_path.parent_path());
                auto blanketter_std_os = std::ofstream{blanketter_file_path};
                SKV::SRU::OStream blanketter_sru_os{blanketter_std_os};
                SKV::SRU::BlanketterOStream blanketter_os{blanketter_sru_os};
                if (blanketter_os << *fm) {
                  prompt << "\nCreated " << blanketter_file_path;
                }
                else {
                  prompt << "\nSorry, FAILED to create " << blanketter_file_path;
                }
              }
              else {
                prompt << "\nSorry, failed to create data for input to K10 SRU-files";
              }
            } break;
            case 3: {
              // Generate the K10 and INK1 forms as SRU-files

              // We need two input values verified by the user
              // 1. The Total income to tax (SKV SRU Code 1000)
              // 2. The dividend to Tax (SKV SRU Code 4504)
              Amount income = get_INK1_Income(model);
              prompt << "\n1: INK1 1.1 Lön, förmåner, sjukpenning m.m. = " << income;
              Amount dividend = get_K10_Dividend(model);
              prompt << "\n2: K10 1.6 Utdelning = " << dividend;
              prompt << "\n3: Continue (Create K10 and INK1)";
              model->prompt_state = PromptState::K10INK1EditOptions;
            } break;
            case 4: {
              // "\n4: INK2 + INK2S + INK2R (Company Tax Returns form(s))";
              prompt << "\n1: INK2::meta_xx = ?? (TODO: Add edit of meta data for INK2 forms)";
              prompt << "\n2: Generate INK2";
              model->prompt_state = PromptState::INK2AndAppendixEditOptions;
            } break;
            default: {prompt << "\nPlease enter a valid index";} break;
          }
        } break;

        case PromptState::QuarterOptionIndex: {
          auto today = to_today();
          auto current_qr = zeroth::to_quarter_range(today);
          auto previous_qr = zeroth::to_three_months_earlier(current_qr);
          auto quarter_before_previous_qr = zeroth::to_three_months_earlier(previous_qr);
          auto two_previous_quarters = zeroth::DateRange{quarter_before_previous_qr.begin(),previous_qr.end()};
          zeroth::OptionalDateRange period_range{};
          switch (ix) {
            case 0: {
              prompt << "\nCurrent Quarter " << current_qr << " (to track)";
              period_range = current_qr;
            } break;
            case 1: {
              prompt << "\nPrevious Quarter " << previous_qr << " (to report)";
              period_range = previous_qr;
            } break;
            case 2: {
              prompt << "\nQuarter before previous " << quarter_before_previous_qr << " (to check)";
              period_range = quarter_before_previous_qr;
            } break;
            case 3: {
              prompt << "\nPrevious two Quarters " << two_previous_quarters << " (to check)";
              period_range = two_previous_quarters;
            }
            default: {
              prompt << "\nPlease select a valid option (it seems option " << ix << " is unknown to me";
            } break;
          }
          if (period_range) {
            // Create VAT Returns form for selected period
            prompt << "\nVAT Returns for " << *period_range;
            if (auto vat_returns_meta = SKV::XML::VATReturns::to_vat_returns_meta(*period_range)) {
              SKV::OrganisationMeta org_meta {
                .org_no = model->sie_env_map["current"].organisation_no.CIN
                ,.contact_persons = model->organisation_contacts
              };
              SKV::XML::DeclarationMeta form_meta {
                .declaration_period_id = vat_returns_meta->period_to_declare
              };
              auto is_quarter = [&vat_returns_meta](BAS::MDAccountTransaction const& mdat){
                return vat_returns_meta->period.contains(mdat.meta.date);
              };
              auto box_map = SKV::XML::VATReturns::to_form_box_map(model->sie_env_map,is_quarter);
              if (box_map) {
                prompt << *box_map;
                auto xml_map = SKV::XML::VATReturns::to_xml_map(*box_map,org_meta,form_meta);
                if (xml_map) {
                  std::filesystem::path skv_files_folder{"to_skv"};
                  std::filesystem::path skv_file_name{std::string{"moms_"} + vat_returns_meta->period_to_declare + ".eskd"};
                  std::filesystem::path skv_file_path = skv_files_folder / skv_file_name;
                  std::filesystem::create_directories(skv_file_path.parent_path());
                  std::ofstream skv_file{skv_file_path};
                  SKV::XML::VATReturns::OStream vat_returns_os{skv_file};
                  if (vat_returns_os << *xml_map) {
                    prompt << "\nCreated " << skv_file_path;
                    SKV::XML::VATReturns::OStream vat_returns_prompt{prompt};
                    vat_returns_prompt << "\n" << *xml_map;
                  }
                  else prompt << "\nSorry, failed to create the file " << skv_file_path;
                }
                else prompt << "\nSorry, failed to map form data to XML Data required for the VAR Returns form file";
                // Generate an EU Sales List form for the VAt Returns form
                if (auto eu_list_form = SKV::CSV::EUSalesList::vat_returns_to_eu_sales_list_form(*box_map,org_meta,*period_range)) {
                  auto eu_list_quarter = SKV::CSV::EUSalesList::to_eu_list_quarter(period_range->end());
                  std::filesystem::path skv_files_folder{"to_skv"};
                  std::filesystem::path skv_file_name{std::string{"periodisk_sammanstallning_"} + eu_list_quarter.yy_hyphen_quarter_seq_no + "_" + to_string(today) + ".csv"};
                  std::filesystem::path eu_list_form_file_path = skv_files_folder / skv_file_name;
                  std::filesystem::create_directories(eu_list_form_file_path.parent_path());
                  std::ofstream eu_list_form_file_stream{eu_list_form_file_path};
                  SKV::CSV::EUSalesList::OStream os{eu_list_form_file_stream};
                  if (os << *eu_list_form) {
                    prompt << "\nCreated file " << eu_list_form_file_path << " OK";
                    SKV::CSV::EUSalesList::OStream eu_sales_list_prompt{prompt};
                    eu_sales_list_prompt << "\n" <<  *eu_list_form;
                  }
                  else {
                    prompt << "\nSorry, failed to write " << eu_list_form_file_path;
                  }
                }
                else {
                  prompt << "\nSorry, failed to acquire required data for the EU List form file";
                }
              }
              else prompt << "\nSorry, failed to gather form data required for the VAT Returns form";
            }
            else {
              prompt << "\nSorry, failed to gather meta-data for the VAT returns form for period " << *period_range;
            }
          }
        } break;
        case PromptState::SKVTaxReturnEntryIndex: {
          switch (ix) {
            case 1: {model->prompt_state = PromptState::EnterContact;} break;
            case 2: {model->prompt_state = PromptState::EnterEmployeeID;} break;
            case 3: {
              if (ast.size() == 2) {
                // Assume Tax Returns form
                // Assume second argument is period
                if (auto xml_map = cratchit_to_skv(model->sie_env_map["current"],model->organisation_contacts,model->employee_birth_ids)) {
                  auto period_to_declare = ast[1];
                  // Brute force the period into the map (TODO: Inject this value in a better way into the production code above?)
                  (*xml_map)[R"(Skatteverket^agd:Blankett^agd:Arendeinformation^agd:Period)"] = period_to_declare;
                  (*xml_map)[R"(Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:HU^agd:RedovisningsPeriod faltkod="006")"] = period_to_declare;
                  (*xml_map)[R"(Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:IU^agd:RedovisningsPeriod faltkod="006")"] = period_to_declare;
                  (*xml_map)[R"(Skatteverket^agd:Kontaktperson^agd:Blankettinnehall^agd:HU^agd:RedovisningsPeriod faltkod="006")"] = period_to_declare;
                  std::filesystem::path skv_files_folder{"to_skv"};
                  std::filesystem::path skv_file_name{std::string{"arbetsgivaredeklaration_"} + period_to_declare + ".xml"};
                  std::filesystem::path skv_file_path = skv_files_folder / skv_file_name;
                  std::filesystem::create_directories(skv_file_path.parent_path());
                  std::ofstream skv_file{skv_file_path};
                  if (SKV::XML::to_employee_contributions_and_PAYE_tax_return_file(skv_file,*xml_map)) {
                    prompt << "\nCreated " << skv_file_path;
                    prompt << "\nUpload file to Swedish Skatteverket";
                    prompt << "\n1. Browse to https://skatteverket.se/foretag";
                    prompt << "\n2. Click on 'Lämna arbetsgivardeklaration'";
                    prompt << "\n3. Log in";
                    prompt << "\n4. Choose to represent your company";
                    prompt << "\n5. Click on 'Deklarera via fil' and follow the instructions to upload the file " << skv_file_path;
                  }
                  else {
                    prompt << "\nSorry, failed to create " << skv_file_path;
                  }
                }
                else {
                  prompt << "\nSorry, failed to acquire required data to generate xml-file to SKV";
                }
                model->prompt_state = PromptState::Root;
              }
              else {
                prompt << "\nPlease provide second argument = period on the form YYYYMM (e.g., enter '3 202205' to generate Tax return form for May 2022)";
              }
            } break;
            default: {prompt << "\nPlease enter a valid index";} break;
          }
        } break;

        case PromptState::K10INK1EditOptions: {
          switch (ix) {
            case 1: {model->prompt_state = PromptState::EnterIncome;} break;
            case 2: {model->prompt_state = PromptState::EnterDividend;} break;
            case 3: {
              if (false) {
                // 230420 - began hard coding generation of corrected K10 for last year + K10 and INK1 for this year
                // PAUS for now (I ended up sending in a manually edited INFO.SRU and BLANKETTER.SRU)
                // TODO: Consider to:
                // 1. Use CSV-files as read by skv_specs_mapping_from_csv_files()
                //    Get rid of hard coded SKV::SRU::INK1::k10_csv_to_sru_template and SKV::SRU::INK1::ink1_csv_to_sru_template?
                //    NOTE: They both use the same csv-files as input (2 at compile time and 1 at run-time)
                //    Actually - Decide if these csv-files should be hard coded or read at run-time?
                // 2. Consider to expose these csv-file as actual forms for the user to edit?
                //    I imagine we can generate indexed entries to each field and have the user select end edit its value?
                //    Question is how to implemented computation as defined by SKV for these forms (rates and values gets redefined each year!)
                // ...
                /*
                  SKV::SRU::FilesMapping fm {
                    .info = to_info_sru_file_tag_map(model);
                  };
                  fm.blanketter.push_back(to_k10_blankett(2021));
                  fm.blanketter.push_back(to_k10_blankett(2022));
                  fm.blanketter.push_back(to_ink1_blankett(2022));
                */
              }
              else {
                // TODO: Split to allow edit of Income and Dividend before entering the actual generation phase/code

                SKV::SRU::OptionalSRUValueMap k10_sru_value_map{};
                SKV::SRU::OptionalSRUValueMap ink1_sru_value_map{};

                // k10_csv_to_sru_template
                std::istringstream k10_is{SKV::SRU::INK1::k10_csv_to_sru_template};
                encoding::UTF8::istream utf8_K10_in{k10_is};
                if (auto field_rows = CSV::to_field_rows(utf8_K10_in)) {
                  // LOG
                  for (auto const& field_row : *field_rows) {
                    if (field_row.size()>0) prompt << "\n";
                    for (int i=0;i<field_row.size();++i) {
                      prompt << " [" << i << "]" << field_row[i];
                    }
                  }
                  // Acquire the SRU Values required for the K10 Form
                  k10_sru_value_map = SKV::SRU::to_sru_value_map(model,model->selected_year_index,*field_rows);
                }
                else {
                  prompt << "\nSorry, failed to acquire a valid template for the K10 form";
                }

                // ink1_csv_to_sru_template
                std::istringstream ink1_is{SKV::SRU::INK1::ink1_csv_to_sru_template};
                encoding::UTF8::istream utf8_ink1_in{ink1_is};
                if (auto field_rows = CSV::to_field_rows(utf8_ink1_in)) {
                  for (auto const& field_row : *field_rows) {
                    if (field_row.size()>0) prompt << "\n";
                    for (int i=0;i<field_row.size();++i) {
                      prompt << " [" << i << "]" << field_row[i];
                    }
                  }
                  // Acquire the SRU Values required for the INK1 Form
                  ink1_sru_value_map = SKV::SRU::to_sru_value_map(model,model->selected_year_index,*field_rows);
                }
                else {
                  prompt << "\nSorry, failed to acquire a valid template for the INK1 form";
                }


                // Use gathered values
                if (k10_sru_value_map and ink1_sru_value_map) {

                  // Gather data fro info.sru
                  SKV::SRU::SRUFileTagMap info_sru_file_tag_map{};
                  {
                    // Assume we are to send in with sender being this company?
                    // 9. #ORGNR
                      // #ORGNR 191111111111
                    info_sru_file_tag_map["#ORGNR"] = model->sie_env_map["current"].organisation_no.CIN;
                    // 10. #NAMN
                      // #NAMN Databokföraren
                    info_sru_file_tag_map["#NAMN"] = model->sie_env_map["current"].organisation_name.company_name;

                    auto postal_address = model->sie_env_map["current"].organisation_address.postal_address; // "17668 J?rf?lla" split in <space> to get ZIP and Town
                    auto postal_address_tokens = tokenize::splits(postal_address,' ');

                    // 12. #POSTNR
                      // #POSTNR 12345
                    if (postal_address_tokens.size() > 0) {
                      info_sru_file_tag_map["#POSTNR"] = postal_address_tokens[0];
                    }
                    else {
                      info_sru_file_tag_map["#POSTNR"] = "?POSTNR?";
                    }
                    // 13. #POSTORT
                      // #POSTORT SKATTSTAD
                    if (postal_address_tokens.size() > 1) {
                      info_sru_file_tag_map["#POSTORT"] = postal_address_tokens[1];
                    }
                    else {
                      info_sru_file_tag_map["#POSTORT"] = "?POSTORT?";
                    }
                  }

                  SKV::SRU::SRUFileTagMap k10_sru_file_tag_map{};
                  {
                    // #BLANKETT N7-2013P1
                    // k10_sru_file_tag_map["#BLANKETT"] = "K10-2021P4"; // See file "_Nyheter_from_beskattningsperiod_2021P4_.pdf" (https://skatteverket.se/download/18.96cca41179bad4b1aac351/1642600897155/Nyheter_from_beskattningsperiod_2021P4.zip)
                    k10_sru_file_tag_map["#BLANKETT"] = "K10-2022P4"; // See table 1, entry K10 column "blankett-block" in file "_Nyheter_from_beskattningsperiod_2022P4-3.pdf" (https://skatteverket.se/download/18.48cfd212185efbb440b65a8/1679495970044/_Nyheter_from_beskattningsperiod_2022P4-3.zip)
                    // #IDENTITET 193510250100 20130426 174557
                    std::ostringstream os{};
                    if (model->employee_birth_ids[0].size()>0) {
                      os << " " << model->employee_birth_ids[0];
                    }
                    else {
                      os << " " << "?PERSONNR?";
                    }
                    auto today = to_today();
                    os << " " << today;
                    os << " " << "120000";
                    k10_sru_file_tag_map["#IDENTITET"] = os.str();
                  }

                  SKV::SRU::SRUFileTagMap ink1_sru_file_tag_map{};
                  {
                    // #BLANKETT N7-2013P1
                    // ink1_sru_file_tag_map["#BLANKETT"] = "INK1-2021P4"; // See file "_Nyheter_from_beskattningsperiod_2021P4_.pdf" (https://skatteverket.se/download/18.96cca41179bad4b1aac351/1642600897155/Nyheter_from_beskattningsperiod_2021P4.zip)
                    ink1_sru_file_tag_map["#BLANKETT"] = " INK1-2022P4"; // See entry INK1 column "blankett-block" in file "_Nyheter_from_beskattningsperiod_2022P4-3.pdf" (https://skatteverket.se/download/18.48cfd212185efbb440b65a8/1679495970044/_Nyheter_from_beskattningsperiod_2022P4-3.zip)
                    // #IDENTITET 193510250100 20130426 174557
                    std::ostringstream os{};
                    if (model->employee_birth_ids[0].size()>0) {
                      os << " " << model->employee_birth_ids[0];
                    }
                    else {
                      os << " " << "?PERSONNR?";
                    }
                    auto today = to_today();
                    os << " " << today;
                    os << " " << "120000";

                    ink1_sru_file_tag_map["#IDENTITET"] = os.str();
                  }
                  SKV::SRU::FilesMapping fm {
                    .info = info_sru_file_tag_map
                  };
                  SKV::SRU::Blankett k10_blankett{k10_sru_file_tag_map,*k10_sru_value_map};
                  fm.blanketter.push_back(k10_blankett);
                  SKV::SRU::Blankett ink1_blankett{ink1_sru_file_tag_map,*ink1_sru_value_map};
                  fm.blanketter.push_back(ink1_blankett);

                  std::filesystem::path info_file_path{"to_skv/SRU/INFO.SRU"};
                  std::filesystem::create_directories(info_file_path.parent_path());
                  auto info_std_os = std::ofstream{info_file_path};
                  SKV::SRU::OStream info_sru_os{info_std_os};
                  SKV::SRU::InfoOStream info_os{info_sru_os};

                  if (info_os << fm) {
                    prompt << "\nCreated " << info_file_path;
                  }
                  else {
                    prompt << "\nSorry, FAILED to create " << info_file_path;
                  }

                  std::filesystem::path blanketter_file_path{"to_skv/SRU/BLANKETTER.SRU"};
                  std::filesystem::create_directories(blanketter_file_path.parent_path());
                  auto blanketter_std_os = std::ofstream{blanketter_file_path};
                  SKV::SRU::OStream blanketter_sru_os{blanketter_std_os};
                  SKV::SRU::BlanketterOStream blanketter_os{blanketter_sru_os};

                  if (blanketter_os << fm) {
                    prompt << "\nCreated " << blanketter_file_path;
                  }
                  else {
                    prompt << "\nSorry, FAILED to create " << blanketter_file_path;
                  }

                }
                else {
                  prompt << "\nSorry, Failed to acquirer the data for the K10 and INK1 forms";
                }
              } // if false
            } break;
            default: {prompt << "\nPlease enter a valid index";} break;
          }
        } break;

        case PromptState::INK2AndAppendixEditOptions: {
          switch (ix) {
            case 1: {
              prompt << "\nTODO: Add edit of INK2 meta data";
            } break;
            case 2: {

              // flag to tweak behaviour for 2023-2024 INK2 known result check
              // That is, With this flag cratchit shall produce INFO.SRU and BLANKETTER.SRU
              // with the same values as known files generated by online tool (and accepted by SKV)
              // so known to be 'true enough'
              // const bool mock_for_test_2023_2024{true};
              const bool mock_for_test_2023_2024{false};

              // Inject the SRU Values required for the INK2 Forms

              // Räkenskapsår (INK2R,INK2S,INK2)
              // TODO: Implement taking these dates from Fiscal year defined by model->selected_year_index

              if (mock_for_test_2023_2024) {
                model->sru[model->selected_year_index].set(7011,std::string{"20230501"});
                model->sru[model->selected_year_index].set(7012,std::string{"20240430"});
              }
              else {
                // Hard code fr my own need for INK2 2024-2025
                // #UPPGIFT 7011 20230501
                model->sru[model->selected_year_index].set(7011,std::string{"20240501"});
                // #UPPGIFT 7012 20240430
                model->sru[model->selected_year_index].set(7012,std::string{"20250430"});
              }

              // Upplysningar om årsredovisning (INK2S)
              // Uppdragstagare har biträtt vid upprättande av årsredovisning
              // 8040 = X (JA) 8041 = X (NEJ)
              model->sru[model->selected_year_index].set(8041,std::string{"X"});
              prompt << "\nUppdragstagare har biträtt vid upprättande av årsredovisning SRU:8040 X = JA  - hard coded to " << model->sru[model->selected_year_index].at(8040);
              prompt << "\nUppdragstagare har biträtt vid upprättande av årsredovisning SRU:8041 X = NEJ  - hard coded to " << model->sru[model->selected_year_index].at(8041);

              // Årsredovisning har varit föremål för revision
              // 8044 = X (JA) 8045 = X (NEJ)
              model->sru[model->selected_year_index].set(8045,std::string{"X"});
              prompt << "\nÅrsredovisning har varit föremål för revision SRU:8044 X = JA  - hard coded to " << model->sru[model->selected_year_index].at(8044);
              prompt << "\nÅrsredovisning har varit föremål för revision SRU:8045 X = NEJ  - hard coded to " << model->sru[model->selected_year_index].at(8045);

              SKV::SRU::OptionalSRUValueMap ink2r_sru_value_map{};
              {
                // INK2R_csv_to_sru_template
                std::istringstream is{SKV::SRU::INK2::Y_2024::INK2R_csv_to_sru_template};
                encoding::UTF8::istream utf8_in{is};
                if (auto field_rows = CSV::to_field_rows(utf8_in)) {
                  logger::cout_proxy << "\nParsing INK2R_csv_to_sru_template";
                  for (auto const& field_row : *field_rows) {
                    if (field_row.size()>0) logger::cout_proxy << "\n";
                    for (int i=0;i<field_row.size();++i) {
                      logger::cout_proxy << " [" << i << "]" << field_row[i];
                    }
                  }
                  ink2r_sru_value_map = SKV::SRU::to_sru_value_map(model,model->selected_year_index,*field_rows);

                  // Choose how to get 'årets resultat' 3.26 or 3.27
                  if (false) {
                    // Aggregate the SRU values as defined by INK2R 'resultaträkning'
                    CentsAmount ink2r_year_result{};
                    {
                      ink2r_year_result += to_cents_amount(ink2r_sru_value_map.value()[7410].value_or("0")).value_or(CentsAmount{0});
                      // ...
                      // TODO: Aggregate all SRU codes required for INK2R 'resultaträkning 3.1 .. 3.25
                    }
                    if (ink2r_year_result < CentsAmount{0}) {
                      ink2r_sru_value_map.value()[7550] = to_string(-ink2r_year_result);
                    }
                    else {
                      ink2r_sru_value_map.value()[7450] = to_string(ink2r_year_result);
                    }
                  }
                  else {
                    // Simply use already mapped SRU 7450
                    // For now we asume the SIE file maps 89xx to SRU 7450 and just use that value as required
                    // NOTE: We thus skip the aggregation of 3.1 .. 3.25 on INK2R.
                    //       But if we kept the book OK the value should be the same?

                    // TODO: Consider to implement both alternatives to check them against eachother for safety?

                    if (auto arets_resultat = to_cents_amount(ink2r_sru_value_map.value()[7450].value_or("0"))
                        ;arets_resultat.value_or(CentsAmount{0}) < CentsAmount{0}) {
                      // SRU 7450 is negative. Null and move to SRU 7550
                      ink2r_sru_value_map.value()[7450] = std::nullopt;
                      ink2r_sru_value_map.value()[7550] = to_string(-(arets_resultat.value()));
                      prompt << "\nSRU:7450 (3.26) -> SRU:7550 (3.27) : Förlust öre: " << ink2r_sru_value_map.value()[7550];
                    }
                  }

                  // nullopt all zero SRU values
                  for (auto& [sru_code,maybe_value] : ink2r_sru_value_map.value()) {
                    if (maybe_value.value_or("0") == "0") {
                      maybe_value = std::nullopt;
                    }

                    // TODO: Consider to turn all SRU on 'Resultaträkning' to abs-values (all positive)?
                    //       Some come from 8xxx-acounts and there only the revenue accounts needs sign-flipping to become positive.
                    //       So maybe SKV asumes we should just take the abs-values and put into the form?
                    //       Also see to_sru_value_map(...) where sign flipping is applied somewhat 'hacky' to adress this issue
                  }

                  // Inject selected zero values
                  if (!ink2r_sru_value_map.value()[7368]) {
                    ink2r_sru_value_map.value()[7368] = "0";
                    prompt << "\nSRU:7368 (2.49) Skatteskulder clarified as zero öre: " << ink2r_sru_value_map.value()[7368];
                  }

                }
                else {
                  logger::cout_proxy << "\nSorry, failed to acquire a valid template for the INK2R form";
                }
              }

              SKV::SRU::OptionalSRUValueMap ink2s_sru_value_map{};
              {
                // INK2S_csv_to_sru_template
                std::istringstream is{SKV::SRU::INK2::Y_2024::INK2S_csv_to_sru_template};
                encoding::UTF8::istream utf8_in{is};
                if (auto field_rows = CSV::to_field_rows(utf8_in)) {
                  logger::cout_proxy << "\nParsing INK2S_csv_to_sru_template";
                  for (auto const& field_row : *field_rows) {
                    if (field_row.size()>0) logger::cout_proxy << "\n";
                    for (int i=0;i<field_row.size();++i) {
                      logger::cout_proxy << " [" << i << "]" << field_row[i];
                    }
                  }
                  // Acquire the SRU Values required for the INK2S Form

                  // NOTE: We actually expect no values to be calculated as no INK2S SRU values seems to stem from BAS accounts?
                  // That is, INK2S is 'tax adjustments' and thus a separate domain from the BAS book kept data
                  // For now, call anyhow for the sake of handling INK2R, INK2S and INK2 the 'same' way
                  ink2s_sru_value_map = SKV::SRU::to_sru_value_map(model,model->selected_year_index,*field_rows);

                  // Transer INK2R -> INK2S
                  ink2s_sru_value_map.value()[7650] = ink2r_sru_value_map.value()[7450]; // nullopt or value
                  ink2s_sru_value_map.value()[7750] = ink2r_sru_value_map.value()[7550]; // nullopt or value

                  // TODO: Design a container to hold tax adjustment domain data?
                  //       We could imagine to store the tax return form SRU values for each year.
                  //       Then we could e.g., use last year SRY 7770 as this year SRU 7763
                  ink2s_sru_value_map.value()[7651] = "0"; // Hard coded zero tax paid yet
                  prompt << "\nSRU:7651 (4.3 a) Skatt på årets resultat - hard coded to öre " << ink2s_sru_value_map.value()[7651].value_or("0");

                  if (mock_for_test_2023_2024) {
                    ink2s_sru_value_map.value()[7653] = "62500"; // Hard coded for test 2023-2024
                    prompt << "\nSRU:7653 (4.3 c) Andra bokföra kostnader - hard coded to (2023-2024 test) öre " << ink2s_sru_value_map.value()[7653].value_or("0");

                    ink2s_sru_value_map.value()[7754] = "1500"; // Hard coded for test 2023-2024
                    prompt << "\nSRU:7754 (4.5 c) Andra bokföra intäkter - hard coded to (2023-2024 test) öre " << ink2s_sru_value_map.value()[7754].value_or("0");

                    ink2s_sru_value_map.value()[7763] = "17602600"; // Hard coded 'saved loss from last year SRU:7770 (For test of INK2 2023-2024)
                  }
                  else {
                    // Hard coded for my own 2024-2025 INK2 (to provide for my own need)
                    ink2s_sru_value_map.value()[7763] = "20596300"; // Hard coded 'saved loss from last year SRU:7770
                  }

                  prompt << "\nSRU:7763 (4.14 a) Outnyttjat underskott från föregående år - hard coded to öre: " << ink2s_sru_value_map.value()[7763].value_or("0");

                  CentsAmount ink2s_acc{};
                  {
                    ink2s_acc +=  to_cents_amount(ink2s_sru_value_map.value()[7650].value_or("0")).value_or(CentsAmount{0});
                    ink2s_acc += -to_cents_amount(ink2s_sru_value_map.value()[7750].value_or("0")).value_or(CentsAmount{0});
                    ink2s_acc +=  to_cents_amount(ink2s_sru_value_map.value()[7651].value_or("0")).value_or(CentsAmount{0});
                    ink2s_acc +=  to_cents_amount(ink2s_sru_value_map.value()[7653].value_or("0")).value_or(CentsAmount{0});
                    ink2s_acc += -to_cents_amount(ink2s_sru_value_map.value()[7754].value_or("0")).value_or(CentsAmount{0});
                    ink2s_acc += -to_cents_amount(ink2s_sru_value_map.value()[7763].value_or("0")).value_or(CentsAmount{0});
                  }

                  if (ink2s_acc < CentsAmount{0}) {
                    ink2s_sru_value_map.value()[7770] = to_string(-ink2s_acc);
                  }
                  else {
                    ink2s_sru_value_map.value()[7670] = to_string(ink2s_acc);
                  }

                }
                else {
                  logger::cout_proxy << "\nSorry, failed to acquire a valid template for the INK2S form";
                }
              }

              SKV::SRU::OptionalSRUValueMap ink2_sru_value_map{};
              {
                // ink2_csv_to_sru_template
                std::istringstream is{SKV::SRU::INK2::Y_2024::INK2_csv_to_sru_template};
                encoding::UTF8::istream utf8_in{is};
                if (auto field_rows = CSV::to_field_rows(utf8_in)) {
                  logger::cout_proxy << "\nParsing INK2_csv_to_sru_template";
                  for (auto const& field_row : *field_rows) {
                    if (field_row.size()>0) logger::cout_proxy << "\n";
                    for (int i=0;i<field_row.size();++i) {
                      logger::cout_proxy << " [" << i << "]" << field_row[i];
                    }
                  }
                  // Acquire the SRU Values required for the INK2 Form
                  ink2_sru_value_map = SKV::SRU::to_sru_value_map(model,model->selected_year_index,*field_rows);

                  // Transfer INK2S -> INK2
                  ink2_sru_value_map.value()[7104] = ink2s_sru_value_map.value()[7670]; // nullopt or value
                  ink2_sru_value_map.value()[7114] = ink2s_sru_value_map.value()[7770]; // nullopt or value

                }
                else {
                  logger::cout_proxy << "\nSorry, failed to acquire a valid template for the INK2 form";
                }
              }

              // Process value maps + tag maps
              if (    ink2r_sru_value_map
                  and ink2s_sru_value_map
                  and ink2_sru_value_map) {

                // INFO.SRU tag map
                SKV::SRU::SRUFileTagMap info_sru_file_tag_map{};
                {

                  // See https://skatteverket.se/download/18.96cca41179bad4b1aad958/1636640681760/SKV269_27.pdf
                  // 1. #DATABESKRIVNING_START
                  // 2. #PRODUKT SRU
                  // 3. #MEDIAID (ej obligatorisk)
                  // 4. #SKAPAD (ej obligatorisk)
                  // 5. #PROGRAM (ej obligatorisk)
                  // 6. #FILNAMN (en post)
                  // 7. #DATABESKRIVNING_SLUT
                  // 8. #MEDIELEV_START
                  // 9. #ORGNR
                  info_sru_file_tag_map["#ORGNR"] = model->sie_env_map[model->selected_year_index].organisation_no.CIN;
                  // 10. #NAMN
                  info_sru_file_tag_map["#NAMN"] = model->sie_env_map[model->selected_year_index].organisation_name.company_name;

                  auto postal_address = model->sie_env_map[model->selected_year_index].organisation_address.postal_address; // "17668 J?rf?lla" split in <space> to get ZIP and Town
                  auto postal_address_tokens = tokenize::splits(postal_address,' ');

                  // 11. #ADRESS (ej obligatorisk)
                  // 12. #POSTNR
                  if (postal_address_tokens.size() > 0) {
                    info_sru_file_tag_map["#POSTNR"] = postal_address_tokens[0];
                  }
                  else {
                    info_sru_file_tag_map["#POSTNR"] = "?POSTNR?";
                  }
                  // 13. #POSTORT
                  if (postal_address_tokens.size() > 1) {
                    info_sru_file_tag_map["#POSTORT"] = postal_address_tokens[1];
                  }
                  else {
                    info_sru_file_tag_map["#POSTORT"] = "?POSTORT?";
                  }
                  // 14. #AVDELNING (ej obligatorisk)
                  // 15. #KONTAKT (ej obligatorisk)
                  // 16. #EMAIL (ej obligatorisk)
                  // 17. #TELEFON (ej obligatorisk)
                  // 18. #FAX (ej obligatorisk)
                  // 19. #MEDIELEV_SLUT
                }

                // Create the SRU files mapping for the info.sru + blanketter.sru file pair
                SKV::SRU::FilesMapping fm {
                  .info = info_sru_file_tag_map // info is tags only (no values)
                };

                // Create content for blanketter.sru

                // #BLANKETT INK2R-2024P1
                SKV::SRU::SRUFileTagMap ink2r_sru_file_tag_map{};
                {
                  // #BLANKETT INK2R-2024P1
                  ink2r_sru_file_tag_map["#BLANKETT"] = " INK2R-2025P1"; // See _Nyheter_from_beskattningsperiod_2024P4.pdf

                  // #IDENTITET 165567828172 20240727 195839
                  std::ostringstream os{};
                  os <<  " " << model->sie_env_map[model->selected_year_index].organisation_no.CIN;
                  auto today = to_today();
                  os << " " << today;
                  os << " " << "120000";
                  ink2r_sru_file_tag_map["#IDENTITET"] = os.str();

                  // #NAMN The ITfied AB
                  ink2r_sru_file_tag_map["#NAMN"] = model->sie_env_map[model->selected_year_index].organisation_name.company_name;

                  // #SYSTEMINFO klarmarkerad u. a.
                  // #UPPGIFT 7011 20230501
                  // #UPPGIFT 7012 20240430
                  // #UPPGIFT 7215 42734
                  // #UPPGIFT 7261 2046
                  // #UPPGIFT 7281 119254
                  // #UPPGIFT 7301 100800
                  // #UPPGIFT 7302 39106
                  // #UPPGIFT 7365 -896
                  // #UPPGIFT 7368 0
                  // #UPPGIFT 7369 25024
                  // #UPPGIFT 7410 1
                  // #UPPGIFT 7417 3588
                  // #UPPGIFT 7513 13451
                  // #UPPGIFT 7515 20685
                  // #UPPGIFT 7528 0
                  // #UPPGIFT 7550 30547
                  // #BLANKETTSLUT

                }
                SKV::SRU::Blankett ink2r_blankett{ink2r_sru_file_tag_map,*ink2r_sru_value_map};
                fm.blanketter.push_back(ink2r_blankett); // blankett is tags and values

                // #BLANKETT INK2S-2024P1
                SKV::SRU::SRUFileTagMap ink2s_sru_file_tag_map{};
                {
                  // #BLANKETT INK2S-2024P1
                  ink2s_sru_file_tag_map["#BLANKETT"] = " INK2S-2025P1"; // See _Nyheter_from_beskattningsperiod_2024P4.pdf
                  // #IDENTITET 165567828172 20240727 195839
                  std::ostringstream os{};
                  os <<  " " << model->sie_env_map[model->selected_year_index].organisation_no.CIN;
                  auto today = to_today();
                  os << " " << today;
                  os << " " << "120000";
                  ink2s_sru_file_tag_map["#IDENTITET"] = os.str();

                  // #NAMN The ITfied AB
                  ink2s_sru_file_tag_map["#NAMN"] = model->sie_env_map[model->selected_year_index].organisation_name.company_name;

                  // #SYSTEMINFO klarmarkerad u. a.
                  // #UPPGIFT 7011 20230501
                  // #UPPGIFT 7012 20240430
                  // #UPPGIFT 7651 0
                  // #UPPGIFT 7653 625
                  // #UPPGIFT 7750 30547
                  // #UPPGIFT 7754 15
                  // #UPPGIFT 7763 176026
                  // #UPPGIFT 7770 205963
                  // #UPPGIFT 8041 X
                  // #UPPGIFT 8045 X
                  // #BLANKETTSLUT

                  // NOTE: INK2S carries NO values from BAS accounting!
                  //       These values are all 'skattemässiga justeringar' and needs values from
                  //       the tax / skv domain


                }
                SKV::SRU::Blankett ink2s_blankett{ink2s_sru_file_tag_map,*ink2s_sru_value_map};
                fm.blanketter.push_back(ink2s_blankett); // blankett is tags and values

                SKV::SRU::SRUFileTagMap ink2_sru_file_tag_map{};
                {

                  // #BLANKETT INK2-2024P1
                  ink2_sru_file_tag_map["#BLANKETT"] = " INK2-2025P1"; // See _Nyheter_from_beskattningsperiod_2024P4.pdf
                  // #IDENTITET 165567828172 20240727 195839
                  std::ostringstream os{};
                  os <<  " " << model->sie_env_map[model->selected_year_index].organisation_no.CIN;
                  auto today = to_today();
                  os << " " << today;
                  os << " " << "120000";
                  ink2_sru_file_tag_map["#IDENTITET"] = os.str();

                  // #NAMN The ITfied AB
                  ink2_sru_file_tag_map["#NAMN"] = model->sie_env_map[model->selected_year_index].organisation_name.company_name;

                  // #SYSTEMINFO klarmarkerad u. a.
                  // #UPPGIFT 7011 20230501
                  // #UPPGIFT 7012 20240430
                  // #UPPGIFT 7114 205963
                  // #BLANKETTSLUT
                }
                SKV::SRU::Blankett ink2_blankett{ink2_sru_file_tag_map,*ink2_sru_value_map};
                fm.blanketter.push_back(ink2_blankett); // blankett is tags and values

                // Create the SRU files from the files mapping 'fm'

                // Ensure the ouput folder exists
                std::filesystem::path info_file_path{"to_skv/INK2/SRU/INFO.SRU"};
                std::filesystem::create_directories(info_file_path.parent_path());

                // Create the info.sru file from
                auto info_std_os = std::ofstream{info_file_path};
                SKV::SRU::OStream info_sru_os{info_std_os};
                SKV::SRU::InfoOStream info_os{info_sru_os};

                if (info_os << fm) {
                  prompt << "\nCreated " << info_file_path;
                }
                else {
                  prompt << "\nSorry, FAILED to create " << info_file_path;
                }


                std::filesystem::path blanketter_file_path{"to_skv/INK2/SRU/BLANKETTER.SRU"};
                std::filesystem::create_directories(blanketter_file_path.parent_path());
                auto blanketter_std_os = std::ofstream{blanketter_file_path};
                SKV::SRU::OStream blanketter_sru_os{blanketter_std_os};
                SKV::SRU::BlanketterOStream blanketter_os{blanketter_sru_os};

                if (blanketter_os << fm) {
                  prompt << "\nCreated " << blanketter_file_path;

                  // Note: SKV provides a web interface to test SRU-files at https://www.skatteverket.se/foretag/etjansterochblanketter/allaetjanster/tjanster/filoverforing.4.1f604301062bf0c47e8000527.html
                  // See section "Testa filer
                  //              Testa att dina filer uppfyller Skatteverkets krav på det tekniska formatet. Du behöver ingen e-legitimation för att använda tjänsten."

                }
                else {
                  prompt << "\nSorry, FAILED to create " << blanketter_file_path;
                }

              }
              else {
                prompt << "\nSorry, Failed to acquirer the data for the INK2 forms";
              }

            } break;
          }
        } break;

        case PromptState::EnterIncome:
        case PromptState::EnterDividend:
        case PromptState::EnterHA:
        case PromptState::EnterContact:
        case PromptState::EnterEmployeeID:
        case PromptState::EditAT:
        case PromptState::Undefined:
        case PromptState::Unknown:
          prompt << "\nPlease enter \"word\" like text (index option not available in this state)";
          break;
      }
    }
    /* ======================================================



              Act on on Command?


      ====================================================== */

    // BEGIN LUA REPL Hack
    else if (ast[0] == "-lua") {
      model->prompt_state = PromptState::LUARepl;
    }
    else if (model->prompt_state == PromptState::LUARepl) {
      // Execute input as a lua script
      if (model->L == nullptr) {
        model->L = luaL_newstate();
        luaL_openlibs(model->L);
        // Register cratchit functions in Lua
        lua_faced_ifc::register_cratchit_functions(model->L);
        prompt << "\nNOTE: NEW Lua environment created.";
      }
      int r = luaL_dostring(model->L,command.c_str());
      if (r != LUA_OK) {
        prompt << "\nSorry, ERROR:" << r;
        std::string sErrorMsg = lua_tostring(model->L,-1);
        prompt << " " << std::quoted(sErrorMsg);
      }
    }
    // END LUA REPL Hack

    else if (ast[0] == "-version" or ast[0] == "-v") {
      prompt << "\nCratchit Version " << VERSION;
    }
    else if (ast[0] == "-tas") {
      // Enter tagged Amounts mode for specified period (from any state)
      if (ast.size() == 1 and model->selected_dotas.sequence_size() > 0) {
        // Enter into current selection
        model->prompt_state = PromptState::TAIndex;
      // List current selection
        prompt << "\n<SELECTED>";
        // for (auto const& ta : model->all_dotas.tagged_amounts()) {
        int index = 0;
        for (auto const& ta : model->selected_dotas.ordered_tagged_amounts()) {
          prompt << "\n" << index++ << ". " << ta;
        }
      }
      else {
        // Period required
        OptionalDate begin{}, end{};
        if (ast.size() == 3) {
            begin = to_date(ast[1]);
            end = to_date(ast[2]);
        }
        if (begin and end) {
          model->selected_dotas.clear();
          for (auto const& ta : model->all_dotas.date_range_tagged_amounts({*begin,*end})) {
            model->selected_dotas.dotas_insert_auto_ordered_value(ta);
          }
          model->prompt_state = PromptState::TAIndex;
          prompt << "\n<SELECTED>";
          // for (auto const& ta : model->all_dotas.tagged_amounts()) {
          int index = 0;
          for (auto const& ta : model->selected_dotas.ordered_tagged_amounts()) {
            prompt << "\n" << index++ << ". " << ta;
          }

        }
        else {
          prompt << "\nPlease enter the tagged amounts state on the form '-tas yyyymmdd yyyymmdd'";
        }
      }
    }
    else if (model->prompt_state == PromptState::TAIndex and ast[0] == "-has_tag") {
      if (ast.size() == 2) {
        auto has_tag = [tag = ast[1]](TaggedAmount const& ta) {
          return ta.tags().contains(tag);
        };
        TaggedAmounts reduced{};
        std::ranges::copy(
            model->selected_dotas.ordered_tagged_amounts()
          | std::views::filter(has_tag)
          ,std::back_inserter(reduced)
        );
        // model->selected_dotas = reduced;
        model->selected_dotas.reset(reduced);
      }
      else {
        prompt << "\nPlease provide the tag name you want to filter on";
      }
    }
    else if (model->prompt_state == PromptState::TAIndex and ast[0] == "-has_not_tag") {
      if (ast.size() == 2) {
        auto has_not_tag = [tag = ast[1]](TaggedAmount const& ta) {
          return !ta.tags().contains(tag);
        };
        TaggedAmounts reduced{};
        std::ranges::copy(
            model->selected_dotas.ordered_tagged_amounts()
          | std::views::filter(has_not_tag)
          ,std::back_inserter(reduced)
        );
        // model->selected_dotas = reduced;
        model->selected_dotas.reset(reduced);

      }
      else {
        prompt << "\nPlease provide the tag name you want to filter on";
      }
    }
    else if (model->prompt_state == PromptState::TAIndex and ast[0] == "-is_tagged") {
      if (ast.size() == 2) {
        auto [tag,pattern] = tokenize::split(ast[1],'=');
        if (tag.size()>0) {
          auto is_tagged = [tag=tag,pattern=pattern](TaggedAmount const& ta) {
            const std::regex pattern_regex(pattern);
            return (ta.tags().contains(tag) and std::regex_match(ta.tags().at(tag),pattern_regex));
          };
          TaggedAmounts reduced{};
          std::ranges::copy(
              model->selected_dotas.ordered_tagged_amounts()
            | std::views::filter(is_tagged)
            ,std::back_inserter(reduced)
          );
          // model->selected_dotas = reduced;
          model->selected_dotas.reset(reduced);

        }
        else {
          prompt << "\nPlease provide '<tag name>=<tag_value or regular expression>' to filter on";
        }
      }
      else {
        prompt << "\nPlease provide '<tag name>=<tag_value or regular expression>' to filter on";
      }
    }
    else if (model->prompt_state == PromptState::TAIndex and ast[0] == "-is_not_tagged") {
      if (ast.size() == 2) {
        auto [tag,pattern] = tokenize::split(ast[1],'=');
        if (tag.size()>0) {
          auto is_not_tagged = [tag=tag,pattern=pattern](TaggedAmount const& ta) {
            const std::regex pattern_regex(pattern);
            return (ta.tags().contains(tag)==false or std::regex_match(ta.tags().at(tag),pattern_regex)==false);
          };
          TaggedAmounts reduced{};
          std::ranges::copy(
              model->selected_dotas.ordered_tagged_amounts()
            | std::views::filter(is_not_tagged)
            ,std::back_inserter(reduced)
          );
          // model->selected_dotas = reduced;
          model->selected_dotas.reset(reduced);

        }
        else {
          prompt << "\nPlease provide '<tag name>=<tag_value or regular expression>' to filter on";
        }
      }
      else {
        prompt << "\nPlease provide '<tag name>=<tag_value or regular expression>' to filter on";
      }
    }
    else if (model->prompt_state == PromptState::TAIndex and ast[0] == "-to_bas_account") {
      // -to_bas <BAS account no | Bas Account Name>
      if (ast.size() == 2) {
        if (auto bas_account = BAS::to_account_no(ast[1])) {
          TaggedAmounts created{};
          auto new_ta = [bas_account = *bas_account](TaggedAmount const& ta){
            auto date = ta.date();
            auto cents_amount = ta.cents_amount();
            auto source_tags = ta.tags();
            TaggedAmount::Tags tags{};
            tags["BAS"]=std::to_string(bas_account);
            TaggedAmount result{date,cents_amount,std::move(tags)};
            return result;
          };
          std::ranges::transform(
             model->selected_dotas.ordered_tagged_amounts()
            ,std::back_inserter(created)
            ,new_ta);
          // model->new_dotas = created;
          model->new_dotas.reset(created);
          prompt << "\n<CREATED>";
          // for (auto const& ta : model->all_dotas.tagged_amounts()) {
          int index = 0;
          for (auto const& ta : model->new_dotas.ordered_tagged_amounts()) {
            prompt << "\n\t" << index++ << ". " << ta;
          }
          model->prompt_state = PromptState::AcceptNewTAs;
          prompt << "\n" << options_list_of_prompt_state(model->prompt_state);
        }
        else {
          prompt << "\nPlease enter a valid BAS account no";
        }
      }
      else {
        prompt << "\nPlease enter the BAS account you want to book selected tagged amounts to (E.g., '-to_bas 1920'";
      }
    }
    else if (model->prompt_state == PromptState::TAIndex and ast[0] == "-amount_trails") {
      using AmountTrailsMap = std::map<CentsAmount,TaggedAmounts>;
      AmountTrailsMap amount_trails_map{};
      for (auto const& ta : model->selected_dotas.ordered_tagged_amounts()) {
        amount_trails_map[abs(ta.cents_amount())].push_back(ta);
      }
      std::vector<std::pair<CentsAmount,TaggedAmounts>> date_ordered_amount_trails_map{};
      std::ranges::copy(amount_trails_map,std::back_inserter(date_ordered_amount_trails_map));
      std::ranges::sort(date_ordered_amount_trails_map,[](auto const& e1,auto const& e2){
        if (e1.second.front().date() == e2.second.front().date()) return e1.first < e2.first;
        else return e1.second.front().date() < e2.second.front().date();
      });
      for (auto const& [cents_amount,tas] : date_ordered_amount_trails_map) {
        auto units_and_cents_amount = to_units_and_cents(cents_amount);
        prompt << "\n" << units_and_cents_amount;
        for (auto const& ta : tas) {
          prompt << "\n\t" << ta;
        }
      }
    }
    else if (model->prompt_state == PromptState::TAIndex and ast[0] == "-aggregates") {
      // Reduce to aggregates
      auto is_aggregate = [](TaggedAmount const& ta) {
        return (ta.tags().contains("type") and ta.tags().at("type") == "aggregate");
      };
      TaggedAmounts reduced{};
      std::ranges::copy(
          model->selected_dotas.ordered_tagged_amounts()
        | std::views::filter(is_aggregate)
        ,std::back_inserter(reduced)
      );
      // model->selected_dotas = reduced;
      model->selected_dotas.reset(reduced);

      // List by bucketing on aggregates (listing orphan (non-aggregated) tagged amounts separatly)
      std::cout << "\n<AGGREGATES>" << std::flush;
      prompt << "\n<AGGREGATES>";
      for (auto const& ta : model->selected_dotas.ordered_tagged_amounts()) {
        prompt << "\n" << ta;
        if (auto members_value = ta.tag_value("_members")) {
          auto members = Key::Sequence{*members_value};
          if (auto value_ids = to_maybe_value_ids(members)) {
            prompt << "\n\t<members>";
            if (auto tas = model->all_dotas.to_tagged_amounts(*value_ids)) {
              for (auto const& ta : *tas) {
                prompt << "\n\t" << ta;
              }
            }
          }
        }
      }
    }
    else if (model->prompt_state == PromptState::TAIndex and ast[0] == "-to_hads") {
      prompt << "\nCreating Heading Amount Date entries (HAD:s) from selected Tagged Amounts";
      auto had_candidates_tas =
          model->selected_dotas.ordered_tagged_amounts()
        | std::views::filter([&](auto const& ta){
            // Filter out SIE and BAS entries
            if (ta.tag_value("SIE").has_value()) return false;
            if (ta.tag_value("BAS").has_value()) return false;
            return true; // keep this ta
          })
        | std::ranges::to<TaggedAmounts>();

      for (auto const& ta : had_candidates_tas) {
        if (auto maybe_had = to_had(ta)) {
          model->heading_amount_date_entries.push_back(maybe_had.value());
        }
        else {
          prompt << "\nSORRY, failed to turn tagged amount into a heading amount date entry" << ta;
        }
      }
    }
    else if (model->prompt_state == PromptState::TAIndex and ast[0] == "-todo") {
      // Filter out tagged amounts that are already in the books as an SIE entry / aggregate?
      prompt << "\nSorry, Identifying todos on tagged amounts not yet implemented";
    }
    else if (ast[0] == "-bas") {
// std::cout << " :)";
      if (ast.size() == 2) {
        // Import bas account plan csv file
        if (ast[1] == "?") {
          // List csv-files in the resources sub-folder
          std::filesystem::path resources_folder{"./resources"};
          std::filesystem::directory_iterator dir_iter{resources_folder};
          for (auto const& dir_entry : std::filesystem::directory_iterator{resources_folder}) {
            prompt << "\n" << dir_entry.path(); // TODO: Remove .path() when stdc++ library supports streaming of std::filesystem::directory_entry
          }
        }
        else {
          // Import and parse file at provided file path
          std::filesystem::path csv_file_path{ast[1]};
          if (std::filesystem::exists(csv_file_path)) {
            std::ifstream in{csv_file_path};
            BAS::parse_bas_account_plan_csv(in,prompt);
          }
          else {
            prompt << "\nUse '-bas' to list available files to import (It seems I can't find the file " << csv_file_path;
          }
        }
      }
      else {
        // Use string literal with csv data
        std::istringstream in{BAS::bas_2022_account_plan_csv};
        BAS::parse_bas_account_plan_csv(in,prompt);
      }
    }
    else if (ast[0] == "-sie") {
      // Import sie and add as base of our environment
      if (ast.size()==1) {
        // List current sie environment
        prompt << model->sie_env_map["current"];
        // std::cout << model->sie_env_map["current"];
      }
      else if (ast.size()==2) {
        if (ast[1]=="*") {
          // List unposted (staged) sie entries
          FilteredSIEEnvironment filtered_sie{model->sie_env_map["current"],BAS::filter::is_flagged_unposted{}};
          prompt << filtered_sie;
        }
        else if (model->sie_env_map.contains(ast[1])) {
          // List journal entries of a year index
          prompt << model->sie_env_map[ast[1]];
        }
        else if (auto bas_account_no = BAS::to_account_no(ast[1])) {
          // List journal entries functional::text::filtered on an bas account number
          prompt << "\nFilter journal entries that has a transaction to account no " << *bas_account_no;
          prompt << "\nTIP: If you meant filter on amount please re-enter using '.00' to distinguish it from an account no.";
          FilteredSIEEnvironment filtered_sie{model->sie_env_map["current"],BAS::filter::HasTransactionToAccount{*bas_account_no}};
          prompt << filtered_sie;
        }
        else if (auto gross_amount = to_amount(ast[1])) {
          // List journal entries functional::text::filtered on given amount
          prompt << "\nFilter journal entries that match gross amount " << *gross_amount;
          FilteredSIEEnvironment filtered_sie{model->sie_env_map["current"],BAS::filter::HasGrossAmount{*gross_amount}};
          prompt << filtered_sie;
        }
        else if (auto sie_file_path = path_to_existing_file(ast[1])) {
          // #1 command '-sie file-name' -> register "current" SIE file as provided name
          //    "current" is a place holder (no checks against actual current date and time)
          SIEEnvironmentsMap::RelativeYearKey year_key{"current"};
          prompt << "\nImporting SIE to current year from " << *sie_file_path;
          // auto update_posted_result = model->sie_env_map.update_posted_from_file(year_key,*sie_file_path);
          auto md_maybe_istream = persistent::in::to_md_maybe_istream(*sie_file_path);
          auto update_posted_result = model->sie_env_map.update_posted_from_md_istream(
             year_key
            ,md_maybe_istream);

          prompt << zeroth::to_user_cli_feedback(model,year_key,update_posted_result);

          // if (auto sie_env = sie_from_sie_file(*sie_file_path)) {
          //   model->sie_env_map["current"] = std::move(*sie_env);
          //   // Update the list of staged entries
          //   if (auto sse = sie_from_sie_file(model->sie_env_map["current"].staged_sie_file_path())) {
          //     // #2 staged_sie_file_path is the path to SIE entries NOT in "current" import
          //     //    That is, asumed to be added or edited  by cratchit (and not yet known by external tool)
          //     auto staged = model->sie_env_map["current"].stage(*sse);
          //     auto unposted = model->sie_env_map["current"].unposted();
          //     if (unposted.size() > 0) {
          //       prompt << "\n<UNPOSTED>";
          //       prompt << unposted;
          //     }
          //     else {
          //       prompt << "\nAll staged entries are now posted OK";
          //     }
          //   }
          // }
          // else {
          //   // failed to parse sie-file into an SIE Environment
          //   prompt << "\nERROR - Failed to import sie file " << *sie_file_path;
          // }
        }
        else if (ast[1] == "-types") {
          // Group on Type Topology
          auto meta_entry_topology_map = to_meta_entry_topology_map(model->sie_env_map);
          // Prepare to record journal entries we could not use as template for new entries
          std::vector<BAS::MDTypedJournalEntry> failed_tmes{};
          std::set<BAS::kind::AccountTransactionTypeTopology> detected_topologies{};
          // List grouped on type topology
          for (auto const& [signature,tme_map] : meta_entry_topology_map) {
            for (auto const& [type_topology,tmes] : tme_map) {
              prompt << "\n[" << type_topology << "] ";
              detected_topologies.insert(type_topology);
              // Group tmes on BAS Accounts topology
              auto accounts_topology_map = to_accounts_topology_map(tmes);
              // List grouped BAS Accounts topology
              for (auto const& [signature,bat_map] : accounts_topology_map) {
                for (auto const& [type_topology,tmes] : bat_map) {
                  prompt << "\n    [" << type_topology << "] ";
                  for (auto const& tme : tmes) {
                    prompt << "\n       VAT Type:" << to_vat_type(tme);
                    prompt << "\n      " << tme.meta << " " << std::quoted(tme.defacto.caption) << " " << tme.defacto.date;
                    prompt << IndentedOnNewLine{tme.defacto.account_transactions,10};
                    // TEST that we are able to operate on journal entries with this topology?
                    auto test_result = test_typed_meta_entry(model->sie_env_map,tme);
                    prompt << "\n       TEST: " << test_result;
                    if (test_result.failed) failed_tmes.push_back(tme);
                  }
                }
              }
            }
          }
          // LOG detected journal entry type topologies
          prompt << "\n<DETECTED TOPOLOGIES>";
          for (auto const& topology : detected_topologies) {
            prompt << "\n\t" << topology;
          }
          // LOG the 'tmes' (template meta entries) we failed to identify as templates for new journal entries
          prompt << "\n<DESIGN INSUFFICIENCY: FAILED TO IDENTIFY AND USE THESE ENTRIES AS TEMPLATE>";
          for (auto const& tme : failed_tmes) {
            auto types_topology = BAS::kind::to_types_topology(tme);
            prompt << "\n" << types_topology << " " << tme.meta << " " << tme.defacto.caption << " " << tme.defacto.date;
          }
        }
        else {
          // assume user search criteria on transaction heading and comments
          FilteredSIEEnvironment filtered_sie{model->sie_env_map["current"],BAS::filter::matches_user_search_criteria{ast[1]}};
          prompt << "\nNot '*', existing year id or existing file: " << std::quoted(ast[1]);
          prompt << "\nFilter current sie for " << std::quoted(ast[1]);
          prompt << filtered_sie;
        }
      }
      else if (ast.size()==3) {
        auto year_key = ast[1];
        if (ast[2]=="*") {
          // List unposted (staged) sie entries
          FilteredSIEEnvironment filtered_sie{model->sie_env_map[year_key],BAS::filter::is_flagged_unposted{}};
          prompt << filtered_sie;
        }
        else if (auto sie_file_path = path_to_existing_file(ast[2])) {
          prompt << "\nImporting SIE to realtive year " << year_key << " from " << *sie_file_path;
          // auto update_posted_result = model->sie_env_map.update_posted_from_file(year_key,*sie_file_path);
          auto md_maybe_istream = persistent::in::to_md_maybe_istream(*sie_file_path);
          auto update_posted_result = model->sie_env_map.update_posted_from_md_istream(
             year_key
            ,md_maybe_istream);

          prompt << zeroth::to_user_cli_feedback(model,year_key,update_posted_result);

          // if (auto sie_env = sie_from_sie_file(*sie_file_path)) {
          //   model->sie_env_map[year_key] = std::move(*sie_env);
          //   if (auto sse = sie_from_sie_file(model->sie_env_map[year_key].staged_sie_file_path())) {
          //     auto staged = model->sie_env_map[year_key].stage(*sse);
          //     auto unposted = model->sie_env_map[year_key].unposted();
          //     if (unposted.size() > 0) {
          //       prompt << "\n<UNPOSTED>";
          //       prompt << unposted;
          //     }
          //     else {
          //       prompt << "\nAll staged entries are now posted OK";
          //     }
          //   }
          // }
          // else {
          //   // failed to parse sie-file into an SIE Environment
          //   prompt << "\nERROR - Failed to import sie file " << *sie_file_path;
          // }
        }
        else {
          // assume user search criteria on transaction heading and comments
          if (model->sie_env_map.contains(year_key)) {
            FilteredSIEEnvironment filtered_sie{model->sie_env_map[year_key],BAS::filter::matches_user_search_criteria{ast[2]}};
            prompt << filtered_sie;
          }
          else {
            prompt << "\nYear identifier " << year_key << " is not associated with any data";
          }
        }
      }
      else {
        prompt << "\nSorry, failed to understand your request.";
        prompt << "\n\tI interpreted your input to contain " << ast.size() << " arguments:";
        for (auto const& token : ast) prompt << " " << std::quoted(token);
        prompt << "\n\tPlease enclose arguments (e.g., an SIE file path) that contains space(s) with \"\"";
        prompt << "\n\tPress <Enter> for additional help";
      }
    }
    else if (ast[0] == "-huvudbok") {
      // command "-huvudbok" defaults to "-huvudbok 0", that is the fiscal year of current date
      std::string relative_year_key = (ast.size() == 2)?ast[1]:std::string{"current"};
      if (relative_year_key == "0") relative_year_key = "current";
      prompt << "\nHuvudbok for year id " << relative_year_key;
      auto financial_year_date_range = model->to_financial_year_date_range(relative_year_key);
      if (financial_year_date_range) {
        prompt << " " << *financial_year_date_range;
        TaggedAmounts tas{}; // journal entries
        auto is_journal_entry = [](TaggedAmount const& ta) {
          return (ta.tags().contains("parent_SIE") or ta.tags().contains("IB"));
        };
        std::ranges::copy(model->all_dotas.date_range_tagged_amounts(*financial_year_date_range) | std::views::filter(is_journal_entry),std::back_inserter(tas));
        // 2. Group tagged amounts into same BAS account and a list of parent_SIE
        std::map<BAS::AccountNo,TaggedAmounts> huvudbok{};
        std::map<BAS::AccountNo,CentsAmount> opening_balance{};

        for (auto const& ta : tas) {
          if (ta.tags().contains("BAS")) {
            auto opt_bas_account_no = BAS::to_account_no(ta.tags().at("BAS"));
            if (opt_bas_account_no) {
              if (ta.tags().contains("IB")) {
                opening_balance[*opt_bas_account_no] = ta.cents_amount();
              }
              else {
                huvudbok[*opt_bas_account_no].push_back(ta);
              }
            }
            else {
              std::cout << "\nDESIGN INSUFFICIENCY: Error, BAS tag contains value:" << ta.tags().at("BAS") << " that fails to translate to a BAS account no";
            }
          }
          else {
            std::cout << "\nDESIGN_INSUFFICIENCY: Error, Contains no BAS tag : " << ta;
          }
        }

        // 3. "print" the table grouping BAS accounts and date ordered SIE verifications (also showing a running accumultaion of the account balance)
        prompt << "\n<Journal Entries in year id " << relative_year_key << ">";
        for (auto const& [bas_account_no,tas] : huvudbok) {
          CentsAmount acc{0};
          prompt << "\n" << bas_account_no;
          prompt << ":" << std::quoted(BAS::global_account_metas().at(bas_account_no).name);
          if (opening_balance.contains(bas_account_no)) {
            acc = opening_balance[bas_account_no];
            prompt << "\n\topening balance\t" << to_units_and_cents(acc);
          }
          for (auto const& ta : tas) {
            prompt << "\n\t" << ta;
            acc += ta.cents_amount();
            prompt << "\tsaldo:" << to_units_and_cents(acc);
          }
          prompt << "\n\tclosing balance\t" << to_units_and_cents(acc);
        }
      }
      else {
        prompt << "\nSORRY, Failed to understand what fiscal year " << relative_year_key << " refers to. Please enter a valid year id (0 or 'current', -1 = previous fiscal year...)";
      }
    }
    else if (ast[0] == "-balance") {
      // The user has requested to have us list account balances
      prompt << model->sie_env_map["current"].balances_at(to_today());
    }
    else if (ast[0] == "-hads") {
      auto hads = model->refreshed_hads();
      if (ast.size()==1) {
        prompt << to_had_listing_prompt(hads);
        model->prompt_state = PromptState::HADIndex;

      // 	// Expose current hads (Heading Amount Date transaction entries) to the user
      // 	auto& hads = model->heading_amount_date_entries;
      // 	unsigned int index{0};
      // 	std::vector<std::string> sHads{};
      // 	std::transform(hads.begin(),hads.end(),std::back_inserter(sHads),[&index](auto const& had){
      // 		std::stringstream os{};
      // 		os << index++ << " " << had;
      // 		return os.str();
      // 	});
      // 	prompt << "\n" << std::accumulate(sHads.begin(),sHads.end(),std::string{"Please select:"},[](auto acc,std::string const& entry) {
      // 		acc += "\n  " + entry;
      // 		return acc;
      // 	});
      // 	model->prompt_state = PromptState::HADIndex;
      }
      else if (ast.size()==2) {
        // Assume the user has entered text to macth against had Heading
        // Expose the hads (Heading Amount Date transaction entries) that matches user input
        // NOTE: Keep correct index for later retreiving any had selected by the user
        auto& hads = model->heading_amount_date_entries;
        unsigned int index{0};
        std::vector<std::string> sHads{};
        auto text = ast[1];
        std::transform(hads.begin(),hads.end(),std::back_inserter(sHads),[&index,&text](auto const& had){
          std::stringstream os{};
          if (strings_share_tokens(text,had.heading)) os << index << " " << had;
          ++index; // count even if not listed
          return os.str();
        });
        prompt << std::accumulate(sHads.begin(),sHads.end(),std::string{"Please select:"},[](auto acc,std::string const& entry) {
          if (entry.size()>0) acc += "\n  " + entry;
          return acc;
        });
        model->prompt_state = PromptState::HADIndex;
      }
      else {
        prompt << "\nPlease re-enter a valid input (It seems you entered to many arguments for me to understand)";
      }
    }
    else if (ast[0] == "-meta") {
      if (ast.size() > 1) {
        // Assume filter on provided text
        if (auto to_match_account_no = BAS::to_account_no(ast[1])) {
          auto ams = matches_bas_or_sru_account_no(*to_match_account_no,model->sie_env_map["current"]);
          for (auto const& [account_no,am] : ams) {
            prompt << "\n  " << account_no << " " << std::quoted(am.name);
            if (am.sru_code) prompt << " SRU:" << *am.sru_code;
          }
        }
        else {
          auto ams = matches_bas_account_name(ast[1],model->sie_env_map["current"]);
          for (auto const& [account_no,am] : ams) {
            prompt << "\n  " << account_no << " " << std::quoted(am.name);
            if (am.sru_code) prompt << " SRU:" << *am.sru_code;
          }
        }
      }
      else {
        // list all metas
        for (auto const& [account_no,am] : model->sie_env_map["current"].account_metas()) {
          prompt << "\n  " << account_no << " " << std::quoted(am.name);
          if (am.sru_code) prompt << " SRU:" << *am.sru_code;
        }
      }
    }
    else if (ast[0] == "-sru") {
      if (ast.size() > 1) {
        if (ast[1] == "-bas") {
          // List SRU Accounts mapped to BAS Accounts
          auto const& account_metas = model->sie_env_map["current"].account_metas();
          auto sru_map = sru_to_bas_map(account_metas);
          for (auto const& [sru_account,bas_accounts] : sru_map) {
            prompt << "\nSRU:" << sru_account;
            for (auto const& bas_account_no : bas_accounts) {
              prompt << "\n  BAS:" << bas_account_no << " " << std::quoted(account_metas.at(bas_account_no).name);
            }
          }
        }
        else if (ast.size() > 2) {
          // Assume the user has provided a year-id and a path to a csv-file with SRU values for that year?
          auto relative_year_key = ast[1];
          std::filesystem::path csv_file_path{ast[2]};
          if (std::filesystem::exists(csv_file_path)) {
            std::ifstream ifs{csv_file_path};
            encoding::UTF8::istream utf8_in{ifs}; // Assume UTF8 encoded file (should work for all-digits csv.file as ASCII 0..7F overlaps with UTF8 encoing anyhow?)
            if (auto const& field_rows = CSV::to_field_rows(utf8_in,';')) {
              for (auto const& field_row : *field_rows) {
                if (field_row.size()==2) {
                  if (auto const& sru_code = SKV::SRU::to_account_no(field_row[0])) {
                    model->sru[relative_year_key].set(*sru_code,field_row[1]);
                  }
                }
              }
            }
            else {
              prompt << "\nSorry, seems to be unable to parse for SRU values in csv-file " << csv_file_path;
            }
          }
          else {
            prompt << "\nSorry, I seem to fail to find file " << csv_file_path;
          }
        }
        else {
          prompt << "\nPlease provide no arguments, argument '-bas' or arguments <year-id> <csv-file-path>";
        }
      }
      else {
        for (auto const& [relative_year_key,sru_env] : model->sru) {
          prompt << "\nyear:id:" << relative_year_key;
          prompt << "\n" << sru_env;
        }
      }
    }
    else if (ast[0] == "-gross") {
      auto ats = to_gross_account_transactions(model->sie_env_map);
      for (auto const& at : ats) {
        prompt << "\n" << at;
      }
    }
    else if (ast[0] == "-net") {
      auto ats = to_net_account_transactions(model->sie_env_map);
      for (auto const& at : ats) {
        prompt << "\n" << at;
      }
    }
    else if (ast[0] == "-vat") {
      auto vats = to_vat_account_transactions(model->sie_env_map);
      for (auto const& vat : vats) {
        prompt << "\n" << vat;
      }
    }
    else if (ast[0] == "-t2") {
      auto t2s = t2_entries(model->sie_env_map);
      prompt << t2s;
    }
    else if (ast[0] == "-skv") {
      if (ast.size() == 2 and model->sie_env_map.contains(ast[1])) {
        model->selected_year_index = ast[1];
        prompt << "\nTill Skatteverket för år " << ast[1];
        // List skv options
        prompt << "\n0: Arbetsgivardeklaration (TAX Returns)";
        prompt << "\n1: Momsrapport (VAT Returns)";
        prompt << "\n2: K10 (TAX Declaration Appendix Form)";
        prompt << "\n3: INK1 + K10 (Swedish Tax Agency private TAX Form + Dividend Form";
        prompt << "\n4: INK2 + INK2S + INK2R (Company Tax Returns form(s))";
        model->prompt_state = PromptState::SKVEntryIndex;
      }
      else {
        prompt << "\nSorry, please enter '-skv <year_index>' e.g.,'-skv -1' ";
      }
    }
    else if (ast[0] == "-csv") {
      if (ast.size()==1) {
        // Command "-csv" (and no more)
        // ==> Process all *.csv in folder from_bank and *.skv in folder from_skv

      }
      else if ((ast.size()>2) and (ast[1] == "-had")) {
        // Command "-csv -had <path>"
        std::filesystem::path csv_file_path{ast[2]};
        if (std::filesystem::exists(csv_file_path)) {
          std::ifstream ifs{csv_file_path};
          CSV::NORDEA::istream in{ifs};
          BAS::OptionalAccountNo gross_bas_account_no{};
          if (ast.size()>3) {
            if (auto bas_account_no = BAS::to_account_no(ast[3])) {
              gross_bas_account_no = *bas_account_no;
            }
            else {
              prompt << "\nPlease enter a valid BAS account no for gross amount transaction. " << std::quoted(ast[3]) << " is not a valid BAS account no";
            }
          }
          auto hads = CSV::from_stream(in,gross_bas_account_no);
          // #X Filter entries in the read csv-file against already existing hads and sie-entries
          // #X match date and amount
          // 1) Don't add a had that is already in our models hads list
          std::erase_if(hads,[this](HeadingAmountDateTransEntry const& had) {
            auto iter = std::find_if(this->model->heading_amount_date_entries.begin(),this->model->heading_amount_date_entries.end(),[&had](HeadingAmountDateTransEntry const& other){
              auto result = (had.date == other.date) and (had.amount == other.amount) and (had.heading == other.heading);
              if (result) {
                std::cout << "\nHAD ALREADY AS HAD";
                std::cout << "\n\t" << had;
                std::cout << "\n\t" << other;
              }
              return result;
            });
            auto result = (iter != this->model->heading_amount_date_entries.end());
            return result;
          });
          // 2) Don't add a had that is already accounted for as an sie entry
          auto sie_hads_reducer = [&hads](BAS::MDJournalEntry const& mdje) {
            // Remove the had if it matches me
            std::erase_if(hads,[&mdje](HeadingAmountDateTransEntry const& had) {
              bool result{false};
              if (mdje.defacto.date == had.date) {
                if (had.amount > 0) {
                  auto gross_amount = to_positive_gross_transaction_amount(mdje.defacto);
                  result = (BAS::to_cents_amount(had.amount) == BAS::to_cents_amount(gross_amount));
                }
                else {
                  auto gross_amount = to_negative_gross_transaction_amount(mdje.defacto);
                  result = (BAS::to_cents_amount(had.amount) == BAS::to_cents_amount(gross_amount));
                }
              }
              if (result) {
                std::cout << "\nHAD ALREADY AS SIE: ";
                std::cout << "\n\t" << had;
                std::cout << "\n\t" << mdje;
              }
              return result;
            });
          };
          for_each_md_journal_entry(model->sie_env_map["current"],sie_hads_reducer);
          std::copy(hads.begin(),hads.end(),std::back_inserter(model->heading_amount_date_entries));
        }
        else {
          prompt << "\nPlease provide a path to an existing file. Can't find " << csv_file_path;
        }
      }
      else if ((ast.size()>2) and (ast[1] == "-sru")) {
        // Command "-csv -sru <path>"
        std::filesystem::path csv_file_path{ast[2]};
        if (std::filesystem::exists(csv_file_path)) {
          std::ifstream ifs{csv_file_path};
          encoding::UTF8::istream utf8_in{ifs};
          if (auto field_rows = CSV::to_field_rows(utf8_in)) {
            for (auto const& field_row : *field_rows) {
              if (field_row.size()>0) prompt << "\n";
              for (int i=0;i<field_row.size();++i) {
                prompt << " [" << i << "]" << field_row[i];
              }
            }
            if (auto sru_value_map = SKV::SRU::to_sru_value_map(model,model->selected_year_index,*field_rows)) {
              prompt << "\nSorry, Reading an input csv-file as base for SRU-file creation is not yet implemented.";
            }
            else {
              prompt << "\nSorry, failed to gather required sru values to create a valid sru-file";
            }
          }
          else {
            prompt << "\nSorry, failed to parse as csv the file " << csv_file_path;
          }
        }
        else {
          prompt << "\nPlease provide a path to an existing file. Can't find " << csv_file_path;
        }
      }
      else {
        prompt << "\nPlease provide '-had' or '-sru' followed by a path to a csv-file";
      }
    }
    else if (ast[0] == "-") {
      // model->prompt_state = PromptState::Root;
      auto new_state = model->to_previous_state(model->prompt_state);
      prompt << model->to_prompt_for_entering_state(new_state);
      model->prompt_state = new_state;
    }
    else if (ast[0] == "-omslutning") {
      // Report yearly change for each BAS account
      std::string relative_year_key = "current"; // default
      if (ast.size()>1) {
        if (model->sie_env_map.contains(ast[1])) {
          relative_year_key = ast[1];
        }
        else {
          prompt << "\nSorry, I find no record of year " << std::quoted(ast[1]);
        }
      }

      /*
      Omslutning 20230501...20240430 {
        <Konto>      <IN>  <period>     <OUT>
            1920  36147,89  -7420,27  28727,62
            1999    156,75   -156,75      0,00
            2098      0,00  84192,50  84192,50
            2099  84192,50 -84192,50      0,00
            2440  -5459,55      0,00  -5459,55
            2641   1331,46    156,75   1488,21
            2893  -2601,86   2420,27   -181,59
            2898  -5000,00   5000,00      0,00
            9000                0,00      0,00
      } // Omslutning
      */

      auto financial_year_date_range = model->to_financial_year_date_range(relative_year_key);
      auto bas_omslutning_map = to_ib_period_ub(model,relative_year_key);
      if (financial_year_date_range) {
        prompt << "\nOmslutning " << *financial_year_date_range << " {";
        prompt << "\n" << std::setfill(' ');
        auto w = 12;
        prompt << std::setw(w) << "<Konto>";
        prompt << "\t" << std::setw(w) << "<IB>";
        prompt << "\t" << std::setw(w) << "<period>";
        prompt << "\t" << std::setw(w) <<  "<UB>";

        for (auto const& [bas_account_no,entry] : bas_omslutning_map) {
          prompt << "\n";
          prompt << std::setw(w) << bas_account_no;
          prompt << "\t" << std::setw(w) << to_string(to_units_and_cents(entry.ib));
          prompt << "\t" << std::setw(w) << to_string(to_units_and_cents(entry.period));
          prompt << "\t" << std::setw(w) << to_string(to_units_and_cents(entry.ub));
        }

        prompt << "\n} // Omslutning";

      }
      else {
        prompt << "\nTry '-omslutning' with no argument for current year or enter a valid fiscal year id 'current','-1','-2',...";
      }
    }
    else if (ast[0] == "-ar_vs_bas") {
      auto ar_entries = BAS::K2::AR::parse(BAS::K2::AR::ar_online::bas_2024_mapping_to_k2_ar_text);
      std::cout << "\nÅR <--> BAS Accounts table {";
      std::cout << "\n\tÅR\tBAS Range";
      for (auto const& ar_entry : ar_entries) {
        std::cout << "\n\t" << ar_entry.m_field_heading_text;
        std::cout << "\t" << ar_entry.m_bas_accounts_text;
      }
      std::cout << "\n} // ÅR <--> BAS Accounts table";
    }
    else if (ast[0] == "-plain_ar") {
      // Brute force an Annual Financial Statement as defined by Swedish Bolagsverket
      // Parse BAS::K2::bas_2024_mapping_to_k2_ar_text to get mapping of BAS account saldos
      // to Swedish Bolagsverket Annual Financial Statement ("Årsredovisning") according to K2 rules

      // So BAS::K2::bas_2024_mapping_to_k2_ar_text maps AR <--> BAS
      // So we can use it to accumulate saldos for AR fields from the specified BAS accounts over the fiscal year


      // Create tagged amounts that aggregates BAS Accounts to a saldo and AR=<AR Field ID> ARTEXT=<AR Field Heading> ARCOMMENT=<AR Field Description>
      // and the aggregates BAS accounts to accumulate for this AR Field Saldo - members=id;id;id;...

      auto ar_entries = BAS::K2::AR::parse(BAS::K2::AR::ar_online::bas_2024_mapping_to_k2_ar_text);
      auto financial_year_date_range = model->sie_env_map["-1"].financial_year_date_range();

      if (false and financial_year_date_range) {
        auto financial_year_tagged_amounts = model->all_dotas.date_range_tagged_amounts(*financial_year_date_range);
        auto bas_account_accs = tas::to_bas_omslutning(financial_year_tagged_amounts);

        if (true) {
          // Log Omslutning
          std::cout << "\nOmslutning {";
          for (auto const& ta : bas_account_accs) {
            auto omslutning = to_units_and_cents(ta.cents_amount());
            std::string bas_account_string = ta.tags().at("BAS");
            std::cout << "\n\tkonto:" << bas_account_string;
            auto ib = model->sie_env_map["-1"].opening_balance_of(*BAS::to_account_no(bas_account_string));
            if (ib) {
              auto ib_units_and_cents = to_units_and_cents(to_cents_amount(*ib));
              std::cout << " IB:" << to_string(ib_units_and_cents);
              std::cout << " omslutning:" << to_string(omslutning);
              std::cout << " UB:" << to_string(to_units_and_cents(to_cents_amount(*ib) + ta.cents_amount()));
            }
            else {
              std::cout << " omslutning:" << to_string(omslutning);
            }
          }
          std::cout << "\n} // Omslutning";
        }

        auto not_accumulated = bas_account_accs;
        for (auto const& ta : bas_account_accs) {
          for (auto& ar_entry : ar_entries) {
            if (ta.tags().contains("BAS")) {
              if (auto bas_account_no = BAS::to_account_no(ta.tags().at("BAS"))) {
                if (ar_entry.accumulate_this_bas_account(*bas_account_no,ta.cents_amount())) {
                  if (auto iter = std::find(not_accumulated.begin(),not_accumulated.end(),ta); iter != not_accumulated.end()) {
                    not_accumulated.erase(iter);
                  }
                }
              }
              else {
                std::cout << "\nDESIGN INSUFFICIENCY: A tagged amount has a BAS tag set to an invalid (Non BAS account no) value " << ta.tags().at("BAS");
              }
            }
            else {
              // skip, this tagged amount  is NOT a transaction to a BAS account
            }
          }
        }
        prompt << "\nÅrsredovisning {";
        for (auto& ar_entry : ar_entries) {
          if (ar_entry.m_amount != 0) {
            prompt << "\n\tfält:" << ar_entry.m_field_heading_text << " belopp:" << to_string(to_units_and_cents(ar_entry.m_amount));
            prompt << " " << std::quoted(ar_entry.m_bas_accounts_text) << " matched:";
            for (auto const& bas_account_no : ar_entry.m_bas_account_nos) {
              prompt << " " << bas_account_no;
            }
          }
        }
        prompt << "\n} // Årsredovisning";

        prompt << "\nNOT Accumulated {";
        for (auto const& ta : not_accumulated) {
          prompt << "\n\t" << ta;
        }
        prompt << "\n} // NOT Accumulated";
      }
      else {
        prompt << "\nSORRY, I seem to have lost track of last fiscal year first and last dates?";
        prompt << "\nCheck that you have used the '-sie' command to import an sie file with that years data from another application?";
      }
    }
    else if (ast[0] == "-plain_ink2") {
      // SKV Tax return according to K2 rules (plain text)
      //

      // Parse BAS::SRU::INK2_19_P1_intervall_vers_2_csv to get a mapping between SRU Codes and field designations on SKV TAX Return and BAS Account ranges
      // From https://www.bas.se/kontoplaner/sru/
      // Also in resources/INK2_19_P1-intervall-vers-2.csv

      // Create tagged amounts that aggregates BAS Accounts to a saldo and SRU=<SRU code> TAX_RETURN_ID=<SKV Tax Return form box id>
      // and the aggregates BAS accounts to accumulate for this Tax Return Field Saldo - members=id;id;id;...

      BAS::SRU::INK2::parse(BAS::SRU::INK2::INK2_19_P1_intervall_vers_2_csv);

    }
    else if (ast[0] == "-doc_ar") {
      // Assume the user wants to generate an annual report
      // ==> The first document seems to be the  1) financial statements approval (fastställelseintyg) ?
      doc::Document annual_report_financial_statements_approval{};
      {
        annual_report_financial_statements_approval << doc::plain_text("Fastställelseintyg");
      }

      doc::Document annual_report{};
      // ==> The second document seems to be the 2) directors’ report  (förvaltningsberättelse)?
      auto annual_report_front_page = doc::separate_page();
      {
        *annual_report_front_page << doc::plain_text("Årsredovisning");
      }
      auto annual_report_directors_report = doc::separate_page();
      {
        *annual_report_directors_report << doc::plain_text("Förvaltningsberättelse");
      }
      // ==> The third document seems to be the 3)  profit and loss statement (resultaträkning)?
      auto annual_report_profit_and_loss_statement = doc::separate_page();
      {
        *annual_report_profit_and_loss_statement << doc::plain_text("Resultaträkning");
      }
      // ==> The fourth document seems to be the 4) balance sheet (balansräkning)?
      auto annual_report_balance_sheet = doc::separate_page();
      {
        *annual_report_balance_sheet << doc::plain_text("Balansräkning");
      }
      // ==> The fifth document seems to be the 5) notes (noter)?
      auto annual_report_annual_report_notes = doc::separate_page();
      {
        *annual_report_annual_report_notes << doc::plain_text("Noter");
      }

      std::filesystem::path to_bolagsverket_file_folder{"to_bolagsverket"};

      {
        // Generate documents in RTF format
        auto annual_report_file_folder = to_bolagsverket_file_folder / "rtf";

        // Create one document for the financial statemnts approval (to accomodate the annual report copy sent to Swedish Bolagsverket)
        {
          auto annual_report_file_path = annual_report_file_folder / "financial_statement_approval.rtf";
          std::filesystem::create_directories(annual_report_file_path.parent_path());
          std::ofstream raw_annual_report_os{annual_report_file_path};

          RTF::OStream annual_report_os{raw_annual_report_os};

          annual_report_os << annual_report_financial_statements_approval;
        }
        // Create the annual report with all the other sections
        {
          auto annual_report_file_path = annual_report_file_folder / "annual_report.rtf";
          std::filesystem::create_directories(annual_report_file_path.parent_path());
          std::ofstream raw_annual_report_os{annual_report_file_path};

          RTF::OStream annual_report_os{raw_annual_report_os};

          annual_report_os << annual_report_front_page;
          annual_report_os << annual_report_directors_report;
          annual_report_os << annual_report_profit_and_loss_statement;
          annual_report_os << annual_report_balance_sheet;
          annual_report_os << annual_report_annual_report_notes;
        }
      }

      {
        // Generate documents in HTML format
        auto annual_report_file_folder = to_bolagsverket_file_folder / "html";

        // Create one document for the financial statemnts approval (to accomodate the annual report copy sent to Swedish Bolagsverket)
        {
          auto annual_report_file_path = annual_report_file_folder / "financial_statement_approval.html";
          std::filesystem::create_directories(annual_report_file_path.parent_path());
          std::ofstream raw_annual_report_os{annual_report_file_path};

          HTML::OStream annual_report_os{raw_annual_report_os};

          annual_report_os << annual_report_financial_statements_approval;
        }
        // Create the annual report with all the other sections
        {
          auto annual_report_file_path = annual_report_file_folder / "annual_report.html";
          std::filesystem::create_directories(annual_report_file_path.parent_path());
          std::ofstream raw_annual_report_os{annual_report_file_path};

          HTML::OStream annual_report_os{raw_annual_report_os};

          annual_report_os << annual_report_front_page;
          annual_report_os << annual_report_directors_report;
          annual_report_os << annual_report_profit_and_loss_statement;
          annual_report_os << annual_report_balance_sheet;
          annual_report_os << annual_report_annual_report_notes;
        }
      }

      {
        // Generate documents in LaTeX format and then transform them into pdf using OS installed pdflatex command
        auto annual_report_file_folder = to_bolagsverket_file_folder / "tex";
        std::filesystem::create_directories(annual_report_file_folder);

        {
          // Test code
          char const* test_tex{R"(\documentclass{article}

\begin{document}

\begin{center}
The ITfied AB
\par \vspace*{12pt} 556782-8172
\par \vspace{12pt} Årsredovisning för räkenskapsåret
\par \vspace{12pt} 2021-05-01 - 2022-04-40
\par \vspace{12pt} Styrelsen avger följande årsredovisning
\par \vspace{12pt} Samtliga belopp är angivna i hela kronor
\end{center}

\pagebreak
\section*{Förvaltningsberättelse}

\subsection*{Verksamheten}
\subsubsection*{Allmänt om verksamheten}
\subsection*{Flerårsöversikt}
\subsection*{Förändring i eget kapital}
\subsection*{Resultatdisposition}

\pagebreak
\section*{Resultaträkning}

\pagebreak
\section*{Balansräkning}

\pagebreak
\section*{Noter}

\pagebreak
\section*{Underskrifter}

\end{document})"};
          std::ofstream os{annual_report_file_folder / "test.tex"};
          os << test_tex;
          // std::filesystem::copy_file("test/test.tex",annual_report_file_folder / "test.tex",std::filesystem::copy_options::overwrite_existing);
        }
      }

      {
        // Transform LaTeX documents to pdf:s
        auto annual_report_file_folder = to_bolagsverket_file_folder / "pdf";
        std::filesystem::create_directories(annual_report_file_folder);

        // pdflatex --output-directory to_bolagsverket/pdf test/test.tex

        for (auto const& dir_entry : std::filesystem::directory_iterator{to_bolagsverket_file_folder / "tex"}) {
          // Test for installed pdflatex
          std::ostringstream command{};
          command << "pdflatex";
          command << " --output-directory " << annual_report_file_folder;
          command << " " << dir_entry.path();
          if (auto result = std::system(command.str().c_str());result == 0) {
            prompt << "\nCreated pdf-document " << (annual_report_file_folder / dir_entry.path().stem()) << ".pdf" << " OK";
          }
          else {
            prompt << "\nSorry, failed to create pdf-documents.";
            prompt << "\n==> Please ensure you have installed command line tool 'pdflatex' on your system?";
            prompt << "\nmacOS: brew install --cask mactex";
            prompt << "\nLinux: 'sudo apt-get install texlive-latex-base texlive-fonts-recommended texlive-fonts-extra texlive-latex-extra'";
          }
        }


      }

    }
    /* ======================================================



              Act on on Words?


      ====================================================== */
    else {
      // std::cout << "\nAct on words";
      // Assume word based input
      if (model->prompt_state == PromptState::HADIndex) {
        if (do_assign) {
          prompt << "\nSorry, ASSIGN not yet implemented for your input " << std::quoted(command);
        }
      }
      else if ((model->prompt_state == PromptState::NetVATAccountInput) or (model->prompt_state == PromptState::GrossAccountInput))  {
        // Assume the user has enterd text to search for suitable accounts
        // Assume match to account name
        for (auto const& [account_no,am] : model->sie_env_map["current"].account_metas()) {
          if (first_in_second_case_insensitive(ast[1],am.name)) {
            prompt << "\n  " << account_no << " " << std::quoted(am.name);
            if (am.sru_code) prompt << " SRU:" << *am.sru_code;
          }
        }
      }
      else if (model->prompt_state == PromptState::EnterHA) {
        if (auto had_iter = model->selected_had()) {
          auto& had = *(*had_iter);
          if (!had.optional.current_candidate) std::cout << "\nNo had.optional.current_candidate";
          if (!had.optional.counter_ats_producer) std::cout << "\nNo had.optional.counter_ats_producer";
          if (had.optional.current_candidate and had.optional.counter_ats_producer) {
            auto gross_positive_amount = to_positive_gross_transaction_amount(had.optional.current_candidate->defacto);
            auto gross_negative_amount = to_negative_gross_transaction_amount(had.optional.current_candidate->defacto);
            auto gross_amounts_diff = gross_positive_amount + gross_negative_amount;
// std::cout << "\ngross_positive_amount:" << gross_positive_amount << " gross_negative_amount:" << gross_negative_amount << " gross_amounts_diff:" << gross_amounts_diff;

            switch (ast.size()) {
              case 0: {
                prompt << "\nPlease enter:";
                prompt << "\n\t Heading + Amount (to add a transaction aggregate with a caption)";
                prompt << "\n\t Heading          (to add a transaction aggregate with a caption and full remaining amount)";
              } break;
              case 1: {
                if (auto amount = to_amount(ast[0])) {
                  prompt << "\nAMOUNT " << *amount;
                  prompt << "\nPlease enter Heading only (full remaining amount implied) or Heading + Amount";
                }
                else {
                  prompt << "\nHEADER " << ast[0];
                  auto ats = (*had.optional.counter_ats_producer)(abs(gross_amounts_diff),ast[0]);
                  std::copy(ats.begin(),ats.end(),std::back_inserter(had.optional.current_candidate->defacto.account_transactions));
                  prompt << "\nAdded transaction aggregate for REMAINING NET AMOUNT" << ats;;
                }
              } break;
              case 2: {
                if (auto amount = to_amount(ast[1])) {
                  prompt << "\nHEADER " << ast[0];
                  prompt << "\nAMOUNT " << *amount;
                  prompt << "\nWe will create a {net,vat} using this this header and amount";
                  if (gross_amounts_diff > 0) {
                    // We need to balance up with negative account transaction aggregates
                    auto ats = (*had.optional.counter_ats_producer)(abs(gross_amounts_diff),ast[0],amount);
                    std::copy(ats.begin(),ats.end(),std::back_inserter(had.optional.current_candidate->defacto.account_transactions));
                    prompt << "\nAdded negative transactions aggregate" << ats;
                  }
                  else if (gross_amounts_diff < 0) {
                    // We need to balance up with positive account transaction aggregates
                    auto ats = (*had.optional.counter_ats_producer)(abs(gross_amounts_diff),ast[0],amount);
                    std::copy(ats.begin(),ats.end(),std::back_inserter(had.optional.current_candidate->defacto.account_transactions));
                    prompt << "\nAdded positive transaction aggregate";
                  }
                  else if (abs(gross_amounts_diff) < 1.0) {
                    // Consider a cents rounding account transaction
                    prompt << "\nAdded cents rounding to account 3740";
                    auto cents_rounding_at = BAS::anonymous::AccountTransaction{
                      .account_no = 3740
                      ,.amount = -gross_amounts_diff
                    };
                    had.optional.current_candidate->defacto.account_transactions.push_back(cents_rounding_at);
                  }
                  else {
                    // The journal entry candidate balances. Consider to stage it
                    prompt << "\nTODO: Stage balancing journal entry";
                  }
                }
              } break;
            }
            prompt << "\ncandidate:" << *had.optional.current_candidate;
            gross_positive_amount = to_positive_gross_transaction_amount(had.optional.current_candidate->defacto);
            gross_negative_amount = to_negative_gross_transaction_amount(had.optional.current_candidate->defacto);
            gross_amounts_diff = gross_positive_amount + gross_negative_amount;
            prompt << "\n-------------------------------";
            prompt << "\ndiff:" << gross_amounts_diff;
            if (gross_amounts_diff == 0) {
              // Stage the journal entry
              auto stage_result = model->sie_env_map.stage(*had.optional.current_candidate);
              if (stage_result) {
                prompt << "\n" << stage_result.md_entry() << " STAGED";
                model->heading_amount_date_entries.erase(*had_iter);
                model->prompt_state = PromptState::HADIndex;
              }
              else {
                prompt << "\nSORRY - Failed to stage entry";
                model->prompt_state = PromptState::Root;
              }
            }

          }
          else {
            prompt << "\nPlease re-select a valid HAD and template (Seems to have failed to identify a valid template for current situation)";
          }
        }
        else {
          prompt << "\nPlease re-select a valid had (seems to have lost previously selected had)";
        }

      }
      else if (    (model->prompt_state == PromptState::JEIndex)
                or (model->prompt_state == PromptState::JEAggregateOptionIndex)) {
        if (do_assign) {
          // Assume <text> = <text>
          if (auto had_iter = model->selected_had()) {
            auto& had = *(*had_iter);
            if (ast.size() >= 3 and ast[1] == "=") {
              // Assume at least three tokens 0:<token> 1:'=' 2:<Token>
              if (auto amount = to_amount(ast[2])) {
                BAS::AccountMetas ams{};
                auto transaction_amount = (abs(*amount) <= 1)?(to_double(*amount) * had.amount):(*amount); // quote of had amount or actual amount

                if (auto to_match_account_no = BAS::to_account_no(ast[0])) {
                  // The user entered <target> = a BAS Account or SRU account
                  ams = matches_bas_or_sru_account_no(*to_match_account_no,model->sie_env_map["current"]);
                }
                else {
                  // The user entered a search criteria for a BAS account name
                  ams = matches_bas_account_name(ast[0],model->sie_env_map["current"]);
                }
                if (ams.size() == 0) {
                  prompt << "\nSorry, failed to match your input to any BAS or SRU account";
                }
                else if (ams.size() == 1) {
                  // Go ahead and use this account for an account transaction
                  if (had.optional.current_candidate) {
                    // extend current candidate
                    BAS::anonymous::AccountTransaction new_at{
                      .account_no = ams.begin()->first
                      ,.amount = transaction_amount
                    };
                    had.optional.current_candidate->defacto.account_transactions.push_back(new_at);
                    prompt << "\n" << *had.optional.current_candidate;
                  }
                  else {
                    // Create options from scratch
                    had.optional.gross_account_no = ams.begin()->first;
                    BAS::TypedMetaEntries template_candidates{};
                    std::string transtext = (ast.size() == 4)?ast[3]:"";
                    for (auto series : std::string{"ABCDEIM"}) {
                      BAS::MDJournalEntry mdje{
                        .meta = {
                          .series = series
                        }
                        ,.defacto = {
                          .caption = had.heading
                          ,.date = had.date
                        }
                      };
                      BAS::anonymous::AccountTransaction new_at{
                        .account_no = ams.begin()->first
                        ,.transtext = transtext
                        ,.amount = transaction_amount
                      };
                      mdje.defacto.account_transactions.push_back(new_at);
                      template_candidates.push_back(to_typed_md_entry(mdje));
                    }
                    model->template_candidates.clear();
                    std::copy(template_candidates.begin(),template_candidates.end(),std::back_inserter(model->template_candidates));
                    unsigned ix = 0;
                    for (int i=0; i < model->template_candidates.size(); ++i) {
                      prompt << "\n    " << ix++ << " " << model->template_candidates[i];
                    }
                  }
                }
                else {
                  // List the options to inform the user there are more than one BAS account that matches the input
                  prompt << "\nDid you mean...";
                  for (auto const& [account_no,am] : ams) {
                    prompt << "\n\t" << std::quoted(am.name) << " " << account_no << " = " << transaction_amount;
                  }
                  prompt << "\n==> Please try again <target> = <Quote or amount>";
                }
              } // to_amount
              else {
                  prompt << "\nPlease provide a valid amount after '='. I failed to recognise your input " << std::quoted(ast[2]);
              }
            } // ast[1] == '='
            else {
              prompt << "Please provide a space on both sides of the '=' in your input " << std::quoted(command);
            }
          }
          else {
            prompt << "\nSorry, I seem to have lost track of what had you selected.";
            prompt << "\nPlease try again with a new had.";
          }
        }
        else {
          // Assume the user has entered a new search criteria for template candidates
          model->template_candidates = this->all_years_template_candidates([&command](BAS::anonymous::JournalEntry const& aje){
            return strings_share_tokens(command,aje.caption);
          });
          int ix{0};
          for (int i = 0; i < model->template_candidates.size(); ++i) {
            prompt << "\n    " << ix++ << " " << model->template_candidates[i];
          }
          // Consider the user may have entered the name of a gross account to journal the transaction amount
          auto gats = to_gross_account_transactions(model->sie_env_map);
          model->at_candidates.clear();
          std::copy_if(gats.begin(),gats.end(),std::back_inserter(model->at_candidates),[&command,this](BAS::anonymous::AccountTransaction const& at){
            bool result{false};
            if (at.transtext) result |= strings_share_tokens(command,*at.transtext);
            if (model->sie_env_map["current"].account_metas().contains(at.account_no)) {
              auto const& meta = model->sie_env_map["current"].account_metas().at(at.account_no);
              result |= strings_share_tokens(command,meta.name);
            }
            return result;
          });
          for (int i=0;i < model->at_candidates.size();++i) {
            prompt << "\n    " << ix++<< " " << model->at_candidates[i];
          }
        }
      }
      else if (model->prompt_state == PromptState::CounterAccountsEntry) {
        if (auto nha = to_name_heading_amount(ast)) {
          // List account candidates for the assumed "Name, Heading + Amount" entry by the user
          prompt << "\nAccount:" << std::quoted(nha->account_name);
          if (nha->trans_text) prompt << " text:" << std::quoted(*nha->trans_text);
          prompt << " amount:" << nha->amount;
        }
        else {
          prompt << "\nPlease enter an account, and optional transaction text and an amount";
        }
        // List the new current options
        if (auto had_iter = model->selected_had()) {
          if ((*had_iter)->optional.current_candidate) {
            unsigned int i{};
            prompt << "\n" << (*had_iter)->optional.current_candidate->defacto.caption << " " << (*had_iter)->optional.current_candidate->defacto.date;
            for (auto const& at : (*had_iter)->optional.current_candidate->defacto.account_transactions) {
              prompt << "\n  " << i++ << " " << at;
            }
          }
          else {
            prompt << "\nPlease enter a valid Account Transaction Index";
          }
        }
        else {
          prompt << "\nPlease select a Heading Amount Date entry";
        }
      }
      else if (model->prompt_state == PromptState::EditAT) {
        // Handle user Edit of currently selected account transaction (at)
// std::cout << "\nPromptState::EditAT " << std::quoted(command);
        if (auto had_iter = model->selected_had()) {
          auto const& had = **had_iter;
      		if (auto at_iter = model->to_at_iter(had_iter,model->at_index)) {
            auto& at = **at_iter;
            prompt << "\n\tbefore:" << at;
            if (auto account_no = BAS::to_account_no(command)) {
              at.account_no = *account_no;
              prompt << "\nBAS Account: " << *account_no;
            }
            else if (auto amount = to_amount(command)) {
              prompt << "\nAmount " << *amount;
              at.amount = *amount;
            }
            else if (command[0] == 'x') {
              // ignore input
            }
            else {
              // Assume the user entered a new transtext
              prompt << "\nTranstext " << std::quoted(command);
              at.transtext = command;
            }
            unsigned int i{};
            std::for_each(had.optional.current_candidate->defacto.account_transactions.begin(),had.optional.current_candidate->defacto.account_transactions.end(),[&i,&prompt](auto const& at){
              prompt << "\n  " << i++ << " " << at;
            });
            model->prompt_state = PromptState::ATIndex;
          }
          else {
            prompt << "\nSORRY, I seems to have forgotten what account transaction you selected. Please try over again";
            model->prompt_state = PromptState::HADIndex;
          }
        }
        else {
          prompt << "\nSORRY, I seem to have forgotten what HAD you selected. Please select a Heading Amount Date entry";
          model->prompt_state = PromptState::HADIndex;
        }
      }
      else if (model->prompt_state == PromptState::EnterContact) {
        if (ast.size() == 3) {
          SKV::ContactPersonMeta cpm {
            .name = ast[0]
            ,.phone = ast[1]
            ,.e_mail = ast[2]
          };
          if (model->organisation_contacts.size() == 0) {
            model->organisation_contacts.push_back(cpm);
          }
          else {
            model->organisation_contacts[0] = cpm;
          }
          auto const& [delta_prompt,prompt_state] = this->transition_prompt_state(model->prompt_state,PromptState::SKVTaxReturnEntryIndex);
          prompt << delta_prompt;
          model->prompt_state = prompt_state;
        }
      }
      else if (model->prompt_state == PromptState::EnterEmployeeID) {
        if (ast.size() > 0) {
          if (model->employee_birth_ids.size()==0) {
            model->employee_birth_ids.push_back(ast[0]);
          }
          else {
            model->employee_birth_ids[0] = ast[0];
          }
          auto const& [delta_prompt,prompt_state] = this->transition_prompt_state(model->prompt_state,PromptState::SKVTaxReturnEntryIndex);
          prompt << delta_prompt;
          model->prompt_state = prompt_state;
        }
      }
      else if (model->prompt_state == PromptState::EnterIncome) {
        if (auto amount = to_amount(command)) {
          model->sru["0"].set(1000,std::to_string(SKV::to_tax(*amount)));

          Amount income = get_INK1_Income(model);
          prompt << "\n1) INK1 1.1 Lön, förmåner, sjukpenning m.m. = " << income;
          Amount dividend = get_K10_Dividend(model);
          prompt << "\n2) K10 1.6 Utdelning = " << dividend;
          prompt << "\n3) Continue (Create K10 and INK1)";
          model->prompt_state = PromptState::K10INK1EditOptions;
        }
        else {
          prompt << "\nPlease enter a valid amount";
        }
      }
      else if (model->prompt_state == PromptState::EnterDividend) {
        if (auto amount = to_amount(command)) {
          model->sru["0"].set(4504,std::to_string(SKV::to_tax(*amount)));
          Amount income = get_INK1_Income(model);
          prompt << "\n1) INK1 1.1 Lön, förmåner, sjukpenning m.m. = " << income;
          Amount dividend = get_K10_Dividend(model);
          prompt << "\n2) K10 1.6 Utdelning = " << dividend;
          prompt << "\n3) Continue (Create K10 and INK1)";
          model->prompt_state = PromptState::K10INK1EditOptions;
        }
        else {
          prompt << "\nPlease enter a valid amount";
        }
      }
      else if (auto mdje = find_meta_entry(model->sie_env_map["current"],ast)) {
        // The user has entered a search term for a specific journal entry (to edit)
        // Allow the user to edit individual account transactions
        if (auto had = to_had(*mdje)) {
          model->heading_amount_date_entries.push_back(*had);
          model->had_index = 0; // index zero is the "last" (newest) one
          unsigned int i{};
          std::for_each(
             had->optional.current_candidate->defacto.account_transactions.begin()
            ,had->optional.current_candidate->defacto.account_transactions.end()
            ,[&i,&prompt](auto const& at) {
              prompt << "\n  " << i++ << " " << at;
          });
          model->prompt_state = PromptState::ATIndex;
        }
        else {
          prompt << "\nSorry, I failed to turn selected journal entry into a valid had (it seems I am not sure exactly why...)";
        }
      }
      else {
        // Assume Heading + Amount + Date (had) input
        auto tokens = tokenize::splits(command,tokenize::SplitOn::TextAmountAndDate);
        if (auto had = to_had(tokens)) {
          prompt << "\n" << *had;
          model->heading_amount_date_entries.push_back(*had);
          // Decided NOT to sort hads here to keep last listed had indexes intact (new -had command = sort and new indexes)
        }
        else {
          prompt << "\nERROR - Expected Heading + Amount + Date";
          prompt << "\nI Interpreted your input as,";
          for (auto const& token : tokens) prompt << "\n\ttoken: \"" << token << "\"";
          prompt << "\n\nPlease check that your input matches my expectations?";
          prompt << "\n\tHeading = any text (\"...\" enclosure allowed)";
          prompt << "\n\tAmount = any positive or negative amount with optional ',' or '.' decimal point with one or two decimal digits";
          prompt << "\n\tDate = YYYYMMDD or YYYY-MM-DD";
        }
      }
    }
  }
  if (prompt.str().size()>0) prompt << "\n";
  prompt << prompt_line(model->prompt_state);
  model->prompt = prompt.str();
  return {};
}
Cmd Updater::operator()(Quit const& quit) {
  // std::cout << "\noperator(Quit)";
  std::ostringstream os{};
  os << "\nBye for now :)";
  model->prompt = os.str();
  model->quit = true;
  return {};
}
Cmd Updater::operator()(Nop const& nop) {
  // std::cout << "\noperator(Nop)";
  return {};
}
BAS::TypedMetaEntries Updater::all_years_template_candidates(auto const& matches) {
  BAS::TypedMetaEntries result{};
  auto meta_entry_topology_map = to_meta_entry_topology_map(model->sie_env_map);
  for (auto const& [signature,tme_map] : meta_entry_topology_map) {
    for (auto const& [topology,tmes] : tme_map) {
      auto accounts_topology_map = to_accounts_topology_map(tmes);
      for (auto const& [signature,bat_map] : accounts_topology_map) {
        for (auto const& [topology,tmes] : bat_map) {
          for (auto const& tme : tmes) {
            auto mdje = to_md_entry(tme);
            if (matches(mdje.defacto)) result.push_back(tme);
          }
        }
      }
    }
  }
  return result;
}
std::pair<std::string,PromptState> Updater::transition_prompt_state(PromptState const& from_state,PromptState const& to_state) {
  std::ostringstream prompt{};
  switch (to_state) {
    case PromptState::SKVTaxReturnEntryIndex: {
      prompt << "\n1 Organisation Contact:" << std::quoted(model->organisation_contacts[0].name) << " " << std::quoted(model->organisation_contacts[0].phone) << " " << model->organisation_contacts[0].e_mail;
      prompt << "\n2 Employee birth no:" << model->employee_birth_ids[0];
      prompt << "\n3 <Period> Generate Tax Returns form (Period in form YYYYMM)";
    } break;
    default: {
      prompt << "\nPlease mail developer that transition_prompt_state detected a potential design insufficiency";
    } break;
  }
  return {prompt.str(),to_state};
}

// Now in BASFramework unit
// HeadingAmountDateTransEntries hads_from_environment(Environment const& environment);


class Cratchit {
public:
	Cratchit(std::filesystem::path const& p)
		: cratchit_file_path{p}
          ,m_persistent_environment_file{p,::environment_from_file,::environment_to_file} {}

        Model init(Command const& command) {
          std::ostringstream prompt{};
          prompt << "\nInit from ";
          prompt << cratchit_file_path;
          m_persistent_environment_file.init();
          if (auto const& cached_env = m_persistent_environment_file.cached()) {
            auto model = this->model_from_environment_and_files(*cached_env);
            model->prompt_state = PromptState::Root;
            prompt << "\n" << prompt_line(model->prompt_state);
            model->prompt += prompt.str();
            return model;
          } else {
            prompt << "\nSorry, Failed to load environment from " << cratchit_file_path;
            auto model = std::make_unique<ConcreteModel>();
            model->prompt = prompt.str();
            model->prompt_state = PromptState::Root;
            return model;
          }
        }
        std::pair<Model, Cmd> update(Msg const& msg, Model &&model) {
          // std::cout << "\nupdate" << std::flush;
          Cmd cmd{};
          {
            // Scope to ensure correct move of model in and out of updater
            model->prompt.clear();
            Updater updater{std::move(model)};
            cmd = std::visit(updater, msg);
            model = std::move(updater.model);
          }
          // std::cout << "\nCratchit::update updater visit ok" << std::flush;
          if (model->quit) {
            // std::cout << "\nCratchit::update if (updater.model->quit) {" << std::flush;
            logger::development_trace("zeroth::model->quit DETECTED");
            auto model_environment = environment_from_model(model);
            auto cratchit_environment = this->add_cratchit_environment(model_environment);
            this->m_persistent_environment_file.update(cratchit_environment);
            // #3 unposted_to_sie_files save unstaged entries per environment (unique file names)
            unposted_to_sie_files(model->sie_env_map);
            // Update/create the skv xml-file (employer monthly tax declaration)
            // std::cout << R"(\nmodel->sie_env_map["current"].organisation_no.CIN=)" << model->sie_env_map["current"].organisation_no.CIN;
          }
          // std::cout << "\nCratchit::update before prompt update" << std::flush;
          // std::cout << "\nCratchit::update before return" << std::flush;
          return {std::move(model), cmd};
        }
        Ux view(Model const& model) {
		// std::cout << "\nview" << std::flush;
		Ux ux{};
		ux.push_back(model->prompt);
		return ux;
	}
private:
    PersistentFile<Environment> m_persistent_environment_file;
	std::filesystem::path cratchit_file_path{};

  // Now free functions
	// std::vector<SKV::ContactPersonMeta> contacts_from_environment(Environment const& environment) {
	// DateOrderedTaggedAmountsContainer dotas_from_sie_environment(SIEEnvironment const& sie_env) {
  // SKV::SpecsDummy skv_specs_mapping_from_csv_files(std::filesystem::path cratchit_file_path,Environment const& environment) {
  // TaggedAmounts tas_sequence_from_consumed_account_statement_files(Environment const& environment) {
  // DateOrderedTaggedAmountsContainer dotas_from_environment_and_account_statement_files(Environment const& environment) {
  // void synchronize_tagged_amounts_with_sie(DateOrderedTaggedAmountsContainer& all_dotas,SIEEnvironment const& sie_environment) {
	// SRUEnvironments srus_from_environment(Environment const& environment) {
	// std::vector<std::string> employee_birth_ids_from_environment(Environment const& environment) {
  // HeadingAmountDateTransEntries hads_from_environment(Environment const& environment)

	bool is_value_line(std::string const& line) {
		// Now in environment unit
		return in::is_value_line(line);
	}

	Model model_from_environment_and_files(Environment const& environment) {
		return zeroth::model_from_environment_and_files(this->cratchit_file_path,environment);
	}

  // indexed_env_entries_from now in HADFramework
  // std_overload::generator<EnvironmentIdValuePair> indexed_env_entries_from(HeadingAmountDateTransEntries const& entries) {

	Environment environment_from_model(Model const& model) {
    return zeroth::environment_from_model(model);
	}
	Environment add_cratchit_environment(Environment const& environment) {
		Environment result{environment};
		// Add cratchit environment values if/when there are any
		return result;
	}

}; // class Cratchit

class REPL {
public:
    REPL(std::filesystem::path const& environment_file_path) : cratchit{environment_file_path} {}

	void run(Command const& command) {
    auto model = cratchit.init(command);
		auto ux = cratchit.view(model);
		render_ux(ux);
		while (true) {
			try {
				// process events (user input)
				if (in.size()>0) {
					auto msg = in.front();
					in.pop();
					// std::cout << "\nmsg[" << msg.index() << "]";
					// Turn Event (Msg) into updated model
					// std::cout << "\nREPL::run before cratchit.update " << std::flush;
					auto [updated_model,cmd] = cratchit.update(msg,std::move(model));
					// std::cout << "\nREPL::run cratchit.update ok" << std::flush;
					model = std::move(updated_model);
					// std::cout << "\nREPL::run model moved ok" << std::flush;
					if (cmd.msg) in.push(*cmd.msg);
					// std::cout << "\nREPL::run before view" << std::flush;
					auto ux = cratchit.view(model);
					// std::cout << "\nREPL::run before render" << std::flush;
					render_ux(ux);
					if (model->quit) break; // Done
				}
				else {
					// Get more buffered user input
					std::string user_input{};
					std::getline(std::cin,user_input);
					auto cmd = to_cmd(user_input);
					if (cmd.msg) this->in.push(*cmd.msg);
				}
			}
			catch (std::exception& e) {
				std::cout << "\nERROR: run() loop failed, exception=" << e.what() << std::flush;
			}
		}
		// std::cout << "\nREPL::run exit" << std::flush;
	}
private:
    Cratchit cratchit;
		void render_ux(Ux const& ux) {
			for (auto const&  row : ux) {
				if (row.size()>0) std::cout << row;
			}
		}
    std::queue<Msg> in{};
}; // Cratchit


// Environment -> Model

std::vector<SKV::ContactPersonMeta> contacts_from_environment(Environment const& environment) {
  std::vector<SKV::ContactPersonMeta> result{};
  // auto [begin,end] = environment.equal_range("contact");
  // if (begin == end) {
  // 	// Instantiate default values if required
  // 	result.push_back({
  // 			.name = "Ville Vessla"
  // 			,.phone = "555-244454"
  // 			,.e_mail = "ville.vessla@foretaget.se"
  // 		});
  // }
  // else {
  // 	std::transform(begin,end,std::back_inserter(result),[](auto const& entry){
  // 		return *to_contact(entry.second); // Assume success
  // 	});
  // }
  if (environment.contains("contact")) {
    auto const& id_ev_pairs = environment.at("contact");
    std::transform(id_ev_pairs.begin(),id_ev_pairs.end(),std::back_inserter(result),[](auto const& id_ev_pair){
      auto const& [id,ev] = id_ev_pair;
      return *to_contact(ev); // Assume success
    });
  }
  else {
    // Instantiate default values if required
    result.push_back({
        .name = "Ville Vessla"
        ,.phone = "555-244454"
        ,.e_mail = "ville.vessla@foretaget.se"
      });
  }

  return result;
} // contacts_from_environment

std::vector<std::string> employee_birth_ids_from_environment(Environment const& environment) {
  std::vector<std::string> result{};
  // auto [begin,end] = environment.equal_range("employee");
  // if (begin == end) {
  // 	// Instantiate default values if required
  // 	result.push_back({"198202252386"});
  // }
  // else {
  // 	std::transform(begin,end,std::back_inserter(result),[](auto const& entry){
  // 		return *to_employee(entry.second); // dereference optional = Assume success
  // 	});
  // }
  if (environment.contains("employee")) {
    auto const id_ev_pairs = environment.at("employee");
    std::transform(id_ev_pairs.begin(),id_ev_pairs.end(),std::back_inserter(result),[](auto const& id_ev_pair){
      auto const [id,ev] = id_ev_pair;
      return *to_employee(ev); // dereference optional = Assume success
    });
  }
  else {
    // Instantiate default values if required
    result.push_back({"198202252386"});
  }

  return result;
} // employee_birth_ids_from_environment

SRUEnvironments srus_from_environment(Environment const& environment) {
  SRUEnvironments result{};
  // auto [begin,end] = environment.equal_range("SRU:S");
  // std::transform(begin,end,std::inserter(result,result.end()),[](auto const& entry){
  // 	if (auto result_entry = to_sru_environments_entry(entry.second)) return *result_entry;
  // 	return SRUEnvironments::value_type{"null",{}};
  // });
  if (environment.contains("SRU:S")) {
    auto const id_ev_pairs = environment.at("SRU:S");
    std::transform(id_ev_pairs.begin(),id_ev_pairs.end(),std::inserter(result,result.end()),[](auto const& id_ev_pair){
      auto const& [id,ev] = id_ev_pair;
      if (auto result_entry = to_sru_environments_entry(ev)) return *result_entry;
      return SRUEnvironments::value_type{"null",{}};
    });
  }
  else {
    // Nop
  }
  return result;
} // srus_from_environment

void synchronize_tagged_amounts_with_sie(DateOrderedTaggedAmountsContainer& all_dotas,SIEEnvironment const& sie_environment) {
  // TODO: Base the implementation on DateOrderedTaggedAmountsContainer handling 'branching' on CAS based value-id ordering?

  logger::scope_logger scope_raii{logger::development_trace,"SYNHRONIZE TAGGED AMOUNTS WITH SIE"};
  if (auto financial_year_date_range = sie_environment.financial_year_date_range()) {
    std::cout << "\n\tSIE fiscal year:" << *financial_year_date_range;
    auto environment_dotas = dotas_from_sie_environment(sie_environment);      

    // TODO: 'put' all tagged amounts in environment_dotas
    //       into 'all_dotas' and trust it to detect branching
    //       for conflicting ordering (when implemented)
    logger::design_insufficiency("synchronize_tagged_amounts_with_sie NOT YET IMPLEMENTED");

  }
  else {
    std::cout << "\n\tERROR, synchronize_tagged_amounts_with_sie failed -  Provided SIE Environment has no fiscal year set";
  }
} // synchronize_tagged_amounts_with_sie

DateOrderedTaggedAmountsContainer dotas_from_sie_environment(SIEEnvironment const& sie_env) {
  if (true) {
    std::cout << "\ndotas_from_sie_environment" << std::flush;
  }
  DateOrderedTaggedAmountsContainer result{};
  // Create / add opening balances for BAS accounts as tagged amounts
  auto financial_year_date_range = sie_env.financial_year_date_range();
  auto opening_saldo_date = financial_year_date_range->begin();
  std::cout << "\nOpening Saldo Date:" << opening_saldo_date;
  for (auto const& [bas_account_no,saldo] : sie_env.opening_balances()) {
    auto saldo_cents_amount = to_cents_amount(saldo);
    TaggedAmount saldo_ta{opening_saldo_date,saldo_cents_amount};
    saldo_ta.tags()["BAS"] = std::to_string(bas_account_no);
    saldo_ta.tags()["IB"] = "True";
    result.dotas_insert_auto_ordered_value(saldo_ta);
    if (true) {
      std::cout << "\n\tsaldo_ta : " << saldo_ta;
    }
  }
  auto create_and_merge_to_result = [&result](BAS::MDJournalEntry const& mdje) {
    auto tagged_amounts = to_tagged_amounts(mdje);
    // TODO #SIE: Consider to check here is we already have tagged amounts reflecting the same SIE transaction (This one in the SIE file is the one to use)
    //       Can we first delete any existing tagged amounts for the same SIE transaction (to ensure we do not get dublikates for SIE transactions edited externally?)
    // Hm...problem is that here we do not have access to the other tagged amounts already in the environment...
    // result += tagged_amounts;
    result.dotas_insert_auto_ordered_sequence(tagged_amounts);
  };
  for_each_md_journal_entry(sie_env,create_and_merge_to_result);
  return result;
} // dotas_from_sie_environment

// Environment + cratchit_file_path -> Model

DateOrderedTaggedAmountsContainer dotas_from_account_statement_files(std::filesystem::path cratchit_file_path) {
  DateOrderedTaggedAmountsContainer result{};

  // Import any new account statements in dedicated "files from bank or skv" folder
  result.dotas_insert_auto_ordered_sequence(
    tas_sequence_from_consumed_account_statement_files(
       cratchit_file_path));

  return result;
}

DateOrderedTaggedAmountsContainer dotas_from_environment_and_account_statement_files(std::filesystem::path cratchit_file_path,Environment const& environment) {
  if (false) {
    std::cout << "\ndotas_from_environment_and_account_statement_files" << std::flush;
  }
  DateOrderedTaggedAmountsContainer result{};

  result.dotas_insert_auto_ordered_container(
    dotas_from_environment(environment));

  result.dotas_insert_auto_ordered_container(
    dotas_from_account_statement_files(cratchit_file_path));
  
  return result;
} // dotas_from_environment_and_account_statement_files

std::pair<std::filesystem::path,bool> make_consumed(std::filesystem::path statement_file_path) {
  auto consumed_files_path = statement_file_path.parent_path() / "consumed";
  auto consumed_file_path = consumed_files_path / statement_file_path.filename();
  bool was_consumed{true};
  try {
    std::filesystem::create_directories(consumed_files_path); // Returns false both if already exists and if it fails (so useless to check...I think?)
    std::filesystem::rename(statement_file_path,consumed_file_path);
  }
  catch ( std::filesystem::filesystem_error const& e) {
    // 
    logger::design_insufficiency("make_consumed: Exception: {}",e.what());
    was_consumed = false;
  }
  return {consumed_file_path,was_consumed};
}

TaggedAmounts tas_sequence_from_consumed_account_statement_file(std::filesystem::path statement_file_path) {
  TaggedAmounts result{};
  if (auto maybe_tas = tas_from_statment_file(statement_file_path)) {
    result = maybe_tas.value();
    std::cout << "\n\tValid entries count:" << maybe_tas->size();
    if (false) {
      // std::filesystem::create_directories(consumed_files_path); // Returns false both if already exists and if it fails (so useless to check...I think?)
      // std::filesystem::rename(statement_file_path,consumed_files_path / statement_file_path.filename());
      auto make_consumed_result = make_consumed(statement_file_path);
      if (make_consumed_result.second == true) {
        std::cout << "\n\tConsumed account statement file moved to " << make_consumed_result.first;
      }
      else {
        std::cout << "\nSorry, It seems I failed to move statement file " << statement_file_path << " to " << make_consumed_result.first;
      }
    }
    else {
      std::cout << "\n\tConsumed account statement file move DISABLED = Remains at " << statement_file_path;
    }
  }
  else {
    std::cout << "\n*Note* " << statement_file_path << " (produced zero entries)";
  }  
  return result;
}

TaggedAmounts tas_sequence_from_consumed_account_statement_files(std::filesystem::path cratchit_file_path) {
  if (false) {
    std::cout << "\n" << "tas_sequence_from_consumed_account_statement_files" << std::flush;
  }
  TaggedAmounts result{};
  // Ensure folder "from_bank_or_skv folder" exists
  auto from_bank_or_skv_path = cratchit_file_path.parent_path() /  "from_bank_or_skv";
  std::filesystem::create_directories(from_bank_or_skv_path); // Returns false both if already exists and if it fails (so useless to check...I think?)
  if (std::filesystem::exists(from_bank_or_skv_path) == true) {
    // Process all files in from_bank_or_skv_path
    std::cout << "\nBEGIN: Processing files in " << from_bank_or_skv_path << std::flush;
    // auto dir_entries = std::filesystem::directory_iterator{}; // Test empty iterator (does heap error disappear?) - YES!
    // TODO 240611 - Locate heap error detected here for g++ built with address sanitizer
    // NOTE: Seems to be all std::filesystem::directory_iterator usage in this code (Empty iterator here makes another loop cause heap error)
    for (auto const& dir_entry : std::filesystem::directory_iterator{from_bank_or_skv_path}) { 
      // 240612 Test if g++14 with sanitizer detects heap violation also for an empty std::filesystem::directory_iterator loop?
      //        YES - c++14 with "-fsanitize=address,undefined" reports runtime heap violation error on the for statement above!
      if (true) {
        auto statement_file_path = dir_entry.path();
        std::cout << "\n\nBEGIN File: " << statement_file_path << std::flush;
        if (dir_entry.is_directory()) {
          // skip directories (will process regular files and symlinks etc...)
        }
        // Process file
        else if (
           auto tas = tas_sequence_from_consumed_account_statement_file(statement_file_path)
          ;tas.size() > 0) {
          result.insert(result.end(),tas.begin(),tas.end());
        }
        else {
          std::cout << "\n*Ignored* " << statement_file_path << " (File empty or failed to understand file content)";
        }
        std::cout << "\nEND File: " << statement_file_path;
      }
    }
    std::cout << "\nEND: Processed Files in " << from_bank_or_skv_path;
  }
  if (true) {
    std::cout 
      << "\n" << "tas_sequence_from_consumed_account_statement_files RETURNS " 
      << result.size() 
      << " entries";
  }
  return result;
} // tas_sequence_from_consumed_account_statement_files

SKV::SpecsDummy skv_specs_mapping_from_csv_files(std::filesystem::path cratchit_file_path,Environment const& environment) {
  // TODO 230420: Implement actual storage in model for these mappings (when usage is implemented)
  SKV::SpecsDummy result{};
  auto skv_specs_path = cratchit_file_path.parent_path() /  "skv_specs";
  std::filesystem::create_directories(skv_specs_path); // Returns false both if already exists and if it fails (so useless to check...I think?)
  if (std::filesystem::exists(skv_specs_path) == true) {
    // Process all files in skv_specs_path
    std::cout << "\nBEGIN: Processing files in " << skv_specs_path;
    for (auto const& dir_entry : std::filesystem::directory_iterator{skv_specs_path}) {
      auto skv_specs_member_path = dir_entry.path();
      if (std::filesystem::is_directory(skv_specs_member_path)) {
        std::cout << "\n\nBEGIN Folder: " << skv_specs_member_path;
        for (auto const& dir_entry : std::filesystem::directory_iterator{skv_specs_member_path}) {
          auto financial_year_member_path = dir_entry.path();
          std::cout << "\n\tBEGIN " <<  financial_year_member_path;
          if (std::filesystem::is_regular_file(financial_year_member_path) and (financial_year_member_path.extension() == ".csv")) {
            auto ifs = std::ifstream{financial_year_member_path};
            encoding::UTF8::istream utf8_in{ifs}; // Assume the file is created in UTF8 character set encoding
            if (auto field_rows = CSV::to_field_rows(utf8_in,';')) {
              std::cout << "\n\tNo Operation implemented";
              // std::cout << "\n\t<Entries>";
              for (int i=0;i<field_rows->size();++i) {
                auto field_row = field_rows->at(i);
                // std::cout << "\n\t\t[" << i << "] : ";
                for (int j=0;j<field_row.size();++j) {
                  // [14] :  [0]1.1 Årets gränsbelopp enligt förenklingsregeln [1]4511 [2]Numeriskt_B [3]N [4]+ [5]Regel_E
                  // index 1 = SRU Code
                  // Index 0 = Readable field name on actual human readable form
                  // The other fields defines representation and "rules"
                  // std::cout << " [" << j << "]" << field_row[j];
                }
              }
            }
            else {
              std::cout << "\n\tFAILED PARSING FILE " <<  financial_year_member_path;
            }
          }
          else {
            std::cout << "\n\tSKIPPED UNUSED FILE" <<  financial_year_member_path;
          }
          std::cout << "\n\tEND " <<  financial_year_member_path;
        }
        std::cout << "\nEND Folder: " << skv_specs_member_path;
      }
      else {
        std::cout << "\n\tSKIPPED NON FOLDER PATH " << skv_specs_member_path;
      }
    }
    std::cout << "\nEND: Processing files in " << skv_specs_path;
  }
  return result;
} // skv_specs_mapping_from_csv_files

namespace zeroth {

  ConfiguredSIEFilePaths to_configured_posted_sie_file_paths(
    Environment const& environment) {
    ConfiguredSIEFilePaths result{};

    if (environment.contains("sie_file")) {
      auto const id_ev_pairs = environment.at("sie_file");
      for (auto const& id_ev_pair : id_ev_pairs) {
        auto const& [id,ev] = id_ev_pair;
        for (auto const& [year_key,sie_file_name] : ev) {
          std::filesystem::path sie_file_path{sie_file_name};
          result.push_back({year_key,sie_file_path});
        }
      }
    }
    else {
      logger::development_trace("to_configured_posted_sie_file_paths:No sie_file entries found in environment");
    }

    // Return year id order old to new ...,"-3","-2","-1","current" / "0"
    // This will make cratchit consume posted sie-files in 'date order' old to new
    std::ranges::sort(result, [](auto const& lhs, auto const& rhs){
        if (lhs.first == "current") return false;
        if (rhs.first == "current") return true;
        return std::stoi(lhs.first) < std::stoi(rhs.first);
    });

    return result;
  }

  class ConfiguredSIEInputFileStreams {
  public:
    using unique_istream_ptr = std::unique_ptr<std::istream>;
    ConfiguredSIEInputFileStreams& insert(std::string year_id,std::filesystem::path sie_file_path) {
      m_map[year_id] = std::make_unique<std::ifstream>(sie_file_path);
      return *this;
    }
    auto begin() const { return m_map.begin();}
    auto end() const { return m_map.end();}
  private:
    std::map<std::string, unique_istream_ptr> m_map;

    // Note: There is no real gain trying to keep streams on the stack (and thus in cache).
    // For one, the internal data of streams lives on the heap anyways.
    // And the very nature of streams reading from external storage means 
    // the data has to arrive to the cache at some later date anyhows.
    // I tried a design with mstream objects in a vector. This works as
    // stream objects are movable and so is vector. And I used a map
    // string -> index to help with looking up the streams in the vector.
    // This worked but was cumbersome.
    // Final conclusion: Making std::stream which is not copyable live on the stack
    // opened up a whole can of worms best kept closed shut!
  };
  
  ConfiguredSIEInputFileStreams to_configured_sie_input_streams(ConfiguredSIEFilePaths const& configured_sie_file_paths) {
    ConfiguredSIEInputFileStreams result{};
    std::ranges::for_each(configured_sie_file_paths,[&result](auto const& id_path_pair){
      auto const& [year_id,sie_file_path] = id_path_pair;
      result.insert(year_id,sie_file_path);
    });
    return result;
  }

  std::string to_user_cli_feedback(
     Model const& model
    ,SIEEnvironmentsMap::RelativeYearKey year_id
    ,SIEEnvironmentsMap::UpdateFromPostedResult const& change_results) {

		std::ostringstream prompt{};

    if (!change_results.second) {
      // Insert/assign posted failed
      prompt << std::format("Sorry, Update posted for year id:{} failed",year_id);
    }
    else {
      if (std::ranges::any_of(
        change_results.first
        ,[&prompt](auto const& e){ return !static_cast<bool>(e); })) {
        // At least one element is “false”
        prompt << "\n\nSTAGE of cracthit entries FAILED when merging with posted (from external tool)";
        if (model->sie_env_map.meta().posted_sie_files.contains(year_id)) {
          prompt << std::format(
            "\nEntries in sie-file:{} overrides values in cratchit staged entries"
            // ,model->sie_env_map[year_id].source_sie_file_path().string());
            ,model->sie_env_map.meta().posted_sie_files.at(year_id).string());
        }
        else {
          logger::design_insufficiency(
              "model_from_environment_and_files: Expected posted_sie_files[{}] to exist for failed staging of entries"
            ,year_id);
        }
      }

      std::ranges::for_each(
        change_results.first
        ,[&prompt](auto const& entry_result) {
            if (!entry_result) {
              prompt << std::format(
                "\nCONFLICTED! : No longer valid entry {}"
                ,to_string(entry_result.md_entry()));
            }
          }
      );

      auto unposted = model->sie_env_map[year_id].unposted();
      if (unposted.size() > 0) {
        prompt << "\n year id:" << year_id << " - STAGED for posting";
        prompt << "\n" << unposted;
      }
      else {
        prompt << "\n year id:" << year_id << " - ALL POSTED OK";
      }

    }


    return prompt.str();
  }

  SIEEnvironmentsMap sie_env_map_from_all_dotas(DateOrderedTaggedAmountsContainer const& all_dotas) {
    logger::scope_logger log_raii{logger::development_trace,"sie_env_map_from_all_dotas"};

    SIEEnvironmentsMap result{};
    logger::design_insufficiency("sie_env_map_from_all_dotas: NOT YET IMPLEMENTED");
    return result;
  }

  Model model_from_environment(Environment const& environment) {
    logger::scope_logger log_raii{logger::development_trace,"model_from_environment"};

		Model model = std::make_unique<ConcreteModel>();
		std::ostringstream prompt{};

		model->heading_amount_date_entries = hads_from_environment(environment);
		model->organisation_contacts = contacts_from_environment(environment);
		model->employee_birth_ids = employee_birth_ids_from_environment(environment);
		model->sru = srus_from_environment(environment);
    model->all_dotas = dotas_from_environment(environment);
    model->sie_env_map = sie_env_map_from_all_dotas(model->all_dotas);

		model->prompt = prompt.str();
		return model;
  }

  Model model_with_posted_and_staged_env(
     Model model
    ,SIEEnvironmentsMap::RelativeYearKey year_id
    ,SIEEnvironment const& posted_env
    ,SIEEnvironment const& staged_env) {

    logger::scope_logger log_raii{logger::development_trace,"model_with_posted_and_staged_env"};
		std::ostringstream prompt{};

    model->sie_env_map.update_from_posted_and_staged_sie_env(year_id,posted_env,staged_env);

    model->prompt = prompt.str();
    return model;
  }

  Model model_with_posted_sie_files(Model model,Runtime const& runtime) {
    logger::scope_logger log_raii{logger::development_trace,"model_with_posted_sie_files(runtime)"};
    auto const& configured_sie_file_paths = runtime.meta.m_configured_sie_file_paths;

		std::ostringstream prompt{};

    {
      std::ranges::for_each(
        configured_sie_file_paths
        ,[&](auto const& id_path_pair){
          auto const& [year_id,sie_file_path] = id_path_pair;
          // auto update_posted_result = model->sie_env_map.update_posted_from_file(year_id,sie_file_path);
          auto md_maybe_istream = persistent::in::to_md_maybe_istream(sie_file_path);
          auto update_posted_result = model->sie_env_map.update_posted_from_md_istream(
             year_id
            ,md_maybe_istream);
          prompt << to_user_cli_feedback(model,year_id,update_posted_result);
      });
    }

    model->prompt = prompt.str();
    return model;
  }

  // Replaced with model_with_posted_sie_files(model,runtime)
  // Model model_with_posted_sie_files(Model model,ConfiguredSIEFilePaths const& configured_sie_file_paths) {

  //   logger::scope_logger log_raii{logger::development_trace,"model_with_posted_sie_files(configured_sie_file_paths)"};
	// 	std::ostringstream prompt{};

  //   {
  //     std::ranges::for_each(
  //       configured_sie_file_paths
  //       ,[&](auto const& id_path_pair){
  //         auto const& [year_id,sie_file_path] = id_path_pair;
  //         auto update_posted_result = model->sie_env_map.update_posted_from_file(year_id,sie_file_path);
  //         prompt << to_user_cli_feedback(model,year_id,update_posted_result);
  //     });
  //   }

  //   model->prompt = prompt.str();
  //   return model;
  // }

  Model model_with_dotas_from_sie_envs(Model model) {

    logger::scope_logger log_raii{logger::development_trace,"model_with_dotas_from_sie_envs"};
		std::ostringstream prompt{};

    {
      for (auto const& sie_environments_entry : model->sie_env_map) {
        model->all_dotas.dotas_insert_auto_ordered_container(
          dotas_from_sie_environment(sie_environments_entry.second));	
        prompt << std::format(
          "\nSIE year id:{} --> Tagged Amounts = size:{}"
          ,sie_environments_entry.first
          ,model->all_dotas.sequence_size());
      }
    }

    model->prompt = prompt.str();
    return model;
  }

  Model model_with_dotas_from_environment(Model model,Environment const& environment) {

    logger::scope_logger log_raii{logger::development_trace,"model_with_dotas_from_environment"};
		std::ostringstream prompt{};

    model->all_dotas.dotas_insert_auto_ordered_container(
      dotas_from_environment(environment));      

    model->prompt = prompt.str();
    return model;
  }

  Model model_with_dotas_from_account_statement_files(Model model,std::filesystem::path cratchit_file_path) {

    logger::scope_logger log_raii{logger::development_trace,"model_with_dotas_from_account_statement_files"};
		std::ostringstream prompt{};

    model->all_dotas.dotas_insert_auto_ordered_container(
      dotas_from_account_statement_files(cratchit_file_path));
    prompt << std::format(
      "\nAccount Statement Files --> Tagged Amounts = size:{}"
      ,model->all_dotas.sequence_size());

    model->prompt = prompt.str();
    return model;
  }

	Model model_from_environment_and_runtime(Runtime runtime,Environment const& environment) {
    logger::scope_logger log_raii{logger::development_trace,"model_from_environment_and_runtime"};
		std::ostringstream prompt{};
    auto cratchit_file_path = runtime.meta.m_root_path;

    Model model = model_from_environment(environment);
    prompt << model->prompt;

    runtime.meta.m_configured_sie_file_paths = to_configured_posted_sie_file_paths(environment);
    model = model_with_posted_sie_files(
       std::move(model)
      ,runtime
    );
    prompt << model->prompt;

    model = model_with_dotas_from_sie_envs(std::move(model));
    prompt << model->prompt;

    model = model_with_dotas_from_environment(
       std::move(model)
      ,environment);
    prompt << model->prompt;

    model = model_with_dotas_from_account_statement_files(
       std::move(model)
      ,cratchit_file_path);
    prompt << model->prompt;

    // TODO: 240216: Is skv_specs_mapping_from_csv_files still of interest to use for something?
    auto dummy = skv_specs_mapping_from_csv_files(cratchit_file_path,environment);

		model->prompt = prompt.str();
		return model;
  }

	Model model_from_environment_and_files(std::filesystem::path cratchit_file_path,Environment const& environment) {
    logger::scope_logger log_raii{logger::development_trace,"model_from_environment_and_files"};
    Runtime runtime{{
        .m_root_path = cratchit_file_path
      },{
      }};
    return model_from_environment_and_runtime(runtime,environment);
	} // model_from_environment_and_files

  // indexed_env_entries_from now in HADFramework
  // std_overload::generator<EnvironmentIdValuePair> indexed_env_entries_from(HeadingAmountDateTransEntries const& entries) {

	Environment environment_from_model(Model const& model) {

    logger::scope_logger log_raii{logger::development_trace,"environment_from_model"};

		Environment result{};
		auto tagged_amount_to_environment = [&result](TaggedAmount const& ta) {
      // TODO 240524 - Attend to this code when final implemenation of tagged amounts <--> SIE entries are in place
      //               Problem for now is that syncing between tagged amounts and SIE entries is flawed and insufficient (and also error prone)
      if (false) {
        if (ta.tag_value("BAS") or ta.tag_value("SIE")) {
          // std::cout << "\nDESIGN INSUFFICIENCY: No persistent storage yet for SIE or BAS TA:" << ta;
          return; // discard any tagged amounts relating to SIE entries (no persistent storage yet for these)
        }
      }
			result["TaggedAmount"].push_back({to_value_id(ta),to_environment_value(ta)});
		};

    // Removed 20250829 - Saving tagged amunts to persistent storage and app end.
    //                    Currently We can get duplicates in tagged amounts which screws up valid processing.
    //                    So until further notice, we only inject tagged amounts from imported SIE-files.
    //                    And never save anything on shut-down.
    //                    In this way cratchit will internally only have tagged amounts from SIE-file(s).
    //                    No duplication error can occurr.
    // Note:              The problem is when SIE entries are 'edited'. I imagine either the 'old' entries
    //                    remains (as they have different value hash = are uniqie / immutable individs).
    //                    Or, the problem comes from when importing edited SIE entries from an external app?
    //                    Then the 'old' SIE entries as cratchit sees them remains in the persistet storage.
    //                    Finally, the filtering / processing does NOT detect if a BAS account tagged amount
    //                    belongs to an 'active' SIE entry aggregate or not and so processing yelds the wrong result...

    // 20251008 - Disable mechanism controlled with 'disable_ta_persistent_storage' flag

    // static const bool disable_ta_persistent_storage = true;
    static const bool disable_ta_persistent_storage = true;
    if (not disable_ta_persistent_storage) {
  		// model->all_dotas.for_each(tagged_amount_to_environment);
      std::ranges::for_each(
         model->all_dotas.ordered_tagged_amounts()
        ,tagged_amount_to_environment);
    }

    // Log
    if (true) {
      logger::scope_logger log_raii{logger::development_trace,"Model -> Environment Log"};
      logger::development_trace("model->all_dotas.cas().size():{}",model->all_dotas.cas().size());
      logger::development_trace("model->all_dotas.ordered_tagged_amounts().size():{}",model->all_dotas.ordered_tagged_amounts().size());
      if (result.contains("TaggedAmount")) {
        logger::development_trace(R"(result.at("TaggedAmount").size():{})",result.at("TaggedAmount").size());
      }
      else {
        logger::development_trace(R"(result.at("TaggedAmount") - EMPTY)");
      }
    }

    if (result.contains("TaggedAmount") and (result.at("TaggedAmount").size() != model->all_dotas.ordered_tagged_amounts().size())) {
      logger::design_insufficiency(R"(environment_from_model: Failed. Expected same size model->all_dotas.ordered_tagged_amounts():{} != result.at("TaggedAmount").size():{})",model->all_dotas.ordered_tagged_amounts().size(),result.at("TaggedAmount").size());
    }

		// for (auto const& [index,entry] :  std::views::zip(std::views::iota(0),model->heading_amount_date_entries)) {
		// 	// result.insert({"HeadingAmountDateTransEntry",to_environment_value(entry)});
		// 	result["HeadingAmountDateTransEntry"].push_back({index,to_environment_value(entry)});
		// }
    for (auto const& [index, env_val] : indexed_env_entries_from(model->heading_amount_date_entries)) {
        result["HeadingAmountDateTransEntry"].push_back({index, env_val});
    }
    
    // Assemble sie file paths from existing sie environments
		std::string sev = std::accumulate(
       model->sie_env_map.meta().posted_sie_files.begin()
      ,model->sie_env_map.meta().posted_sie_files.end()
      ,std::string{}
      ,[](auto acc,auto const& entry) {
        std::ostringstream os{};
        if (acc.size()>0) os << acc << ";";
        os << entry.first << "=" << entry.second.string();
        return os.str();
		});
		result["sie_file"].push_back({0,in::to_environment_value(sev)});

		for (auto const& [index,entry] : std::views::zip(std::views::iota(0),model->organisation_contacts)) {
			result["contact"].push_back({index,to_environment_value(entry)});
		}
		for (auto const& [index,entry] : std::views::zip(std::views::iota(0),model->employee_birth_ids)) {
			result["employee"].push_back({index,in::to_environment_value(std::string{"birth-id="} + entry)});
		}
    for (auto const& [index,entry] : std::views::zip(std::views::iota(0),model->sru)) {
      auto const& [relative_year_key,sru_env] = entry;
      std::ostringstream os{};
      OEnvironmentValueOStream en_val_os{os};
      en_val_os << "relative_year_key=" << relative_year_key << sru_env;
      result["SRU:S"].push_back({index,in::to_environment_value(os.str())});
    }
		return result;
	} // environment_from_model

}

// namespace to isolate this 'zeroth' variant of cratchin 'main' until refactored to 'next' variant
// (This whole file conatins the 'zeroth' version of cratchit)
namespace zeroth {

	int main(int argc, char *argv[]) {

    logger::business("zeroth::main sais << Hello >> --------------------------------- ");

	if (true) {
		auto map = sie::current_date_to_year_id_map(std::chrono::month{5},7);
	}
	if (false) {
		test_immutable_file_manager();
	}
	if (false) {
		// test_directory_iterator();
		// exit(0);
	}
		if (false) {
			// Log current locale and test charachter encoding.
			// TODO: Activate to adjust for cross platform handling 
				std::cout << "\nDeafult (current) locale setting is " << std::locale().name().c_str();
				std::string sHello{"Hallå Åland! Ömsom ödmjuk. Ärligt äkta."}; // This source file expected to be in UTF-8
		std::cout << "\nUTF-8 sHello:" << std::quoted(sHello);
				// std::string sPATH{std::getenv("PATH")};
		// std::cout << "\nPATH=" << sPATH;
		}
	
  	std::signal(SIGWINCH, handle_winch); // We need a signal handler to not confuse std::cin on console window resize
		std::string command{};
		for (int i=1;i < argc;i++) command+= std::string{argv[i]} + " ";
		auto current_path = std::filesystem::current_path();
		auto environment_file_path = current_path / "cratchit.env";
		REPL repl{environment_file_path};
		repl.run(command);

    logger::business("zeroth::main sais >> Bye << -----------------------------------");

		return 0;
	}

} // zeroth

// char const* ACCOUNT_VAT_CSV now in SKVFramework unit

// namespace charset::CP437 now in charset.hpp/cpp

namespace SKV {
	namespace XML {

		namespace TAXReturns {
			SKV::XML::XMLMap tax_returns_template{
			{R"(Skatteverket^agd:Avsandare^agd:Programnamn)",R"(Programmakarna AB)"}
			,{R"(Skatteverket^agd:Avsandare^agd:Organisationsnummer)",R"(190002039006)"}
			,{R"(Skatteverket^agd:Avsandare^agd:TekniskKontaktperson^agd:Namn)",R"(Valle Vadman)"}
			,{R"(Skatteverket^agd:Avsandare^agd:TekniskKontaktperson^agd:Telefon)",R"(23-2-4-244454)"}
			,{R"(Skatteverket^agd:Avsandare^agd:TekniskKontaktperson^agd:Epostadress)",R"(valle.vadman@programmakarna.se)"}
			,{R"(Skatteverket^agd:Avsandare^agd:Skapad)",R"(2021-01-30T07:42:25)"}

			,{R"(Skatteverket^agd:Blankettgemensamt^agd:Arbetsgivare^agd:AgRegistreradId)",R"(165560269986)"}
			,{R"(Skatteverket^agd:Blankettgemensamt^agd:Arbetsgivare^agd:Kontaktperson^agd:Namn)",R"(Ville Vessla)"}
			,{R"(Skatteverket^agd:Blankettgemensamt^agd:Arbetsgivare^agd:Kontaktperson^agd:Telefon)",R"(555-244454)"}
			,{R"(Skatteverket^agd:Blankettgemensamt^agd:Arbetsgivare^agd:Kontaktperson^agd:Epostadress)",R"(ville.vessla@foretaget.se)"}

			,{R"(Skatteverket^agd:Kontaktperson^agd:Arendeinformation^agd:Arendeagare)",R"(165560269986)"}
			,{R"(Skatteverket^agd:Kontaktperson^agd:Arendeinformation^agd:Period)",R"(202101)"}

			,{R"(Skatteverket^agd:Kontaktperson^agd:Blankettinnehall^agd:HU^agd:ArbetsgivareHUGROUP^agd:AgRegistreradId faltkod="201")",R"(16556026998)"}
			,{R"(Skatteverket^agd:Kontaktperson^agd:Blankettinnehall^agd:HU^agd:RedovisningsPeriod faltkod="006")",R"(202101)"}
			,{R"(Skatteverket^agd:Kontaktperson^agd:Blankettinnehall^agd:HU^agd:SummaArbAvgSlf faltkod="487")",R"(0)"}
			,{R"(Skatteverket^agd:Kontaktperson^agd:Blankettinnehall^agd:HU^agd:SummaSkatteavdr faltkod="497")",R"(0)"}

			,{R"(Skatteverket^agd:Blankett^agd:Arendeinformation^agd:Arendeagare)",R"(165560269986)"}
			,{R"(Skatteverket^agd:Blankett^agd:Arendeinformation^agd:Period)",R"(202101)"}

			,{R"(Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:IU^agd:ArbetsgivareIUGROUP^agd:AgRegistreradId faltkod="201"))",R"(165560269986)"}
			,{R"(Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:IU^agd:BetalningsmottagareIUGROUP^agd:BetalningsmottagareIDChoice^agd:BetalningsmottagarId faltkod="215")",R"(198202252386)"}
			,{R"(Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:IU^agd:RedovisningsPeriod faltkod="006")",R"(202101)"}
			,{R"(Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:IU^agd:Specifikationsnummer faltkod="570")",R"(001)"}
			,{R"(Skatteverket^agd:Blankett^agd:Blankettinnehall^agd:IU^agd:AvdrPrelSkatt faltkod="001")",R"(0)"}
			};
		} // namespace TAXReturns
	} // namespace XML

	namespace SRU {
		namespace INK1 {
      // csv-format of file "INK1_SKV2000-32-02-0022-02_SKV269.xls" in zip file "_Nyheter_from_beskattningsperiod_2022P4-3.zip" from web-location https://skatteverket.se/download/18.48cfd212185efbb440b65a8/1679495970044/_Nyheter_from_beskattningsperiod_2022P4-3.zip
      // Also see project folder "cratchit/skv_specs".
			const char* ink1_csv_to_sru_template = R"(Fältnamn på INK1_SKV2000-32-02-0022-02;;;;;
;;;;;
;;;;;
Attribut;Fältnamn;Datatyp;Obl.;*/+/-;Regel
Framställningsdatum;DatFramst;Datum_A;J;;
Framställningstid;TidFramst;Tid_A;J;;
Fältkodsnummer;FältKod;Numeriskt_E;N;;
Intern information för framställande program/system;SystemInfo;Str_250;N;;
Korrekt person-/samordningsnummer eller organisationsnummer som inleds med 161 (dödsbo) eller 163 (gemensamma distriktet);PersOrgNr;Orgnr_Id_PD;J;;
1.1 Lön, förmåner, sjukpenning m.m.;1000;Numeriskt_10;N;*;
1.2 Kostnadsersättningar;1001;Numeriskt_10;N;*;
1.3 Allmän pension och tjänstepension m.m.;1002;Numeriskt_10;N;*;
1.4 Privat pension och livränta;1003;Numeriskt_10;N;*;
1.5 Andra inkomster som inte är pensionsgrundande;1004;Numeriskt_10;N;*;
1.6 Inkomster, t.ex. hobby som du själv ska betala egenavgifter för;1005;Numeriskt_10;N;*;
1.7 Inkomst enligt bilaga K10, K10A och K13;1006;Numeriskt_10;N;*;
2.1 Resor till och från arbetet;1070;Numeriskt_10;N;*;
2.2 Tjänsteresor;1071;Numeriskt_10;N;*;
2.3 Tillfälligt arbete, dubbel bosättning och hemresor;1072;Numeriskt_10;N;*;
2.4 Övriga utgifter;1073;Numeriskt_10;N;*;
3.1 Socialförsäkringsavgifter enligt EU-förordning m.m.;1501;Numeriskt_10;N;*;
4.1 Rotarbete;1583;Numeriskt_10;N;*;
4.2 Rutarbete;1584;Numeriskt_10;N;*;
4.3 Förnybar el;1582;Numeriskt_10;N;*;
4.4 Gåva;1581;Numeriskt_10;N;*;
4.5 Installation av grön teknik;1585;Numeriskt_10;N;*;
5.1 Småhus/ägarlägenhet  0,75 %;80;Numeriskt_B;N;*;
6.1 Småhus/ägarlägenhet: tomtmark, byggnad under uppförande 1,0 %;84;Numeriskt_B;N;*;
7.1 Schablonintäkter;1106;Numeriskt_10;N;*;
7.2 Ränteinkomster, utdelningar m.m. Vinst enligt bilaga K4 avsnitt C m.m.;1100;Numeriskt_10;N;*;
7.3 Överskott vid uthyrning av privatbostad;1101;Numeriskt_10;N;*;
"7.4 Vinst fondandelar m.m. Vinst från bilaga K4 avsnitt A och B, K9 avsnitt B, 
K10, K10A, K11, K12 avsnitt B och K13.";1102;Numeriskt_10;N;*;
7.5 Vinst ej marknadsnoterade fondandelar m.m. Vinst från bilaga K4 avsnitt D, K9 avsnitt B, K12 avsnitt C och K15A/B m.m.;1103;Numeriskt_10;N;*;
7.6 Vinst från bilaga K5 och K6. Återfört uppskov från bilaga K2;1104;Numeriskt_10;N;*;
7.7 Vinst från bilaga K7 och K8;1105;Numeriskt_10;N;*;
8.1 Ränteutgifter m.m. Förlust enligt bilaga K4 avsnitt C m.m.;1170;Numeriskt_10;N;*;
"8.3 Förlust fondandelar m.m. Förlust från bilaga K4 avsnitt A, K9 avsnitt B, K10,
K12 avsnitt B och K13.";1172;Numeriskt_10;N;*;
"8.4 Förlust ej marknadsnoterade fondandelar. Förlust från bilaga K4 avsnitt D, K9 avsnitt B,  K10A,
K12 avsnitt Coch K15A/B.";1173;Numeriskt_10;N;*;
8.5 Förlust från bilaga K5 och K6;1174;Numeriskt_10;N;*;
8.6 Förlust enligt bilaga K7 och K8;1175;Numeriskt_10;N;*;
8.7 Investeraravdrag från bilaga K11;1176;Numeriskt_10;N;*;
9.1 Skatteunderlag för kapitalförsäkring;1550;Numeriskt_10;N;*;
9.2 Skatteunderlag för pensionsförsäkring;1551;Numeriskt_10;N;*;
10.1 Överskott av aktiv näringsverksamhet enskild;1200;Numeriskt_10;N;*;
10.1 Överskott av aktiv näringsverksamhet handelsbolag;1201;Numeriskt_10;N;*;
10.2 Underskott av aktiv näringsverksamhet enskild;1202;Numeriskt_10;N;*;
10.2 Underskott av aktiv näringsverksamhet handelsbolag;1203;Numeriskt_10;N;*;
10.3 Överskott av passiv näringsverksamhet enskild;1250;Numeriskt_10;N;*;
10.3 Överskott av passiv näringsverksamhet handelsbolag;1251;Numeriskt_10;N;*;
10.4 Underskott av passiv näringsverksamhet enskild;1252;Numeriskt_10;N;*;
10.4 Underskott av passiv näringsverksamhet handelsbolag;1253;Numeriskt_10;N;*;
10.5 Inkomster för vilka uppdragsgivare ska betala socialavgifter Brutto;1400;Numeriskt_10;N;*;
10.5 Inkomster för vilka uppdragsgivare ska betala socialavgifter Kostnader;1401;Numeriskt_10;N;*;
10.6 Underlag för särskild löneskatt på pensionskostnader eget;1301;Numeriskt_10;N;*;
10.6 Underlag för särskild löneskatt på pensionskostnader anställdas;1300;Numeriskt_10;N;*;
10.7 Underlag för avkastningsskatt på pensionskostnader;1305;Numeriskt_10;N;*;
11.1 Positiv räntefördelning;1570;Numeriskt_10;N;*;
11.2 Negativ räntefördelning;1571;Numeriskt_10;N;*;
12.1 Underlag för expansionsfondsskatt ökning;1310;Numeriskt_10;N;*;
12.2 Underlag för expansionsfondsskatt minskning;1311;Numeriskt_10;N;*;
13.1 Regionalt nedsättningsbelopp, endast näringsverksamhet i stödområde;1411;Numeriskt_10;N;*;
14.1 Allmänna avdrag underskott näringsverksamhet;1510;Numeriskt_10;N;*;
15.1 Hyreshus: bostäder 0,3 %;93;Numeriskt_B;N;*;
16.1 Hyreshus: tomtmark, bostäder under uppförande 0,4 %;86;Numeriskt_B;N;*;
16.2 Hyreshus: lokaler 1,0 %;95;Numeriskt_B;N;*;
16.3 Industri och elproduktionsenhet, värmekraftverk 0,5 %;96;Numeriskt_B;N;*;
16.4 Elproduktionsenhet: vattenkraftverk 0,5 %;97;Numeriskt_B;N;*;
16.5 Elproduktionsenhet: vindkraftverk 0,2 %;98;Numeriskt_B;N;*;
17.1 Inventarieinköp under 2021;1586;Numeriskt_B;N;*;
Kryssa här om din näringsverksamhet har upphört;92;Str_X;N;;
Kryssa här om du begär omfördelning av skattereduktion för rot/-rutavdrag eller installation av grön teknik.;8052;Str_X;N;;
Kryssa här om någon inkomstuppgift är felaktig/saknas;8051;Str_X;N;;
Kryssa här om du har haft inkomst från utlandet;8055;Str_X;N;;
Kryssa här om du begär avräkning av utländsk skatt.;8056;Str_X;N;;
Övrigt;8090;Str_1000;N;;

;;;;
;Dokumenthistorik;;;
;Datum;Version;Beskrivning;Signatur
;;;;
;;;;
;;;;
;;;;
;;;;
;Referenser;;;
;"Definition av format återfinns i SKV 269 ""Teknisk beskrivning Näringsuppgifter(SRU) Anstånd Tjänst Kapital""";;;)";
      // csv-format of file "K10_SKV2110-35-04-22-01.xls" in zip file "_Nyheter_from_beskattningsperiod_2022P4-3.zip" from web-location https://skatteverket.se/download/18.48cfd212185efbb440b65a8/1679495970044/_Nyheter_from_beskattningsperiod_2022P4-3.zip
      // Also see project folder "cratchit/skv_specs".
			const char* k10_csv_to_sru_template = R"(Fältnamn på K10_SKV2110-35-04-22-01;;;;;
;;;;;
;;;;;
Attribut;Fältnamn;Datatyp;Obl.;*/+/-;Regel
Framställningsdatum;DatFramst;Datum_A;J;;
Framställningstid;TidFramst;Tid_A;J;;
Fältkodsnummer;FältKod;Numeriskt_E;N;;
Intern information för framställande program/system;SystemInfo;Str_250;N;;
Korrekt person-/samordningsnummer eller organisationsnummer som inleds med 161 (dödsbo) eller 163 (gemensamma distriktet);PersOrgNr;Orgnr_Id_PD;J;;
Uppgiftslämnarens namn;Namn;Str_250;N;;
Företagets organisationsnummer;4530;Orgnr_Id_O;J;;Ingen kontroll av värdet sker om 7050 är ifyllt
Antal ägda andelar vid årets ingång;4531;Numeriskt_B;N;;
Totala antalet andelar i hela företaget vid årets ingång;4532;Numeriskt_B;N;;
Bolaget är ett utländskt bolag;7050;Str_X;N;;
1.1 Årets gränsbelopp enligt förenklingsregeln;4511;Numeriskt_B;N;+;Regel_E
1.2 Sparat utdelningsutrymme från föregående år x 103,23 %;4501;Numeriskt_B;N;+;Regel_E
1.3 Gränsbelopp enligt förenklingsregeln;4502;Numeriskt_B;N;*;Regel_E
1.4 Vid avyttring eller gåva innan utdelningstillfället...;4720;Numeriskt_B;N;-;Regel_E
1.5 Gränsbelopp att ytnyttja vid  p. 1.7 nedan;4503;Numeriskt_B;N;*;Regel_E
1.6 Utdelning;4504;Numeriskt_B;N;+;Regel_E
1.7 Gränsbeloppet enligt p. 1.5 ovan;4721;Numeriskt_B;N;-;Regel_E
1.8 Utdelning som beskattas i tjänst;4505;Numeriskt_B;N;+;Regel_E
1.9 Sparat utdelningsutrymme;4722;Numeriskt_B;N;-;Regel_E
1.10 Vid delavyttring eller gåva efter utdelningstillfället : sparat utdelningsutrymme som är hänförligt till de överlåtna andelarna enligt p. 3.7a;4723;Numeriskt_B;N;-;Regel_E
1.11 Sparat utdelningsutrymme till nästa år;4724;Numeriskt_B;N;*;Regel_E
1.12 Utdelning;4506;Numeriskt_B;N;+;Regel_E
1.13 Belopp som beskattas i tjänst enligt p. 1.8 ovan;4725;Numeriskt_B;N;-;Regel_E
1.14 Utdelning i kapital;4507;Numeriskt_B;N;*;Regel_E
1.15 Beloppet i p. 1.5 x 2/3;4508;Numeriskt_B;N;+;Regel_E
1.16 Resterande utdelning;4509;Numeriskt_B;N;+;Regel_E
1.17 Utdelning som ska tas upp i kapital;4515;Numeriskt_B;N;*;Regel_E
2.1 Omkostnadsbelopp vid årets ingång (alternativt omräknat omkostnadsbelopp) x 9,23 %;4520;Numeriskt_B;N;+;Regel_F
2.2 Lönebaserat utrymme enligt avsnitt D p. 6.11;4521;Numeriskt_B;N;+;Regel_F
2.3 Sparat utdelningsutrymme från föregående år x 103,23 %;4522;Numeriskt_B;N;+;Regel_F
2.4 Gränsbelopp enligt huvudregeln;4523;Numeriskt_B;N;*;Regel_F
2.5 Vid avyttring eller gåva..innanutdelningstillfället...;4730;Numeriskt_B;N;-;Regel_F
2.6 Gränsbelopp att utnyttja vid punkt 2.8 nedan;4524;Numeriskt_B;N;*;Regel_F
2.7 Utdelning;4525;Numeriskt_B;N;+;Regel_F
2.8 Gränsbeloppet enligt p. 2.6 ovan;4731;Numeriskt_B;N;-;Regel_F
2.9 Utdelning som beskattas i tjänst;4526;Numeriskt_B;N;+;Regel_F
2.10 Sparat utdelningsutrymme;4732;Numeriskt_B;N;-;Regel_F
2.11 Vid delavyttring eller gåva efter utdelningstillfället......;4733;Numeriskt_B;N;-;Regel_F
2.12 Sparat utdelningsutrymme till nästa år;4734;Numeriskt_B;N;*;Regel_F
2.13 Utdelning;4527;Numeriskt_B;N;+;Regel_F
2.14 Belopp som beskattas i tjänst enligt p. 2.9 ovan;4735;Numeriskt_B;N;-;Regel_F
2.15 Utdelning i kapital;4528;Numeriskt_B;N;*;Regel_F
2.16 Beloppet i p. 2.6 x 2/3. Om beloppet i p. ...;4529;Numeriskt_B;N;+;Regel_F
2.17 Resterande utdelning...;4540;Numeriskt_B;N;+;Regel_F
2.18 Utdelning som ska tas upp i kapital;4541;Numeriskt_B;N;*;Regel_F
Antal sålda andelar;4543;Numeriskt_B;N;*;
Försäljningsdatum;4544;Datum_D;N;;
3.1 Ersättning minus utgifter för avyttring;4545;Numeriskt_B;N;+;
3.2 Verkligt omkostnadsbelopp;4740;Numeriskt_B;N;-;
3.3 Vinst;4546;Numeriskt_B;N;+;
3.4a. Förlust;4741;Numeriskt_B;N;-;
3.4b. Förlust i p. 3.4a x 2/3;4742;Numeriskt_B;N;*;
3.5 Ersättning minus utgifter för avyttring...;4547;Numeriskt_B;N;+;
3.6 Omkostnadsbelopp (alternativt omräknat omkostnadsbelopp);4743;Numeriskt_B;N;-;
3.7a. Om utdelning erhållits under året...;4744;Numeriskt_B;N;-;
3.7b. Om utdelning erhållits efter delavyttring...;4745;Numeriskt_B;N;-;
3.8 Skattepliktig vinst som ska beskattas i tjänst. (Max 7 100 000 kr);4548;Numeriskt_B;N;*;
3.9 Vinst enligt p. 3.3 ovan;4549;Numeriskt_B;N;+;
3.10 Belopp som beskattas i tjänst enligt p. 3.8 (max 7 100 000 kr;4746;Numeriskt_B;N;-;
3.11 Vinst i inkomstslaget kapital;4550;Numeriskt_B;N;*;
3.12. Belopp antingen enligt p. 3.7a x 2/3 eller 3.7b x 2/3. Om beloppet i p. 3.7a eller 3.7b är större än vinsten i p. 3.11 tas istället 2/3 av vinsten i p. 3.11 upp.;4551;Numeriskt_B;N;+;
"3.13 Resterande vinst (p. 3.11 minus antingen p. 3.7a eller
p. 3.7b). Beloppet kan inte bli lägre än 0 kr.";4552;Numeriskt_B;N;+;
3.14 Vinst som ska tas upp i inkomstslaget kapital;4553;Numeriskt_B;N;*;
Den i avsnitt B redovisade överlåtelsen avser avyttring (ej efterföljande byte) av andelar som har förvärvats genom andelsbyte;4555;Str_X;N;;
4.1 Det omräknade omkostnadsbeloppet har beräknats enligt Indexregeln (andelar anskaffade före 1990);4556;Str_X;N;;
4.2 Det omräknade omkostnadsbeloppet har beräknats enligt Kapitalunderlagsregeln (andelar anskaffade före 1992);4557;Str_X;N;;
Lönekravet uppfylls av närstående. Personnummer;4560;Orgnr_Id_PD;N;;
5.1 Din kontanta ersättning från företaget och dess dotterföretag under 2021;4561;Numeriskt_B;N;*;
5.2 Sammanlagd kontant ersättning i företaget och dess dotterföretag under 2021;4562;Numeriskt_B;N;*;
5.3 (Punkt 5.2 x 5%) + 409 200 kr;4563;Numeriskt_B;N;*;
Om löneuttag enligt ovan gjorts i dotterföretag. Organisationsnummer;4564;Orgnr_Id_O;N;;Ingen kontroll av värdet sker om 7060 är ifyllt
Om löneuttag enligt ovan gjorts i dotterföretag. Organisationsnummer;4565;Orgnr_Id_O;N;;Ingen kontroll av värdet sker om 7060 är ifyllt
Om löneuttag enligt ovan gjorts i dotterföretag. Organisationsnummer;4566;Orgnr_Id_O;N;;Ingen kontroll av värdet sker om 7060 är ifyllt
Utländskt dotterföretag;7060;Str_X;N;;
6.1 Kontant ersättning till arbetstagare under 2021...;4570;Numeriskt_B;N;+;
6.2 Kontant ersättning under 2021...;4571;Numeriskt_B;N;+;
6.3 Löneunderlag;4572;Numeriskt_B;N;*;
6.4 Löneunderlag enligt p. 6.3 x 50 %;4573;Numeriskt_B;N;*;
6.5 Ditt lönebaserade utrymme för andelar ägda under hela 2021...;4576;Numeriskt_B;N;*;
6.6 Kontant ersättning till arbetstagare under...;4577;Numeriskt_B;N;+;
6.7 Kontant ersättning under...;4578;Numeriskt_B;N;+;
6.8 Löneunderlag avseende den tid under 2021 andelarna ägts;4579;Numeriskt_B;N;*;
6.9 Löneunderlag enligt p. 6.8 x 50 %;4580;Numeriskt_B;N;+;
6.10 Ditt lönebaserade utrymme för andelar som anskaffats under 2021.;4583;Numeriskt_B;N;*;
6.11 Totalt lönebaserat utrymme;4584;Numeriskt_B;N;*;)";			
		} // INK1

    namespace INK2 {
      // SKV::SRU data for organisations is defined by https://skatteverket.se/foretag/inkomstdeklaration/forredovisningsbyraer/tekniskinformationomfiloverforing.4.13948c0e18e810bfa0cca8.html

      namespace Y_2024 {
        // Also in INK2_SKV2002-33-01-24-04.csv
    		const char* INK2_csv_to_sru_template{R"INK2(INK2: Table 1
Fältnamn på INK2_SKV2002-33-01-24-04;;;;;
;;;;;
;;;;;
Attribut;Fältnamn;Datatyp;Obl.;*/+/-;Regel
Framställningsdatum;DatFramst;Datum_A;J;;
Framställningstid;TidFramst;Tid_A;J;;
Fältkodsnummer;FältKod;Numeriskt_E;N;;
Intern information för framställande program/system;SystemInfo;Str_250;N;;
Korrekt organisationsnummer;PersOrgNr;Orgnr_Id_O;J;;
Räkenskapsårets början;7011;Datum_D;N;;
Räkenskapsårets slut;7012;Datum_D;N;;"Tillåtna bokslutsdatum.
Period     fr.o.m           t.o.m
ÅÅÅÅP1 ÅÅÅÅ0101 ÅÅÅÅ0430
ÅÅÅÅP2 ÅÅÅÅ0501 ÅÅÅÅ0630
ÅÅÅÅP3 ÅÅÅÅ0701 ÅÅÅÅ0831
ÅÅÅÅP4 ÅÅÅÅ0901 ÅÅÅÅ1231"
1.1 Överskott av näringsverksamhet;7104;Numeriskt_B;N;*;
1.2 Underskott av näringsverksamhet;7114;Numeriskt_B;N;*;
1.3 Kreditinstituts underlag för riskskatt;7131;Numeriskt_13;N;*;
1.4 Underlag för särskild löneskatt på pensionskostnader;7132;Numeriskt_B;N;*;Får ej förekomma om 7133 finns med och den <> 0.
1.5 Negativt underlag för särskild löneskatt på pensionskostnader;7133;Numeriskt_B;N;*;Får ej förekomma om 7132 finns med och den <> 0.
1.6 a Försäkringsföretag m.fl. samt avsatt till pensioner 15 %;7153;Numeriskt_B;N;*;
1.6 b Utländska pensions- försäkringar 15 %;7154;Numeriskt_B;N;*;
1.7 a Försäkringsföretag m.fl 30 %;7155;Numeriskt_B;N;*;
"1.7 b Utländska kapitalförsäkringar 
 30 %";7156;Numeriskt_B;N;*;
1.8 Småhus/ ägarlägenhet;80;Numeriskt_B;N;*;
"1.9 Hyreshus: 
 bostäder";93;Numeriskt_B;N;*;
1.10 Småhus/ägarlägenhet: tomtmark, byggnad under uppförande;84;Numeriskt_B;N;*;
1.11 Hyreshus: tomtmark, bostäder under uppförande;86;Numeriskt_B;N;*;
1.12 Hyreshus: lokaler;95;Numeriskt_B;N;*;
1.13 Industrienhet och elproduktionsenhet: värme- kraftverk;96;Numeriskt_B;N;*;
1.14 Elproduktionsenhet, vattenkraftverk;97;Numeriskt_B;N;*;
1.15 Elproduktionsenhet, vindkraftverk;98;Numeriskt_B;N;*;
Övriga upplysningar på bilaga;90;Str_X;N;;
1.16 Förnybar el (kilowattimmar);1582;Numeriskt_10;N;*;

Revisonshistorik: Table 1
;;;;
;Dokumenthistorik;;;
;Datum;Version;Beskrivning;Signatur
;;;;
;;;;
;;;;
;;;;
;;;;
;Referenser;;;
;"Definition av format återfinns i SKV 269 ""Teknisk beskrivning Näringsuppgifter(SRU) Anstånd Tjänst Kapital""";;;)INK2"};

        // Also in INK2R_SKV2002-33-01-24-04.csv
    		const char* INK2R_csv_to_sru_template{R"INK2(Table 1
Fältnamn på INK2R_SKV2002-33-01-24-04;;;;;
;;;;;
;;;;;
Attribut;Fältnamn;Datatyp;Obl.;*/+/-;Regel
Framställningsdatum;DatFramst;Datum_A;J;;
Framställningstid;TidFramst;Tid_A;J;;
Fältkodsnummer;FältKod;Numeriskt_E;N;;
Intern information för framställande program/system;SystemInfo;Str_250;N;;
Korrekt organisationsnummer;PersOrgNr;Orgnr_Id_O;J;;
Uppgiftslämnarens namn;Namn;Str_250;N;;
Räkenskapsårets början;7011;Datum_D;N;;
Räkenskapsårets slut;7012;Datum_D;N;;
2.1 Immateriella anläggningstillgångar Koncessioner, patent, licenser, varumärken, hyresrätter, goodwill och liknande rättigheter;7201;Numeriskt_A;N;*;
2.2 Immateriella anläggningstillgångar Förskott avseende immateriella anläggningstillgångar;7202;Numeriskt_A;N;*;
2.3 Materiella anläggningstillgångar Byggnader och mark;7214;Numeriskt_A;N;*;
2.4 Materiella anläggningstillgångar Maskiner, inventarier och övriga materiella anläggningstillgångar;7215;Numeriskt_A;N;*;
2.5 Materiella anläggningstillgångar Förbättringsutgifter på annans fastighet;7216;Numeriskt_A;N;*;
2.6 Materiella anläggningstillgångar Pågående nyanläggningar och förskott avseende materiella anläggningstillgångar;7217;Numeriskt_A;N;*;
2.7 Finansiella anläggningstillgångar Andelar i koncernföretag;7230;Numeriskt_A;N;*;
2.8 Finansiella anläggningstillgångar Andelar i intresseföretag och gemensamt styrda företag;7231;Numeriskt_A;N;*;
"2.9 Ägarintressen i övriga företag och 
 andra långfristiga värdepappersinnehav";7233;Numeriskt_A;N;*;
2.10 Finansiella anläggningstillgångar Fordringar hos koncern-, intresse- och  gemensamt styrda företag;7232;Numeriskt_A;N;*;
2.11 Finansiella anläggningstillgångar Lån till delägare eller närstående;7234;Numeriskt_A;N;*;
"2.12 Fordringar hos övriga företag som 
 det finns ett ägarintresse i och 
 andra långfristiga fordringar";7235;Numeriskt_A;N;*;
2.13 Varulager m.m. Råvaror och förnödenheter;7241;Numeriskt_A;N;*;
2.14 Varulager Varor under tillverkning;7242;Numeriskt_A;N;*;
2.15 Varulager Färdiga varor och handelsvaror;7243;Numeriskt_A;N;*;
2.16 Varulager Övriga lagertillgångar;7244;Numeriskt_A;N;*;
2.17 Varulager Pågående arbeten för annans räkning;7245;Numeriskt_A;N;*;
2.18 Varulager Förskott till leverantörer;7246;Numeriskt_A;N;*;
2.19 Kortfristiga fordringar Kundfordringar;7251;Numeriskt_A;N;*;
2.20 Kortfristiga fordringar Fordringar hos koncern-, intresse- och gemensamt styrda företag;7252;Numeriskt_A;N;*;
"2.21 Fordringar hos övriga företag som det 
 finns ett ägarintresse i och 
 övriga fordringar";7261;Numeriskt_A;N;*;
2.22 Kortfristiga fordringar Upparbetad men ej fakturerad intäkt;7262;Numeriskt_A;N;*;
2.23 Kortfristiga fordringar Förutbetalda kostnader och upplupna intäkter;7263;Numeriskt_A;N;*;
2.24 Kortfristiga placeringar Andelar i koncernföretag;7270;Numeriskt_A;N;*;
2.25 Kortfristiga placeringar Övriga kortfristiga placeringar;7271;Numeriskt_A;N;*;
2.26 Kassa och bank Kassa, bank och redovisningsmedel;7281;Numeriskt_A;N;*;
2.27 Eget kapital Bundet eget kapital;7301;Numeriskt_A;N;*;
2.28 Eget kapital Fritt eget kapital;7302;Numeriskt_A;N;*;
2.29 Obeskattade reserver Periodiseringsfonder;7321;Numeriskt_A;N;*;
2.30 Obeskattade reserver Ackumulerade överavskrivningar;7322;Numeriskt_A;N;*;
2.31 Obeskattade reserver Övriga obeskattade reserver;7323;Numeriskt_A;N;*;
2.32 Avsättningar Avsättningar för pensioner och liknande förpliktelser enligt lag (1967:531)...;7331;Numeriskt_A;N;*;
2.33 Avsättningar Övriga avsättningar för pensioner och liknande förpliktelser;7332;Numeriskt_A;N;*;
2.34 Avsättningar Övriga avsättningar;7333;Numeriskt_A;N;*;
2.35 Långfristiga skulder Obligationslån;7350;Numeriskt_A;N;*;
2.36 Långfristiga skulder Checkräkningskredit;7351;Numeriskt_A;N;*;
2.37 Långfristiga skulder Övriga skulder till kreditinstitut;7352;Numeriskt_A;N;*;
2.38 Långfristiga skulder Skulder till koncern-, intresse- och gemensamt styrda företag;7353;Numeriskt_A;N;*;
"2.39 Skulder till övriga företag som det finns 
 ett ägarintresse i och övriga skulder";7354;Numeriskt_A;N;*;
2.40 Kortfristiga skulder Checkräkningskredit;7360;Numeriskt_A;N;*;
2.41 Kortfristiga skulder Övriga skulder till kreditinstitut;7361;Numeriskt_A;N;*;
2.42 Kortfristiga skulder Förskott från kunder;7362;Numeriskt_A;N;*;
2.43 Kortfristiga skulder Pågående arbeten för annans räkning;7363;Numeriskt_A;N;*;
2.44 Kortfristiga skulder Fakturerad men ej upparbetad intäkt;7364;Numeriskt_A;N;*;
2.45 Kortfristiga skulder Leverantörsskulder;7365;Numeriskt_A;N;*;
2.46 Kortfristiga skulder Växelskulder;7366;Numeriskt_A;N;*;
2.47 Kortfristiga skulder Skulder till koncern-, intresse- och gemensamt styrda företag;7367;Numeriskt_A;N;*;
"2.48 Skulder till övriga företag som det finns 
 ett ägarintresse i och övriga skulder";7369;Numeriskt_A;N;*;
2.49 Kortfristiga skulder Skattesskulder;7368;Numeriskt_A;N;*;
2.50 Kortfristiga skulder Upplupna kostnader och förutbetalda intäkter;7370;Numeriskt_A;N;*;
3.1 Nettoomsättning;7410;Numeriskt_A;N;+;
3.2 Förändring av lager av produkter i arbete, färdiga varor och pågående arbete för annans räkning. +;7411;Numeriskt_A;N;+;
3.2 Förändring av lager av produkter i arbete, färdiga varor och pågående arbete för annans räkning. -;7510;Numeriskt_A;N;-;
3.3 Aktiverat arbete för egen räkning;7412;Numeriskt_A;N;+;
3.4 Övriga rörelseintäkter;7413;Numeriskt_A;N;+;
3.5 Råvaror och förnödenheter;7511;Numeriskt_A;N;-;
3.6 Handelsvaror;7512;Numeriskt_A;N;-;
3.7 Övriga externa kostnader;7513;Numeriskt_A;N;-;
3.8 Personalkostnader;7514;Numeriskt_A;N;-;
3.9 Av- och nedskrivningar av materiella och immateriella anläggningstillgångar;7515;Numeriskt_A;N;-;
3.10 Nedskrivningar av omsättningstillgångar utöver normala nedskrivningar;7516;Numeriskt_A;N;-;
3.11 Övriga rörelsekostnader;7517;Numeriskt_A;N;-;
3.12 Resultat från andelar i koncernföretag +;7414;Numeriskt_A;N;+;
3.12 Resultat från andelar i koncernföretag -;7518;Numeriskt_A;N;-;
3.13 Resultat från andelar i intresseföretag och gemensamt styrda företag +;7415;Numeriskt_A;N;+;
3.13 Resultat från andelar i intresseföretag och gemensamt styrda företag -;7519;Numeriskt_A;N;-;
3.14 Resultat från övriga företag som det finns ett ägarintresse i +;7423;Numeriskt_A;N;+;
3.14 Resultat från övriga företag som det finns ett ägarintresse i -;7530;Numeriskt_A;N;-;
3.15 Resultat från övriga finansiella anläggningstillgångar +;7416;Numeriskt_A;N;+;
3.15 Resultat från övriga finansiella anläggningstillgångar -;7520;Numeriskt_A;N;-;
3.16 Övriga ränteintäkter och liknande resultatposter;7417;Numeriskt_A;N;+;
3.17 Nedskrivning av finansiella anläggningstillgångar och kortfristiga placeringar;7521;Numeriskt_A;N;-;
3.18 Räntekostnader och liknande resultatposter;7522;Numeriskt_A;N;-;
3.19 Lämnade koncernbidrag;7524;Numeriskt_A;N;-;
3.20 Mottagna koncernbidrag;7419;Numeriskt_A;N;+;
3.21 Återföring av periodiseringsfond;7420;Numeriskt_B;N;+;
3.22 Avsättning till periodiseringsfond;7525;Numeriskt_B;N;-;
3.23 Förändring av överavskrivningar +;7421;Numeriskt_A;N;+;
3.23 Förändring av överavskrivningar -;7526;Numeriskt_A;N;-;
3.24 Övriga bokslutsdispositioner +;7422;Numeriskt_A;N;+;
3.24 Övriga bokslutsdispositioner -;7527;Numeriskt_A;N;-;
3.25 Skatt på årets resultat;7528;Numeriskt_A;N;-;
3.26 Årets resultat, vinst (flyttas till p. 4.1);7450;Numeriskt_B;N;+;Får ej förekomma om 7550 finns med och 7550 <> 0.
3.27 Årets resultat, förlust (flyttas till p. 4.2);7550;Numeriskt_B;N;-;Får ej förekomma om 7450 finns med och 7450 <> 0.)INK2"};

        // Also in INK2S_SKV2002-33-01-24-04.csv
    		const char* INK2S_csv_to_sru_template{R"INK2(Table 1
Fältnamn på INK2S_SKV2002-33-01-24-04;;;;;
;;;;;
;;;;;
Attribut;Fältnamn;Datatyp;Obl.;*/+/-;Regel
Framställningsdatum;DatFramst;Datum_A;J;;
Framställningstid;TidFramst;Tid_A;J;;
Fältkodsnummer;FältKod;Numeriskt_E;N;;
Intern information för framställande program/system;SystemInfo;Str_250;N;;
Korrekt organisationsnummer;PersOrgNr;Orgnr_Id_O;J;;
Uppgiftslämnarens namn;Namn;Str_250;N;;
Räkenskapsårets början;7011;Datum_D;N;;
Räkenskapsårets slut;7012;Datum_D;N;;
4.1 Årets resultat, vinst;7650;Numeriskt_B;N;+;Får ej förekomma om 7750 finns med och den <> 0.
4.2 Årets resultat, förlust;7750;Numeriskt_B;N;-;Får ej förekomma om 7650 finns med och den <> 0.
4.3a. Bokförda kostnader som inte ska dras av: a. Skatt på årets resultat;7651;Numeriskt_A;N;+;
b. Bokförda kostnader som inte ska dras av: b. Nedskrivning av finansiella tillgångar;7652;Numeriskt_A;N;+;
c. Bokförda kostnader som inte ska dras av: c. Andra bokförda kostnader;7653;Numeriskt_A;N;+;
4.4a. Kostnader som ska dras av men som inte ingår i det redovisade resultatet: a. Lämnade koncernbidrag;7751;Numeriskt_A;N;-;
b. Kostnader som ska dras av men som inte ingår i det redovisade resultatet: b. Andra ej bokförda kostnader;7764;Numeriskt_A;N;-;
4.5a. Bokförda intäkter som inte ska tas upp: a. Ackordsvinster;7752;Numeriskt_A;N;-;
b. Bokförda intäkter som inte ska tas upp: b. Utdelning;7753;Numeriskt_A;N;-;
c. Bokförda intäkter som inte ska tas upp: c. Andra bokförda intäkter;7754;Numeriskt_A;N;-;
"4.6a. Intäkter som ska tas upp men som inte 
 ingår i det redovisade resultatet
a. Beräknad schablonintäkt på 
 periodiseringsfonder vid 
 beskattningsårets ingång";7654;Numeriskt_B;N;+;
"b. Beräknad schablonintäkt på 
 fondandelar ägda vid 
 kalenderårets ingång";7668;Numeriskt_A;N;+;
c. Intäkter som ska tas upp men som inte ingår i det redovisade resultatet: c. Mottagna koncernbidrag;7655;Numeriskt_A;N;+;
d. Intäkter som ska tas upp men som inte ingår i det redovisade resultatet: d. Uppräknat belopp vid återföring av periodiseringsfond;7673;Numeriskt_A;N;+;
e. Intäkter som ska tas upp men som inte ingår i det redovisade resultatet: e. Andra ej bokförda intäkter;7665;Numeriskt_A;N;+;
4.7a. Avyttring av delägarrätter: a. Bokförd vinst;7755;Numeriskt_B;N;-;
b. Avyttring av delägarrätter: b. Bokförd förlust;7656;Numeriskt_B;N;+;
c. Avyttring av delägarrätter: c. Uppskov med kapitalvinst enligt blankett N4;7756;Numeriskt_B;N;-;
d. Avyttring av delägarrätter: d. Återfört uppskov med kapitalvinst enligt blankett N4;7657;Numeriskt_B;N;+;
e. Avyttring av delägarrätter: e. Kapitalvinst för beskattningsåret;7658;Numeriskt_B;N;+;
f. Avyttring av delägarrätter: f. Kapitalförlust som ska dras av;7757;Numeriskt_B;N;-;
4.8a. Andel i handelsbolag (inkl. avyttring): a. Bokförd intäkt/vinst;7758;Numeriskt_B;N;-;
b. Andel i handelsbolag (inkl. avyttring): b. Skattemässigt överskott enligt N3B;7659;Numeriskt_B;N;+;
c. Andel i handelsbolag (inkl. avyttring): c. Bokförd kostnad/förlust;7660;Numeriskt_B;N;+;
d. Andel i handelsbolag (inkl. avyttring): d. Skattemässigt underskott enligt N3B;7759;Numeriskt_B;N;-;
4.9 Skattemässig justering av bokfört resultat för avskrivningar på byggnader och annan fast egendom samt restvärdesavskrivning på maskiner och inventarier (+);7666;Numeriskt_B;N;+;
4.9 Skattemässig justering av bokfört resultat för avskrivningar på byggnader och annan fast egendom samt restvärdesavskrivning på maskiner och inventarier (-);7765;Numeriskt_B;N;-;
"4.10 Skattemässig justering av bokfört 
 resultat vid avyttring av näringsfastighet 
 och näringsbostadsrätt";7661;Numeriskt_B;N;+;
4.10 Skattemässig korrigering av bokfört resultat vid avyttring av näringsfastighet och näringsbostadsrätt: -;7760;Numeriskt_B;N;-;
4.11 Skogs-/substansminskningsavdrag (specificeras på blankett N8);7761;Numeriskt_B;N;-;
4.12 Återföringar vid avyttring av fastighet t.ex. värdeminskningsavdrag, skogsavdrag och substansminskningsavdrag...;7662;Numeriskt_A;N;+;
4.13 Andra skattemässiga justeringar av resultatet: +;7663;Numeriskt_B;N;+;
4.13 Andra skattemässiga justeringar av resultatet: -;7762;Numeriskt_B;N;-;
4.14a. Underskott: a. Outnyttjat underskott från föregående år;7763;Numeriskt_B;N;-;
4.14b. Reduktion av outnyttjat underskott med hänsyn till beloppsspärr, ackord eller konkurs;7671;Numeriskt_A;N;+;
4.14c. Reduktion av outnyttjat underskott med hänsyn till koncernbidragsspärr, fusionsspärr m.m. (beloppet ska också tas upp vid p. 1.2. på sid. 1);7672;Numeriskt_A;N;+;
4.15 Överskott (flyttas till p. 1.1 på sid. 1);7670;Numeriskt_B;N;+;Får ej förekomma om 7770 finns med och 7770 <> 0.
4.16 Underskott (flyttas till p. 1.2 på sid. 1);7770;Numeriskt_B;N;-;Får ej förekomma om 7670 finns med och 7670 <> 0.
4.17 Årets begärda och tidigare års medgivna värdeminskningsavdrag som finns vid beskattningsårets utgång avseende byggnader.;8020;Numeriskt_A;N;*;
4.18 Årets begärda och tidigare års medgivna värdeminskningsavdrag  som finns vid beskattningsårets utgång avseende markanläggningar.;8021;Numeriskt_A;N;*;
"4.19 Vid restvärdesavskrivning:
återförda belopp för av- och nedskrivning,
försäljning, utrangering";8023;Numeriskt_B;N;*;
"4.20 Lån från aktieägare (fysisk person)
vid räkenskapsårets utgång";8026;Numeriskt_B;N;*;
4.21 Pensionskostnader (som ingår i p. 3.8);8022;Numeriskt_A;N;*;
"4.22 Koncernbidragsspärrat och fusionsspärrat 
 underskott m.m. (frivillig uppgift)";8028;Numeriskt_B;N;*;
Uppdragstagare (t.ex. redovisningskonsult) har biträtt vid upprättandet av årsredovisningen: Ja;8040;Str_X;N;;Får ej förekomma om 8041 finns med.
Uppdragstagare (t.ex. redovisningskonsult) har biträtt vid upprättandet av årsredovisningen: Nej;8041;Str_X;N;;Får ej förekomma om 8040 finns med.
Årsredovisningen har varit föremål för revision: Ja;8044;Str_X;N;;Får ej förekomma om 8045 finns med.
Årsredovisningen har varit föremål för revision: Nej;8045;Str_X;N;;Får ej förekomma om 8044 finns med.)INK2"};

  const char* info_ink2_template = R"(#DATABESKRIVNING_START
#PRODUKT SRU
#SKAPAD 20241130 124700
#PROGRAM https://github.com/kjelloh/cratchit
#FILNAMN BLANKETTER.SRU
#DATABESKRIVNING_SLUT
#MEDIELEV_START
#ORGNR 165567828172
#NAMN The ITfied AB
#POSTNR 17141
#POSTORT Solna
#MEDIELEV_SLUT)";

  const char* blanketter_ink2_ink2s_ink2r_template = R"(#BLANKETT INK2R-2024P1
#IDENTITET 165567828172 20240727 195839
#NAMN The ITfied AB
#SYSTEMINFO klarmarkerad u. a.
#UPPGIFT 7011 20230501
#UPPGIFT 7012 20240430
#UPPGIFT 7215 42734
#UPPGIFT 7261 2046
#UPPGIFT 7281 119254
#UPPGIFT 7301 100800
#UPPGIFT 7302 39106
#UPPGIFT 7365 -896
#UPPGIFT 7368 0
#UPPGIFT 7369 25024
#UPPGIFT 7410 1
#UPPGIFT 7417 3588
#UPPGIFT 7513 13451
#UPPGIFT 7515 20685
#UPPGIFT 7528 0
#UPPGIFT 7550 30547
#BLANKETTSLUT
#BLANKETT INK2S-2024P1
#IDENTITET 165567828172 20240727 195839
#NAMN The ITfied AB
#SYSTEMINFO klarmarkerad u. a.
#UPPGIFT 7011 20230501
#UPPGIFT 7012 20240430
#UPPGIFT 7651 0
#UPPGIFT 7653 625
#UPPGIFT 7750 30547
#UPPGIFT 7754 15
#UPPGIFT 7763 176026
#UPPGIFT 7770 205963
#UPPGIFT 8041 X
#UPPGIFT 8045 X
#BLANKETTSLUT
#BLANKETT INK2-2024P1
#IDENTITET 165567828172 20240727 195839
#NAMN The ITfied AB
#SYSTEMINFO klarmarkerad u. a.
#UPPGIFT 7011 20230501
#UPPGIFT 7012 20240430
#UPPGIFT 7114 205963
#BLANKETTSLUT
#FIL_SLUT)";

      } // Y_2024
      
		// Also in resources/INK2_19_P1-intervall-vers-2.csv
    
		const char* INK2_csv_to_sru_template = R"(Fältnamn på INK2_SKV2002-30-01-20-02;;;;;
;;;;;
;;;;;
Attribut;Fältnamn;Datatyp;Obl.;*/+/-;Regel
Framställningsdatum;DatFramst;Datum_A;J;;
Framställningstid;TidFramst;Tid_A;J;;
Fältkodsnummer;FältKod;Numeriskt_E;N;;
Intern information för framställande program/system;SystemInfo;Str_250;N;;
Korrekt organisationsnummer;PersOrgNr;Orgnr_Id_O;J;;
Räkenskapsårets början;7011;Datum_D;N;;
Räkenskapsårets slut;7012;Datum_D;N;;
1.1 Överskott av näringsverksamhet;7104;Numeriskt_B;N;*;
1.2 Underskott av näringsverksamhet;7114;Numeriskt_B;N;*;
1.4 Underlag för särskild löneskatt på pensionskostnader;7132;Numeriskt_B;N;*;Får ej förekomma om 7133 finns med och den <> 0.
1.5 Negativt underlag för särskild löneskatt på pensionskostnader;7133;Numeriskt_B;N;*;Får ej förekomma om 7132 finns med och den <> 0.
1.6 a Underlag för avkastningsskatt 15% Försäkringsföretag m.fl. samt Avsatt till pensioner;7153;Numeriskt_B;N;*;
1.6 b Underlag för avkastningsskatt 15 % Utländska pensionsförsäkringar;7154;Numeriskt_B;N;*;
1.7 a Underlag för avkastningsskatt 30% Försäkringsföretag m.fl.;7155;Numeriskt_B;N;*;
1.7 b Underlag för avkastningsskatt 30 % Utländska kapitalförsäkringar;7156;Numeriskt_B;N;*;
1.8 Småhus/ägarlägenhet hel avgift;80;Numeriskt_B;N;*;
1.8 Småhus/ägarlägenhet halv avgift;82;Numeriskt_B;N;*;
1.9 Hyreshus, bostäder hel avgift;93;Numeriskt_B;N;*;
1.9 Hyreshus, bostäder halv avgift;94;Numeriskt_B;N;*;
1.10 Småhus/ägarlägenhet: tomtmark, byggnad under uppförande;84;Numeriskt_B;N;*;
1.11 Hyreshus: tomtmark, bostäder under uppförande;86;Numeriskt_B;N;*;
1.12 Hyreshus: lokaler;95;Numeriskt_B;N;*;
1.13 Industri/elproduktionsenhet, värmekraftverk (utom vindkraftverk);96;Numeriskt_B;N;*;
1.14 Elproduktionsenhet, vattenkraftverk;97;Numeriskt_B;N;*;
1.15 Elproduktionsenhet, vindkraftverk;98;Numeriskt_B;N;*;
Övriga upplysningar på bilaga;90;Str_X;N;;
Kanal;Kanal;Str_250;N;;

;;;;
;Dokumenthistorik;;;
;Datum;Version;Beskrivning;Signatur
;;;;
;;;;
;;;;
;;;;
;;;;
;Referenser;;;
;"Definition av format återfinns i SKV 269 ""Teknisk beskrivning Näringsuppgifter(SRU) Anstånd Tjänst Kapital""";;;)";

		const char* INK2S_csv_to_sru_template = R"(Fältnamn på INK2S_SKV2002-30-01-20-02;;;;;
;;;;;
;;;;;
Attribut;Fältnamn;Datatyp;Obl.;*/+/-;Regel
Framställningsdatum;DatFramst;Datum_A;J;;
Framställningstid;TidFramst;Tid_A;J;;
Fältkodsnummer;FältKod;Numeriskt_E;N;;
Intern information för framställande program/system;SystemInfo;Str_250;N;;
Korrekt organisationsnummer;PersOrgNr;Orgnr_Id_O;J;;
Uppgiftslämnarens namn;Namn;Str_250;N;;
Räkenskapsårets början;7011;Datum_D;N;;
Räkenskapsårets slut;7012;Datum_D;N;;
4.1 Årets resultat, vinst;7650;Numeriskt_B;N;+;Får ej förekomma om 7750 finns med och den <> 0.
4.2 Årets resultat, förlust;7750;Numeriskt_B;N;-;Får ej förekomma om 7650 finns med och den <> 0.
4.3a. Bokförda kostnader som inte ska dras av: a. Skatt på årets resultat;7651;Numeriskt_A;N;+;
b. Bokförda kostnader som inte ska dras av: b. Nedskrivning av finansiella tillgångar;7652;Numeriskt_A;N;+;
c. Bokförda kostnader som inte ska dras av: c. Andra bokförda kostnader;7653;Numeriskt_A;N;+;
4.4a. Kostnader som ska dras av men som inte ingår i det redovisade resultatet: a. Lämnade koncernbidrag;7751;Numeriskt_A;N;-;
b. Kostnader som ska dras av men som inte ingår i det redovisade resultatet: b. Andra ej bokförda kostnader;7764;Numeriskt_A;N;-;
4.5a. Bokförda intäkter som inte ska tas upp: a. Ackordsvinster;7752;Numeriskt_A;N;-;
b. Bokförda intäkter som inte ska tas upp: b. Utdelning;7753;Numeriskt_A;N;-;
c. Bokförda intäkter som inte ska tas upp: c. Andra bokförda intäkter;7754;Numeriskt_A;N;-;
"4.6a. Intäkter som ska tas upp men som inte ingår i det redovisade resultatet:
a. Beräknad schablonintäkt på kvarvarande periodiseringsfonder vid beskattningsårets ingång";7654;Numeriskt_B;N;+;
b. Intäkter som ska tas upp men som inte ingår i det redovisade resultatet: b. Beräknad schablonintäkt på fondandelar ägda vid ingången av kalenderåret;7668;Numeriskt_A;N;+;
c. Intäkter som ska tas upp men som inte ingår i det redovisade resultatet: c. Mottagna koncernbidrag;7655;Numeriskt_A;N;+;
d. Intäkter som ska tas upp men som inte ingår i det redovisade resultatet: d. Uppräknat belopp vid återföring av periodiseringsfond;7673;Numeriskt_A;N;+;
e. Intäkter som ska tas upp men som inte ingår i det redovisade resultatet: e. Andra ej bokförda intäkter;7665;Numeriskt_A;N;+;
4.7a. Avyttring av delägarrätter: a. Bokförd vinst;7755;Numeriskt_B;N;-;
b. Avyttring av delägarrätter: b. Bokförd förlust;7656;Numeriskt_B;N;+;
c. Avyttring av delägarrätter: c. Uppskov med kapitalvinst enligt blankett N4;7756;Numeriskt_B;N;-;
d. Avyttring av delägarrätter: d. Återfört uppskov med kapitalvinst enligt blankett N4;7657;Numeriskt_B;N;+;
e. Avyttring av delägarrätter: e. Kapitalvinst för beskattningsåret;7658;Numeriskt_B;N;+;
f. Avyttring av delägarrätter: f. Kapitalförlust som ska dras av;7757;Numeriskt_B;N;-;
4.8a. Andel i handelsbolag (inkl. avyttring): a. Bokförd intäkt/vinst;7758;Numeriskt_B;N;-;
b. Andel i handelsbolag (inkl. avyttring): b. Skattemässigt överskott enligt N3B;7659;Numeriskt_B;N;+;
c. Andel i handelsbolag (inkl. avyttring): c. Bokförd kostnad/förlust;7660;Numeriskt_B;N;+;
d. Andel i handelsbolag (inkl. avyttring): d. Skattemässigt underskott enligt N3B;7759;Numeriskt_B;N;-;
4.9 Skattemässig justering av bokfört resultat för avskrivningar på byggnader och annan fast egendom samt restvärdesavskrivning på maskiner och inventarier (+);7666;Numeriskt_B;N;+;
4.9 Skattemässig justering av bokfört resultat för avskrivningar på byggnader och annan fast egendom samt restvärdesavskrivning på maskiner och inventarier (-);7765;Numeriskt_B;N;-;
4.10 Skattemässig korrigering av bokfört resultat vid avyttring av näringsfastighet och näringsbostadsrätt: +;7661;Numeriskt_B;N;+;
4.10 Skattemässig korrigering av bokfört resultat vid avyttring av näringsfastighet och näringsbostadsrätt: -;7760;Numeriskt_B;N;-;
4.11 Skogs-/substansminskningsavdrag (specificeras på blankett N8);7761;Numeriskt_B;N;-;
4.12 Återföringar vid avyttring av fastighet t.ex. värdeminskningsavdrag, skogsavdrag och substansminskningsavdrag...;7662;Numeriskt_A;N;+;
4.13 Andra skattemässiga justeringar av resultatet: +;7663;Numeriskt_B;N;+;
4.13 Andra skattemässiga justeringar av resultatet: -;7762;Numeriskt_B;N;-;
4.14a. Underskott: a. Outnyttjat underskott från föregående år;7763;Numeriskt_B;N;-;
4.14b. Reduktion av outnyttjat underskott med hänsyn till beloppsspärr, ackord eller konkurs;7671;Numeriskt_A;N;+;
4.14c. Reduktion av outnyttjat underskott med hänsyn till koncernbidragsspärr, fusionsspärr m.m.;7672;Numeriskt_A;N;+;
4.15 Överskott (flyttas till p. 1.1 på sid. 1);7670;Numeriskt_B;N;+;Får ej förekomma om 7770 finns med och 7770 <> 0.
4.16 Underskott (flyttas till p. 1.2 på sid. 1);7770;Numeriskt_B;N;-;Får ej förekomma om 7670 finns med och 7670 <> 0.
4.17 Årets begärda och tidigare års medgivna värdeminskningsavdrag som finns vid beskattningsårets utgång avseende byggnader.;8020;Numeriskt_A;N;*;
4.18 Årets begärda och tidigare års medgivna värdeminskningsavdrag  som finns vid beskattningsårets utgång avseende markanläggningar.;8021;Numeriskt_A;N;*;
"4.19 Vid restvärdesavskrivning:
återförda belopp för av- och nedskrivning,
försäljning, utrangering";8023;Numeriskt_B;N;*;
"4.20 Lån från aktieägare (fysisk person)
vid räkenskapsårets utgång";8026;Numeriskt_B;N;*;
4.21 Pensionskostnader (som ingår i p. 3.8);8022;Numeriskt_A;N;*;
4.22 Koncernbidrags-, fusionsspärrat underskott m.m.;8028;Numeriskt_B;N;*;
Uppdragstagare (t.ex. redovisningskonsult) har biträtt vid upprättandet av årsredovisningen: Ja;8040;Str_X;N;;Får ej förekomma om 8041 finns med.
Uppdragstagare (t.ex. redovisningskonsult) har biträtt vid upprättandet av årsredovisningen: Nej;8041;Str_X;N;;Får ej förekomma om 8040 finns med.
Årsredovisningen har varit föremål för revision: Ja;8044;Str_X;N;;Får ej förekomma om 8045 finns med.
Årsredovisningen har varit föremål för revision: Nej;8045;Str_X;N;;Får ej förekomma om 8044 finns med.)";
		const char* INK2R_csv_to_sru_template = R"(Fältnamn på INK2R_SKV2002-30-01-20-02;;;;;
;;;;;
;;;;;
Attribut;Fältnamn;Datatyp;Obl.;*/+/-;Regel
Framställningsdatum;DatFramst;Datum_A;J;;
Framställningstid;TidFramst;Tid_A;J;;
Fältkodsnummer;FältKod;Numeriskt_E;N;;
Intern information för framställande program/system;SystemInfo;Str_250;N;;
Korrekt organisationsnummer;PersOrgNr;Orgnr_Id_O;J;;
Uppgiftslämnarens namn;Namn;Str_250;N;;
Räkenskapsårets början;7011;Datum_D;N;;
Räkenskapsårets slut;7012;Datum_D;N;;
2.1 Immateriella anläggningstillgångar Koncessioner, patent, licenser, varumärken, hyresrätter, goodwill och liknande rättigheter;7201;Numeriskt_A;N;*;
2.2 Immateriella anläggningstillgångar Förskott avseende immateriella anläggningstillgångar;7202;Numeriskt_A;N;*;
2.3 Materiella anläggningstillgångar Byggnader och mark;7214;Numeriskt_A;N;*;
2.4 Materiella anläggningstillgångar Maskiner, inventarier och övriga materiella anläggningstillgångar;7215;Numeriskt_A;N;*;
2.5 Materiella anläggningstillgångar Förbättringsutgifter på annans fastighet;7216;Numeriskt_A;N;*;
2.6 Materiella anläggningstillgångar Pågående nyanläggningar och förskott avseende materiella anläggningstillgångar;7217;Numeriskt_A;N;*;
2.7 Finansiella anläggningstillgångar Andelar i koncernföretag;7230;Numeriskt_A;N;*;
2.8 Finansiella anläggningstillgångar Andelar i intresseföretag och gemensamt styrda företag;7231;Numeriskt_A;N;*;
2.9 Finansiella anläggningstillgångar Ägarintressen i övriga företag och Andra långfristiga värdepappersinnehav;7233;Numeriskt_A;N;*;
2.10 Finansiella anläggningstillgångar Fordringar hos koncern-, intresse- och  gemensamt styrda företag;7232;Numeriskt_A;N;*;
2.11 Finansiella anläggningstillgångar Lån till delägare eller närstående;7234;Numeriskt_A;N;*;
2.12 Finansiella anläggningstillgångar Fordringar hos övriga företag som det finns ett ägarintresse i och Andra långfristiga fordringar;7235;Numeriskt_A;N;*;
2.13 Varulager Råvaror och förnödenheter;7241;Numeriskt_A;N;*;
2.14 Varulager Varor under tillverkning;7242;Numeriskt_A;N;*;
2.15 Varulager Färdiga varor och handelsvaror;7243;Numeriskt_A;N;*;
2.16 Varulager Övriga lagertillgångar;7244;Numeriskt_A;N;*;
2.17 Varulager Pågående arbeten för annans räkning;7245;Numeriskt_A;N;*;
2.18 Varulager Förskott till leverantörer;7246;Numeriskt_A;N;*;
2.19 Kortfristiga fordringar Kundfordringar;7251;Numeriskt_A;N;*;
2.20 Kortfristiga fordringar Fordringar hos koncern-, intresse- och gemensamt styrda företag;7252;Numeriskt_A;N;*;
2.21 Kortfristiga fordringar Fordringar hos övriga företag som det finns ett ägarintresse i och Övriga fordringar;7261;Numeriskt_A;N;*;
2.22 Kortfristiga fordringar Upparbetad men ej fakturerad intäkt;7262;Numeriskt_A;N;*;
2.23 Kortfristiga fordringar Förutbetalda kostnader och upplupna intäkter;7263;Numeriskt_A;N;*;
2.24 Kortfristiga placeringar Andelar i koncernföretag;7270;Numeriskt_A;N;*;
2.25 Kortfristiga placeringar Övriga kortfristiga placeringar;7271;Numeriskt_A;N;*;
2.26 Kassa och bank Kassa, bank och redovisningsmedel;7281;Numeriskt_A;N;*;
2.27 Eget kapital Bundet eget kapital;7301;Numeriskt_A;N;*;
2.28 Eget kapital Fritt eget kapital;7302;Numeriskt_A;N;*;
2.29 Obeskattade reserver Periodiseringsfonder;7321;Numeriskt_A;N;*;
2.30 Obeskattade reserver Ackumulerade överavskrivningar;7322;Numeriskt_A;N;*;
2.31 Obeskattade reserver Övriga obeskattade reserver;7323;Numeriskt_A;N;*;
2.32 Avsättningar Avsättningar för pensioner och liknande förpliktelser enligt lag (1967:531)...;7331;Numeriskt_A;N;*;
2.33 Avsättningar Övriga avsättningar för pensioner och liknande förpliktelser;7332;Numeriskt_A;N;*;
2.34 Avsättningar Övriga avsättningar;7333;Numeriskt_A;N;*;
2.35 Långfristiga skulder Obligationslån;7350;Numeriskt_A;N;*;
2.36 Långfristiga skulder Checkräkningskredit;7351;Numeriskt_A;N;*;
2.37 Långfristiga skulder Övriga skulder till kreditinstitut;7352;Numeriskt_A;N;*;
2.38 Långfristiga skulder Skulder till koncern-, intresse- och gemensamt styrda företag;7353;Numeriskt_A;N;*;
2.39 Långfristiga skulder Skulder till övriga företag som det finns ett ägarintresse i och Övriga skulder;7354;Numeriskt_A;N;*;
2.40 Kortfristiga skulder Checkräkningskredit;7360;Numeriskt_A;N;*;
2.41 Kortfristiga skulder Övriga skulder till kreditinstitut;7361;Numeriskt_A;N;*;
2.42 Kortfristiga skulder Förskott från kunder;7362;Numeriskt_A;N;*;
2.43 Kortfristiga skulder Pågående arbeten för annans räkning;7363;Numeriskt_A;N;*;
2.44 Kortfristiga skulder Fakturerad men ej upparbetad intäkt;7364;Numeriskt_A;N;*;
2.45 Kortfristiga skulder Leverantörsskulder;7365;Numeriskt_A;N;*;
2.46 Kortfristiga skulder Växelskulder;7366;Numeriskt_A;N;*;
2.47 Kortfristiga skulder Skulder till koncern-, intresse- och gemensamt styrda företag;7367;Numeriskt_A;N;*;
2.48 Kortfristiga skulder Skulder till övriga företag som det finns ett ägarintresse i och Övriga skulder;7369;Numeriskt_A;N;*;
2.49 Kortfristiga skulder Skattesskulder;7368;Numeriskt_A;N;*;
2.50 Kortfristiga skulder Upplupna kostnader och förutbetalda intäkter;7370;Numeriskt_A;N;*;
3.1 Nettoomsättning;7410;Numeriskt_A;N;+;
3.2 Förändring av lager av produkter i arbete, färdiga varor och pågående arbete för annans räkning. +;7411;Numeriskt_A;N;+;
3.2 Förändring av lager av produkter i arbete, färdiga varor och pågående arbete för annans räkning. -;7510;Numeriskt_A;N;-;
3.3 Aktiverat arbete för egen räkning;7412;Numeriskt_A;N;+;
3.4 Övriga rörelseintäkter;7413;Numeriskt_A;N;+;
3.5 Råvaror och förnödenheter;7511;Numeriskt_A;N;-;
3.6 Handelsvaror;7512;Numeriskt_A;N;-;
3.7 Övriga externa kostnader;7513;Numeriskt_A;N;-;
3.8 Personalkostnader;7514;Numeriskt_A;N;-;
3.9 Av- och nedskrivningar av materiella och immateriella anläggningstillgångar;7515;Numeriskt_A;N;-;
3.10 Nedskrivningar av omsättningstillgångar utöver normala nedskrivningar;7516;Numeriskt_A;N;-;
3.11 Övriga rörelsekostnader;7517;Numeriskt_A;N;-;
3.12 Resultat från andelar i koncernföretag +;7414;Numeriskt_A;N;+;
3.12 Resultat från andelar i koncernföretag -;7518;Numeriskt_A;N;-;
3.13 Resultat från andelar i intresseföretag och gemensamt styrda företag +;7415;Numeriskt_A;N;+;
3.13 Resultat från andelar i intresseföretag och gemensamt styrda företag -;7519;Numeriskt_A;N;-;
3.14 Resultat från övriga företag som det finns ett ägarintresse i +;7423;Numeriskt_A;N;+;
3.14 Resultat från övriga företag som det finns ett ägarintresse i -;7530;Numeriskt_A;N;-;
3.15 Resultat från övriga finansiella anläggningstillgångar +;7416;Numeriskt_A;N;+;
3.15 Resultat från övriga finansiella anläggningstillgångar -;7520;Numeriskt_A;N;-;
3.16 Övriga ränteintäkter och liknande resultatposter;7417;Numeriskt_A;N;+;
3.17 Nedskrivning av finansiella anläggningstillgångar och kortfristiga placeringar;7521;Numeriskt_A;N;-;
3.18 Räntekostnader och liknande resultatposter;7522;Numeriskt_A;N;-;
3.19 Lämnade koncernbidrag;7524;Numeriskt_A;N;-;
3.20 Mottagna koncernbidrag;7419;Numeriskt_A;N;+;
3.21 Återföring av periodiseringsfond;7420;Numeriskt_B;N;+;
3.22 Avsättning till periodiseringsfond;7525;Numeriskt_B;N;-;
3.23 Förändring av överavskrivningar +;7421;Numeriskt_A;N;+;
3.23 Förändring av överavskrivningar -;7526;Numeriskt_A;N;-;
3.24 Övriga bokslutsdispositioner +;7422;Numeriskt_A;N;+;
3.24 Övriga bokslutsdispositioner -;7527;Numeriskt_A;N;-;
3.25 Skatt på årets resultat;7528;Numeriskt_A;N;-;
3.26 Årets resultat, vinst (flyttas till p. 4.1);7450;Numeriskt_B;N;+;Får ej förekomma om 7550 finns med och 7550 <> 0.
3.27 Årets resultat, förlust (flyttas till p. 4.2);7550;Numeriskt_B;N;-;Får ej förekomma om 7450 finns med och 7450 <> 0.)";


    } // INK2


	} // namespace SRU
} // namespace SKV

namespace BAS { // BAS

	namespace SRU { // BAS::SRU

		//  From file resources/INK2_19_P1-intervall-vers-2.csv
		// 	URL: https://www.bas.se/kontoplaner/sru/
		// 	These are mapping between SKV SRU-codes and BAS accounts
		// 	for INK2 Income TAX Return.
		// 	The csv-files are exported from macOS Numbers from correspodning xls-file.

		namespace INK2 { // BAS::SRU::INK2

      // From INK2_P1_intervall-241119.csv
      char const* INK2_P1_intervall_241119{R"(;;Inkomstdeklaration 2;;;;;;
;;  ;;;;;;
;;;;;;;;
;;;Ändring i kopplingstabellen jämfört med föregående år.;❙;;;;
;;;;;;;;
Fält-kod;Rad Ink 2;Benämning;Konton i BAS 2023;;;;;
;;BALANSRÄKNING;;;;;;
;;Tillgångar;;;;;;
;;;;;;;;
;;Anläggningstillgångar;;;;;;
;;;;;;;;
;;Immateriella anläggningstillgångar;;;;;;
7201;2.1;Koncessioner, patent, licenser, varumärken, hyresrätter, goodwill och liknande rättigheter;1000-1087, 1089-1099;;;;;
7202;2.2;Förskott avseende immateriella anläggningstillgångar;1088;;;;;
;;;;;;;;
;;Materiella anläggningstillgångar;;;;;;
7214;2.3;Byggnader och mark;1100-1119, 1130-1179, 1190-1199;;;;;
7215;2.4 ;Maskiner, inventarier och övriga materiella anläggningstillgångar;1200-1279, 1290-1299;;;;;
7216;2.5;Förbättringsutgifter på annans fastighet;112x;;;;;
7217;2.6;Pågående nyanläggningar och förskott avseende materiella anläggningstillgångar;118x, 128x;;;;;
;;;;;;;;
;;Finansiella anläggningstillgångar;;;;;;
7230;2.7;Andelar i koncernföretag;131x;;;;;
7231;2.8;Andelar i intresseföretag och gemensamt styrda företag;1330-1335, 1338-1339;;;;;
7233;2.9;Ägarintresse i övriga företag och andra långfristiga värdepappersinnehav;135x, 1336, 1337;;;;;
7232;2.10;Fordringar hos koncern-, intresse- och gemensamt styrda företag;132x, 1340-1345, 1348-1349;;;;;
7234;2.11;Lån till delägare eller närstående;136x;;;;;
7235;2.12;Fordringar hos övriga företag som det finns ett ett ägarintresse i och Andra långfristiga fordringar;137x, 138x, 1346, 1347;;;;;
;;;;;;;;
;;Omsättningstillgångar;;;;;;
;;;;;;;;
;;Varulager;;;;;;
7241;2.13;Råvaror och förnödenheter;141x, 142x;;;;;
7242;2.14;Varor under tillverkning;144x;;;;;
7243;2.15;Färdiga varor och handelsvaror;145x, 146x;;;;;
7244;2.16;Övriga lagertillgångar;149x;;;;;
7245;2.17;Pågående arbeten för annans räkning;147x;;;;;
7246;2.18;Förskott till leverantörer;148x;;;;;
;;;;;;;;
;;Kortfristiga fordringar;;;;;;
7251;2.19;Kundfordringar;151x–155x, 158x;;;;;
7252;2.20;Fordringar hos koncern-, intresse- och gemensamt styrda företag;156x, 1570-1572, 1574-1579, 166x, 1671-1672, 1674-1679;;;;;
7261;2.21;Fordringar hos övriga företag som det finns ett ägarintresse i och Övriga fordringar;161x, 163x-165x, 168x-169x, 1573, 1673;;;;;
7262;2.22;Upparbetad men ej fakturerad intäkt;162x;;;;;
7263;2.23;Förutbetalda kostnader och upplupna intäkter;17xx;;;;;
;;;;;;;;
;;Kortfristiga placeringar;;;;;;
7270;2.24;Andelar i koncernföretag;186x;;;;;
7271;2.25;Övriga kortfristiga placeringar;1800-1859, 1870-1899;;;;;
;;;;;;;;
;;Kassa och bank;;;;;;
7281;2.26;Kassa, bank och redovisningsmedel;19xx;;;;;
;;;;;;;;
;;Eget kapital och skulder;;;;;;
;;Eget kapital;;;;;;
;;;;;;;;
7301;2.27;Bundet eget kapital;208x;;;;;
7302;2.28;Fritt eget kapital;209x;;;;;
;;;;;;;;
;;Obeskattade reserver och avsättningar;;;;;;
;;;;;;;;
;;Obeskattade reserver;;;;;;
7321;2.29;Periodiseringsfonder;211x-213x;;;;;
7322;2.30;Ackumulerade överavskrivningar;215x;;;;;
7323;2.31;Övriga obeskattade reserver;216x-219x;;;;;
;;;;;;;;
;;Avsättningar;;;;;;
7331;2.32;Avsättningar för pensioner och liknande förpliktelser enligt lagen (1967:531) om tryggande av pensionsutfästelserr m.m.;221x;;;;;
7332;2.33;Övriga avsättningar för pensioner och liknande förpliktelser;223x;;;;;
7333;2.34;Övriga avsättningar;2220-2229, 2240-2299;;;;;
;;;;;;;;
;;Skulder;;;;;;
;;;;;;;;
;;Långfristiga skulder (förfaller senare än 12 månader från balansdagen);;;;;;
7350;2.35;Obligationslån;231x-232x;;;;;
7351;2.36;Checkräkningskredit;233x;;;;;
7352;2.37;Övriga skulder till kreditinstitut;234x-235x;;;;;
7353;2.38;Skulder till koncern-, intresse- och gemensamt styrda företag;2360- 2372, 2374-2379;;;;;
7354;2.39;Skulder till övriga företag som det finns ett ägarintresse i och övriga skulder;238x-239x, 2373;;;;;
;;;;;;;;
;;Kortfristiga skulder (förfaller inom 12 månader från balansdagen);;;;;;
7360;2.40;Checkräkningskredit;248x;;;;;
7361;2.41;Övriga skulder till kreditinstitut;241x;;;;;
7362;2.42;Förskott från kunder;242x;;;;;
7363;2.43;Pågående arbeten för annans räkning;243x;;;;;
7364;2.44;Fakturerad men ej upparbetad intäkt;245x;;;;;
7365;2.45;Leverantörsskulder;244x;;;;;
7366;2.46;Växelskulder;2492;;;;;
7367;2.47;Skulder till koncern-, intresse- och gemensamt styrda företag;2460-2472, 2474-2479, 2874-2879;;;;;
7369;2.48;Skulder till övriga företag som det finns ett ägarintresse i och Övriga skulder;2490-2491, 2493-2499, 2600-2859, 2880-2899;;;;;
7368;2.49;Skatteskulder;25xx;;;;;
7370;2.50;Upplupna kostnader och förutbetalda intäkter;29xx;;;;;
;;;;;;;;
;;;;;;;;
;;RESULTATRÄKNING;;;;;;
;;;;;;;;
7410;3.1;Nettoomsättning;30xx-37xx;;;;;
7411;3.2;Förändring av lager av produkter i arbete, färdiga varor och pågående arbete för annans räkning;4900-4909, 4930-4959, 4970-4979, 4990-4999 (Om netto +);;;;;
7510;;;4900-4909, 4930-4959, 4970-4979, 4990-4999 (Om netto -);;;;;
7412;3.3;Aktiverat arbete för egen räkning;38xx;;;;;
7413;3.4;Övriga rörelseintäkter;39xx;;;;;
7511;3.5;Råvaror och förnödenheter;40xx-47xx, 4910-4920;;;;;
7512;3.6;Handelsvaror;40xx-47xx, 496x, 498x;;;;;
7513;3.7;Övriga externa kostnader;50xx-69xx;;;;;
7514;3.8;Personalkostnader;70xx-76xx;;;;;
7515;3.9;Av- och nedskrivningar av materiella och immateriella anläggningstillgångar;7700-7739, 7750-7789, 7800-7899;;;;;
7516;3.10;Nedskrivningar av omsättningstillgångar utöver normala nedskrivningar;774x, 779x;;;;;
7517;3.11;Övriga rörelsekostnader;79xx;;;;;
7414;3.12;Resultat från andelar i koncernföretag; 8000-8069, 8090-8099 (Om netto +);;;;;
7518;;;8000-8069,8090-8099 (Om netto -);;;;;
7415;3.13;Resultat från andelar i intresseföretag och gemensamt styrda företag;8100-8112, 8114-8117, 8119-8122, 8124-8132, 8134-8169, 8190-8199 (Om netto +);;;;;
7519;;;8100-8112, 8114-8117, 8119-8122, 8124-8132, 8134-8169, 8190-8199 (Om netto -);;;;;
7423;3.14;Resultat från övriga företag som det finns ett ägarintresse i;8113, 8118, 8123, 8133 (Om netto +);;;;;
7530;;; 8113, 8118, 8123, 8133 (Om netto -);;;;;
7416;3.15;Resultat från övriga anläggningstillgångar; 8200-8269, 8290-8299 (Om netto +);;;;;
7520;;;8200-8269, 8290-8299 (Om netto -);;;;;
7417;3.16;Övriga ränteintäkter och liknande resultatposter;8300-8369, 8390-8399;;;;;
7521;3.17;Nedskrivningar av finansiella anläggningstillgångar och kortfristiga placeringar;807x, 808x, 817x, 818x, 827x, 828x, 837x, 838x;;;;;
7522;3.18;Räntekostnader och liknande resultatposter;84xx;;;;;
7524;3.19;Lämnade koncernbidrag;883x;;;;;
7419;3.20;Mottagna koncernbidrag;882x;;;;;
7420;3.21;Återföring av periodiseringsfond;8810 (Om netto +), 8819;;;;;
7525;3.22;Avsättning till periodiseringsfond;8810 (Om netto -), 8811;;;;;
7421;3.23;Förändring av överavskrivningar;885x (Om netto +);;;;;
7526;;;885x (Om netto -);;;;;
7422;3.24;Övriga bokslutsdispositioner;886x-889x (Om netto +);;;;;
7527;;;886x-889x, 884x (Om netto -);;;;;
7528;3.25;Skatt på årets resultat;8900-8989;;;;;
7450;3.26;Årets resultat, vinst (flyttas till p. 4.1)  (+);+ 899x;;;;;
7550;3.27;Årets resultat, förlust (flyttas till p. 4.2) (-);– 899x;;;;;)"};


		// From resources/INK2_19_P1-intervall-vers-2.csv
			char const* INK2_19_P1_intervall_vers_2_csv{R"(Blad1: Table 1
2018;;Inkomstdeklaration 2;2019 P1;;;;;
;;  ;;;;;;
;;;;;;;;
;;För deklarationer som lämnas fr.o.m. period 1 2018;Ändring i kopplingstabellen jämfört med föregående år.;;;;;
;;;;;;;;
Fält-kod;Rad Ink 2;Benämning;Konton i BAS 2017;;;;;
;;BALANSRÄKNING;;;;;;
;;Tillgångar;;;;;;
;;;;;;;;
;;Anläggningstillgångar;;;;;;
;;;;;;;;
;;Immateriella anläggningstillgångar;;;;;;
7201;2.1;Koncessioner, patent, licenser, varumärken, hyresrätter, goodwill och liknande rättigheter;1000-1087, 1089-1099;;;;;
7202;2.2;Förskott avseende immateriella anläggningstillgångar;1088;;;;;
;;;;;;;;
;;Materiella anläggningstillgångar;;;;;;
7214;2.3;Byggnader och mark;1100-1119, 1130-1179, 1190-1199;;;;;
7215;2.4 ;Maskiner, inventarier och övriga materiella anläggningstillgångar;1200-1279, 1290-1299;;;;;
7216;2.5;Förbättringsutgifter på annans fastighet;112x;;;;;
7217;2.6;Pågående nyanläggningar och förskott avseende materiella anläggningstillgångar;118x, 128x;;;;;
;;;;;;;;
;;Finansiella anläggningstillgångar;;;;;;
7230;2.7;Andelar i koncernföretag;131x;;;;;
7231;2.8;Andelar i intresseföretag och gemensamt styrda företag;1330-1335, 1338-1339;;;;;
7233;2.9;Ägarintresse i övriga företag och andra långfristiga värdepappersinnehav;135x, 1336, 1337;;;;;
7232;2.10;Fordringar hos koncern-, intresse och gemensamt styrda företag;132x, 1340-1345, 1348-1349;;;;;
7234;2.11;Lån till delägare eller närstående;136x;;;;;
7235;2.12;Fordringar hos övriga företag som det finns ett ett ägarintresse i och Andra långfristiga fordringar;137x, 138x, 1346, 1347;;;;;
;;;;;;;;
;;Omsättningstillgångar;;;;;;
;;;;;;;;
;;Varulager;;;;;;
7241;2.13;Råvaror och förnödenheter;141x, 142x;;;;;
7242;2.14;Varor under tillverkning;144x;;;;;
7243;2.15;Färdiga varor och handelsvaror;145x, 146x;;;;;
7244;2.16;Övriga lagertillgångar;149x;;;;;
7245;2.17;Pågående arbeten för annans räkning;147x;;;;;
7246;2.18;Förskott till leverantörer;148x;;;;;
;;;;;;;;
;;Kortfristiga fordringar;;;;;;
7251;2.19;Kundfordringar;151x–155x, 158x;;;;;
7252;2.20;Fordringar hos koncern-, intresse- och gemensamt styrda företag;156x, 1570-1572, 1574-1579, 166x, 1671-1672, 1674-1679;;;;;
7261;2.21;Fordringar hos övriga företag som det finns ett ägarintresse i och Övriga fordringar;161x, 163x-165x, 168x-169x, 1573, 1673;;;;;
7262;2.22;Upparbetad men ej fakturerad intäkt;162x;;;;;
7263;2.23;Förutbetalda kostnader och upplupna intäkter;17xx;;;;;
;;;;;;;;
;;Kortfristiga placeringar;;;;;;
7270;2.24;Andelar i koncernföretag;186x;;;;;
7271;2.25;Övriga kortfristiga placeringar;1800-1859, 1870-1899;;;;;
;;;;;;;;
;;Kassa och bank;;;;;;
7281;2.26;Kassa, bank och redovisningsmedel;19xx;;;;;
;;;;;;;;
;;Eget kapital och skulder;;;;;;
;;Eget kapital;;;;;;
;;;;;;;;
7301;2.27;Bundet eget kapital;208x;;;;;
7302;2.28;Fritt eget kapital;209x;;;;;
;;;;;;;;
;;Obeskattade reserver och avsättningar;;;;;;
;;;;;;;;
;;Obeskattade reserver;;;;;;
7321;2.29;Periodiseringsfonder;211x-213x;;;;;
7322;2.30;Ackumulerade överavskrivningar;215x;;;;;
7323;2.31;Övriga obeskattade reserver;216x-219x;;;;;
;;;;;;;;
;;Avsättningar;;;;;;
7331;2.32;Avsättningar för pensioner och liknande förpliktelser enligt lagen (1967:531) om tryggande av pensionsutfästelserr m.m.;221x;;;;;
7332;2.33;Övriga avsättningar för pensioner och liknande förpliktelser;223x;;;;;
7333;2.34;Övriga avsättningar;2220-2229, 2240-2299;;;;;
;;;;;;;;
;;Skulder;;;;;;
;;;;;;;;
;;Långfristiga skulder (förfaller senare än 12 månader från balansdagen);;;;;;
7350;2.35;Obligationslån;231x-232x;;;;;
7351;2.36;Checkräkningskredit;233x;;;;;
7352;2.37;Övriga skulder till kreditinstitut;234x-235x;;;;;
7353;2.38;Skulder till koncern-, intresse och gemensamt styrda företag;2360- 2372, 2374-2379;;;;;
7354;2.39;Skulder till övriga företag som det finns ett ägarintresse i och övriga skulder;238x-239x, 2373;;;;;
;;;;;;;;
;;Kortfristiga skulder (förfaller inom 12 månader från balansdagen);;;;;;
7360;2.40;Checkräkningskredit;248x;;;;;
7361;2.41;Övriga skulder till kreditinstitut;241x;;;;;
7362;2.42;Förskott från kunder;242x;;;;;
7363;2.43;Pågående arbeten för annans räkning;243x;;;;;
7364;2.44;Fakturerad men ej upparbetad intäkt;245x;;;;;
7365;2.45;Leverantörsskulder;244x;;;;;
7366;2.46;Växelskulder;2492;;;;;
7367;2.47;Skulder till koncern-, intresse och gemensamt styrda företag;2460-2472, 2474-2872, 2874-2879;;;;;
7369;2.48;Skulder till övriga företag som det finns ett ägarintresse i och Övriga skulder;2490-2491, 2493-2499, 2600-2859, 2880-2899;;;;;
7368;2.49;Skatteskulder;25xx;;;;;
7370;2.50;Upplupna kostnader och förutbetalda intäkter;29xx;;;;;
;;;;;;;;
;;;;;;;;
;;RESULTATRÄKNING;;;;;;
;;;;;;;;
7410;3.1;Nettoomsättning;30xx-37xx;;;;;
7411;3.2;Förändring av lager av produkter i arbete, färdiga varor och pågående arbete för annans räkning;`+ 4900-4909, 4930-4959, 4970-4979, 4990-4999;;;;;
7510;;;– 4900-4909, 4930-4959, 4970-4979, 4990-4999;;;;;
7412;3.3;Aktiverat arbete för egen räkning;38xx;;;;;
7413;3.4;Övriga rörelseintäkter;39xx;;;;;
7511;3.5;Råvaror och förnödenheter;40xx-47xx, 4910-4920;;;;;
7512;3.6;Handelsvaror;40xx-47xx, 496x, 498x;;;;;
7513;3.7;Övriga externa kostnader;50xx-69xx;;;;;
7514;3.8;Personalkostnader;70xx-76xx;;;;;
7515;3.9;Av- och nedskrivningar av materiella och immateriella anläggningstillgångar;7700-7739, 7750-7789, 7800-7899;;;;;
7516;3.10;Nedskrivningar av omsättningstillgångar utöver normala nedskrivningar;774x, 779x;;;;;
7517;3.11;Övriga rörelsekostnader;79xx;;;;;
7414;3.12;Resultat från andelar i koncernföretag; +  8000-8069, 8090-8099;;;;;
7518;;;– 8000-8069,8090-8099;;;;;
7415;3.13;Resultat från andelar i intresseföretag och gemensamt styrda företag; + 8100-8112, 8114-8117, 8119-8122, 8124-8132,8134-8169, 8190-8199;;;;;
7519;;;– 8100-8112, 8114-8117, 8119-8122, 8124-8132, 8134-8169, 8190-8199;;;;;
7423;3.14;Resultat från övriga företag som det finns ett ägarintresse i; + 8113, 8118, 8123, 8133;;;;;
7530;;; - 8113, 8118, 8123, 8133;;;;;
7416;3.15;Resultat från övriga anläggningstillgångar; + 8200-8269, 8290-8299;;;;;
7520;;;– 8200-8269, 8290-8299;;;;;
7417;3.16;Övriga ränteintäkter och liknande resultatposter;8300-8369, 8390-8399;;;;;
7521;3.17;Nedskrivningar av finansiella anläggningstillgångar och kortfristiga placeringar;807x, 808x, 817x, 818x, 827x, 828x, 837x, 838x;;;;;
7522;3.18;Räntekostnader och liknande resultatposter;84xx;;;;;
7524;3.19;Lämnade koncernbidrag;883x;;;;;
7419;3.20;Mottagna koncernbidrag;882x;;;;;
7420;3.21;Återföring av periodiseringsfond;8810, 8819;;;;;
7525;3.22;Avsättning till periodiseringsfond;8810, 8811;;;;;
7421;3.23;Förändring av överavskrivningar;+ 885x;;;;;
7526;;;– 885x;;;;;
7422;3.24;Övriga bokslutsdispositioner;+ 886x-889x;;;;;
7527;;;– 886x-889x, 884x;;;;;
7528;3.25;Skatt på årets resultat;8900-8989;;;;;
7450;3.26;Årets resultat, vinst (flyttas till p. 4.1)  (+);+ 899x;;;;;
7550;3.27;Årets resultat, förlust (flyttas till p. 4.2) (-);– 899x;;;;;

Blad2: Table 1
;;;;


Blad3: Table 1
;;;;
)"}; // char const* INK2_19_P1_intervall_vers_2_csv

			void parse(char const* INK2_19_P1_intervall_vers_2_csv) {
				std::cout << "\nTODO: Implement BAS::SRU::INK2::parse";
				std::istringstream in{INK2_19_P1_intervall_vers_2_csv};
				std::string row{};		
				while (std::getline(in,row)) {
					Key::Sequence tokens{row,';'};
					std::cout << "\n------------------";
					int index{};
					for (auto const& token : tokens) {
						std::cout << "\n\t" << index++ << ":" << std::quoted(token);
					}
				}
			}


		} // BAS::SRU::INK2

	} // BAS::SRU

	namespace K2 { // BAS::K2

		namespace AR { // BAS::K2::AR
			// A namespace for Swedish Bolagsverket "Årsredovisning" according to K2 rules

      namespace ar_online {

        // From https://www.arsredovisning-online.se/bas_kontoplan as of 221118
        // From https://www.arsredovisning-online.se/bas-kontoplan/ as of 240714
        // This text defines mapping between fields on the Swedish TAX Return form and ranges of BAS Accounts 
        char const* bas_2024_mapping_to_k2_ar_text{R"(Resultaträkning

Konto 3000-3799

Fält: Nettoomsättning
Beskrivning: Intäkter som genererats av företagets ordinarie verksamhet, t.ex. varuförsäljning och tjänsteintäkter.
Konto 3800-3899

Fält: Aktiverat arbete för egen räkning
Beskrivning: Kostnader för eget arbete där resultatet av arbetet tas upp som en tillgång i balansräkningen.
Konto 3900-3999

Fält: Övriga rörelseintäkter
Beskrivning: Intäkter genererade utanför företagets ordinarie verksamhet, t.ex. valutakursvinster eller realisationsvinster.
Konto 4000-4799 eller 4910-4931

Fält: Råvaror och förnödenheter
Beskrivning: Årets inköp av råvaror och förnödenheter +/- förändringar av lagerposten ”Råvaror och förnödenheter”. Även kostnader för legoarbeten och underentreprenader.
Konto 4900-4999 (förutom 4910-4931, 4960-4969 och 4980-4989)

Fält: Förändring av lager av produkter i arbete, färdiga varor och pågående arbete för annans räkning
Beskrivning: Årets förändring av värdet på produkter i arbete och färdiga egentillverkade varor samt förändring av värde på uppdrag som utförs till fast pris.
Konto 4960-4969 eller 4980-4989

Fält: Handelsvaror
Beskrivning: Årets inköp av handelsvaror +/- förändring av lagerposten ”Handelsvaror”.
Konto 5000-6999

Fält: Övriga externa kostnader
Beskrivning: Normala kostnader som inte passar någon annanstans. T.ex. lokalhyra, konsultarvoden, telefon, porto, reklam och nedskrivning av kortfristiga fordringar.
Konto 7000-7699

Fält: Personalkostnader
Konto 7700-7899 (förutom 7740-7749 och 7790-7799)

Fält: Av- och nedskrivningar av materiella och immateriella anläggningstillgångar
Konto 7740-7749 eller 7790-7799

Fält: Nedskrivningar av omsättningstillgångar utöver normala nedskrivningar
Beskrivning: Används mycket sällan. Ett exempel är om man gör ovanligt stora nedskrivningar av kundfordringar.
Konto 7900-7999

Fält: Övriga rörelsekostnader
Beskrivning: Kostnader som ligger utanför företagets normala verksamhet. T.ex. valutakursförluster och realisationsförlust vid försäljning av icke- finansiella anläggningstillgångar.
Konto 8000-8099 (förutom 8070-8089)

Fält: Resultat från andelar i koncernföretag
Beskrivning: Nettot av företagets finansiella intäkter och kostnader från koncernföretag med undantag av räntor, koncernbidrag och nedskrivningar. T.ex. erhållna utdelningar, andel i handelsbolags resultat och realisationsresultat.
Konto 8070-8089, 8170-8189, 8270-8289 eller 8370-8389

Fält: Nedskrivningar av finansiella anläggningstillgångar och kortfristiga placeringar
Beskrivning: Nedskrivningar av och återföring av nedskrivningar på finansiella anläggningstillgångar och kortfristiga placeringar
Konto 8100-8199 (förutom 8113, 8118, 8123, 8133 och 8170-8189)

Fält: Resultat från andelar i intresseföretag och gemensamt styrda företag
Beskrivning: Nettot av företagets finansiella intäkter och kostnader från intresseföretag och gemensamt styrda företag med undantag av räntor och nedskrivningar. T.ex. erhållna utdelningar, andel i handelsbolags resultat och realisationsresultat.
Konto 8113, 8118, 8123 eller 8133

Fält: Resultat från övriga företag som det finns ett ägarintresse i
Beskrivning: Nettot av företagets finansiella intäkter och kostnader från övriga företag som det finns ett ägarintresse i med undantag av räntor och nedskrivningar. T.ex. vissa erhållna vinstutdelningar, andel i handelsbolags resultat och realisationsresultat.
Konto 8200-8299 (förutom 8270-8289)

Fält: Resultat från övriga finansiella anläggningstillgångar
Beskrivning: Nettot av intäkter och kostnader från företagets övriga värdepapper och fordringar som är anläggningstillgångar, med undantag av nedskrivningar. T.ex. ränteintäkter (även på värdepapper avseende koncern- och intresseföretag), utdelningar, positiva och negativa valutakursdifferenser och realisationsresultat.
Konto 8300-8399 (förutom 8370-8389)

Fält: Övriga ränteintäkter och liknande resultatposter
Beskrivning: Resultat från finansiella omsättningstillgångar med undantag för nedskrivningar. T.ex. ränteintäkter (även dröjsmålsräntor på kundfordringar), utdelningar och positiva valutakursdifferenser.
Konto 8400-8499

Fält: Räntekostnader och liknande resultatposter
Beskrivning: Resultat från finansiella skulder, t.ex. räntor på lån, positiva och negativa valutakursdifferenser samt dröjsmåls-räntor på leverantörsskulder.
Konto 8710

Fält: Extraordinära intäkter
Beskrivning: Används mycket sällan. Får inte användas för räkenskapsår som börjar 2016-01-01 eller senare.
Konto 8750

Fält: Extraordinära kostnader
Beskrivning: Används mycket sällan. Får inte användas för räkenskapsår som börjar 2016-01-01 eller senare.
Konto 8810-8819

Fält: Förändring av periodiseringsfonder
Konto 8820-8829

Fält: Erhållna koncernbidrag
Konto 8830-8839

Fält: Lämnade koncernbidrag
Konto 8840-8849 eller 8860-8899

Fält: Övriga bokslutsdispositioner
Konto 8850-8859

Fält: Förändring av överavskrivningar
Konto 8900-8979

Fält: Skatt på årets resultat
Beskrivning: Skatt som belastar årets resultat. Här ingår beräknad skatt på årets resultat, men även t.ex. justeringar avseende tidigare års skatt.
Konto 8980-8989

Fält: Övriga skatter
Beskrivning: Används sällan.
­

Balansräkning

Konto 1020-1059 eller 1080-1089 (förutom 1088)

Fält: Koncessioner, patent, licenser, varumärken samt liknande rättigheter
Konto 1060-1069

Fält: Hyresrätter och liknande rättigheter
Konto 1070-1079

Fält: Goodwill
Konto 1088

Fält: Förskott avseende immateriella anläggningstillgångar
Beskrivning: Förskott i samband med förvärv, t.ex. handpenning och deposition.
Konto 1100-1199 (förutom 1120-1129 och 1180-1189)

Fält: Byggnader och mark
Beskrivning: Fögnadens allmänna användning.
Konto 1120-1129

Fält: Förbättringsutgifter på annans fastighet
Konto 1180-1189 eller 1280-1289

Fält: Pågående nyanläggningar och förskott avseende materiella anläggningstillgångar
Konto 1210-1219

Fält: Maskiner och andra tekniska anläggningar
Beskrivning: Maskiner och tekniska anläggningar avsedda för produktionen.
Konto 1220-1279 (förutom 1260)

Fält: Inventarier, verktyg och installationer
Beskrivning: Om du fyller i detta fält måste du även fylla i motsvarande not i avsnittet "Noter".
Konto 1290-1299

Fält: Övriga materiella anläggningstillgångar
Beskrivning: T.ex. djur som klassificerats som anläggningstillgång.
Konto 1310-1319

Fält: Andelar i koncernföretag
Beskrivning: Aktier och andelar i koncernföretag.
Konto 1320-1329

Fält: Fordringar hos koncernföretag
Beskrivning: Fordringar på koncernföretag som förfaller till betalning senare än 12 månader från balansdagen.
Konto 1330-1339 (förutom 1336-1337)

Fält: Andelar i intresseföretag och gemensamt styrda företag
Beskrivning: Aktier och andelar i intresseföretag.
Konto 1336-1337

Fält: Ägarintressen i övriga företag
Beskrivning: Aktier och andelar i övriga företag som det redovisningsskyldiga företaget har ett ägarintresse i.
Konto 1340-1349 (förutom 1346-1347)

Fält: Fordringar hos intresseföretag och gemensamt styrda företag
Beskrivning: Fordringar på intresseföretag och gemensamt styrda företag, som förfaller till betalning senare än 12 månader från balansdagen.
Konto 1346-1347

Fält: Fordringar hos övriga företag som det finns ett ägarintresse i
Beskrivning: Fordringar på övriga företag som det finns ett ägarintresse i och som ska betalas senare än 12 månader från balansdagen.
Konto 1350-1359

Fält: Andra långfristiga värdepappersinnehav
Beskrivning: Långsiktigt innehav av värdepapper som inte avser koncern- eller intresseföretag.
Konto 1360-1369

Fält: Lån till delägare eller närstående
Beskrivning: Fordringar på delägare, och andra som står delägare nära, som förfaller till betalning senare än 12 månader från balansdagen.
Konto 1380-1389

Fält: Andra långfristiga fordringar
Beskrivning: Fordringar som förfaller till betalning senare än 12 månader från balansdagen.
Konto 1410-1429, 1430, 1431 eller 1438

Fält: Råvaror och förnödenheter
Beskrivning: Lager av råvaror eller förnödenheter som har köpts för att bearbetas eller för att vara komponenter i den egna tillverkningen.
Konto 1432-1449 (förutom 1438)

Fält: Varor under tillverkning
Beskrivning: Lager av varor där tillverkning har påbörjats.
Konto 1450-1469

Fält: Färdiga varor och handelsvaror
Beskrivning: Lager av färdiga egentillverkade varor eller varor som har köpts för vidareförsäljning (handelsvaror).
Konto 1470-1479

Fält: Pågående arbete för annans räkning
Beskrivning: Om du fyller i detta fält måste du även fylla i motsvarande not i avsnittet "Noter".
Konto 1480-1489

Fält: Förskott till leverantörer
Beskrivning: Betalningar och obetalda fakturor för varor och tjänster som redovisas som lager men där prestationen ännu inte erhållits.
Konto 1490-1499

Fält: Övriga lagertillgångar
Beskrivning: Lager av värdepapper (t.ex. lageraktier), lagerfastigheter och djur som klassificerats som omsättningstillgång.
Konto 1500-1559 eller 1580-1589

Fält: Kundfordringar
Konto 1560-1569 eller 1660-1669

Fält: Fordringar hos koncernföretag
Beskrivning: Fordringar på koncernföretag, inklusive kundfordringar.
Konto 1570-1579 (förutom 1573) eller 1670-1679 (förutom 1673)

Fält: Fordringar hos intresseföretag och gemensamt styrda företag
Beskrivning: Fordringar på intresseföretag och gemensamt styrda företag, inklusive kundfordringar.
Konto 1573 eller 1673

Fält: Fordringar hos övriga företag som det finns ett ägarintresse i
Beskrivning: Fordringar på övriga företag som det finns ett ägarintresse i, inklusive kundfordringar.
Konto 1590-1619, 1630-1659 eller 1680-1689

Fält: Övriga fordringar
Beskrivning: T.ex. aktuella skattefordringar.
Konto 1620-1629

Fält: Upparbetad men ej fakturerad intäkt
Beskrivning: Upparbetade men ej fakturerade intäkter från uppdrag på löpande räkning eller till fast pris enligt huvudregeln.
Konto 1690-1699

Fält: Tecknat men ej inbetalat kapital
Beskrivning: Fordringar på aktieägare före tecknat men ej inbetalt kapital. Används vid nyemission.
Konto 1700-1799

Fält: Förutbetalda kostnader och upplupna intäkter
Beskrivning: Förutbetalda kostnader (t.ex. förutbetalda hyror eller försäkringspremier) och upplupna intäkter (varor eller tjänster som är levererade men där kunden ännu inte betalat).
Konto 1800-1899 (förutom 1860-1869)

Fält: Övriga kortfristiga placeringar
Beskrivning: Innehav av värdepapper eller andra placeringar som inte är anläggningstillgångar och som inte redovisas i någon annan post under Omsättningstillgångar och som ni planerar att avyttra inom 12 månader från bokföringsårets slut.
Konto 1860-1869

Fält: Andelar i koncernföretag
Beskrivning: Här registrerar ni de andelar i koncernföretag som ni planerar att avyttra inom 12 månader från bokföringsårets slut.
Konto 1900-1989

Fält: Kassa och bank
Konto 1990-1999

Fält: Redovisningsmedel
Konto 2081, 2083 eller 2084

Fält: Aktiekapital
Beskrivning: Aktiekapitalet i ett aktiebolag ska vara minst 25 000 kr.
Konto 2082

Fält: Ej registrerat aktiekapital
Beskrivning: Beslutad ökning av aktiekapitalet genom fond- eller nyemission.
Konto 2085

Fält: Uppskrivningsfond
Konto 2086

Fält: Reservfond
Konto 2087

Fält: Bunden överkursfond
Konto 2090, 2091, 2093-2095 eller 2098

Fält: Balanserat resultat
Beskrivning: Summan av tidigare års vinster och förluster. Registrera balanserat resultat med minustecken om det balanserade resultatet är en balanserad förlust. Är det en balanserad vinst ska du inte använda minustecken.
Konto 2097

Fält: Fri överkursfond
Konto 2110-2149

Fält: Periodiseringsfonder
Beskrivning: Man kan avsätta upp till 25% av resultat efter finansiella poster till periodiseringsfonden. Det är ett sätt att skjuta upp bolagsskatten i upp till fem år. Avsättningen måste återföras till beskattning senast på det sjätte året efter det att avsättningen gjordes.
Konto 2150-2159

Fält: Ackumulerade överavskrivningar
Konto 2160-2199

Fält: Övriga obeskattade reserver
Konto 2210-2219

Fält: Avsättningar för pensioner och liknande förpliktelser enligt lagen (1967:531) om tryggande av pensionsutfästelse m.m.
Beskrivning: Åtaganden för pensioner enligt tryggandelagen.
Konto 2220-2229 eller 2250-2299

Fält: Övriga avsättningar
Beskrivning: Andra avsättningar än för pensioner, t.ex. garantiåtaganden.
Konto 2230-2239

Fält: Övriga avsättningar för pensioner och liknande förpliktelser
Beskrivning: Övriga pensionsåtaganden till nuvarande och tidigare anställda.
Konto 2310-2329

Fält: Obligationslån
Konto 2330-2339

Fält: Checkräkningskredit
Konto 2340-2359

Fält: Övriga skulder till kreditinstitut
Konto 2360-2369

Fält: Skulder till koncernföretag
Konto 2370-2379 (förutom 2373)

Fält: Skulder till intresseföretag och gemensamt styrda företag
Konto 2373

Fält: Skulder till övriga företag som det finns ett ägarintresse i
Konto 2390-2399

Fält: Övriga skulder
Konto 2410-2419

Fält: Övriga skulder till kreditinstitut
Konto 2420-2429

Fält: Förskott från kunder
Konto 2430-2439

Fält: Pågående arbete för annans räkning
Beskrivning: Om du fyller i detta fält måste du även fylla i motsvarande not i avsnittet "Noter".
Konto 2440-2449

Fält: Leverantörsskulder
Konto 2450-2459

Fält: Fakturerad men ej upparbetad intäkt
Konto 2460-2469 eller 2860-2869

Fält: Skulder till koncernföretag
Konto 2470-2479 (förutom 2473) eller 2870-2879 (förutom 2873)

Fält: Skulder till intresseföretag och gemensamt styrda företag
Konto 2473 eller 2873

Fält: Skulder till övriga företag som det finns ett ägarintresse i
Konto 2480-2489

Fält: Checkräkningskredit
Konto 2490-2499 (förutom 2492), 2600-2799, 2810-2859 eller 2880-2899

Fält: Övriga skulder
Konto 2492

Fält: Växelskulder
Konto 2500-2599

Fält: Skatteskulder
Konto 2900-2999

Fält: Upplupna kostnader och förutbetalda intäkter)"}; // bas_2024_mapping_to_k2_ar_text
      } // namespace ar_online
		} // namespace BAS::K2::AR
	} // namespace BAS::K2

    // BAS::bas_2022_account_plan_csv now in MASFramework


}
