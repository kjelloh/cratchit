#include <iostream>
#include <locale>
#include <string>
#include <sstream>
#include <queue>
#include <variant>
#include <vector>
#include <optional>
#include <string_view>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <random>
#include <cstdlib>
#include <cctype>
#include <map>
#include <regex>
#include <chrono>
#include <numeric>
#include <functional>

// Scratch comments to "remember" what configuration for VSCode that does "work"

// tasks.json/"tasks"/label:"macOS..."/args: ... "--sysroot=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk" 
//                                               to have third party g++-11 find includes for, and link with, mac OS libraries.
//                                               Gcc will automatically search sub-directories /usr/include and /usr/lib.

//                                               macOS 13 seems to have broken previous path /usr/include and usr/local/include (https://developer.apple.com/forums/thread/655588?answerId=665804022#665804022)
/* "...This is fine if you’re using Xcode, or Apple’s command-line tools, because they know how to look inside an SDK for headers and stub libraries. 
    If you’re using third-party tools then you need to consult the support resources for those tools 
		to find out how they’ve adjusted to this new reality ... the critical point is the linker. 
		If your third-party tools use the Apple linker then you should just be able to point the tools at the 
		usr/include directory within the appropriate SDK." */

// c_cpp_properties.json/"configurations":/"includePath": ... "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include"
//                                               to have Intellisense find c++ library headers.

// c_cpp_properties.json/"configurations":/"includePath": ... "/Library/Developer/CommandLineTools/usr/include/c++/v1"
//                                               to have Intellisense find actual OS specific c++ library headers (required by macro "include_next" in c++ library headers).

    // "tasks": [
    // {
    //     "type": "cppbuild",
    //     "label": "macOS C/C++: g++-11 build active file",
    //     "command": "/usr/local/bin/g++-11",
    //     "args": [
    //         "--sysroot=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk",
    //         "-fdiagnostics-color=always",
    //         "-std=c++20",
    //         "-g",
    //         "${file}",
    //         "-o",
    //         "${workspaceFolder}/cratchit.out"
    //     ],
    //     "options": {
    //         "cwd": "${fileDirname}"
    //     },
    //     "problemMatcher": [
    //         "$gcc"
    //     ],
    //     "group": {
    //         "kind": "build",
    //         "isDefault": true
    //     },
    //     "detail": "compiler: /usr/local/bin/g++-11"
    // }
    // ]

    // "configurations": [
    //     {
    //         "name": "Mac",
    //         "includePath": [
    //             "${workspaceFolder}/**",
    //             "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include",
    //             "/Library/Developer/CommandLineTools/usr/include/c++/v1"
    //         ],
    //         "defines": [],
    //         "macFrameworkPath": [
    //             "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/System/Library/Frameworks"
    //         ],
    //         "compilerPath": "/usr/local/bin/gcc-11",
    //         "cStandard": "gnu17",
    //         "cppStandard": "gnu++17",
    //         "intelliSenseMode": "macos-gcc-x64"
    //     }
    // ],

namespace SKV {
	namespace XML {
		bool to_employer_contributions_and_PAYE_tax_return_file(std::ostream& os) {
			os << "\n" << R"(<?xml version="1.0" encoding="UTF-8" standalone="no"?>)";
			os << "\n" << R"(<Skatteverket omrade="Arbetsgivardeklaration")";
				os << "\n" << R"(xmlns="http://xmls.skatteverket.se/se/skatteverket/da/instans/schema/1.1")";
				os << "\n" << R"(xmlns:agd="http://xmls.skatteverket.se/se/skatteverket/da/komponent/schema/1.1")";
				os << "\n" << R"(xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://xmls.skatteverket.se/se/skatteverket/da/instans/schema/1.1 http://xmls.skatteverket.se/se/skatteverket/da/arbetsgivardeklaration/arbetsgivardeklaration_1.1.xsd">)";
				os << "\n" << R"(<agd:Avsandare>)";
					os << "\n" << R"(<agd:Programnamn>Programmakarna AB</agd:Programnamn>)";
					os << "\n" << R"(<agd:Organisationsnummer>190002039006</agd:Organisationsnummer>)";
					os << "\n" << R"(<agd:TekniskKontaktperson>)";
						os << "\n" << R"(<agd:Namn>Valle Vadman</agd:Namn>)";
						os << "\n" << R"(<agd:Telefon>23-2-4-244454</agd:Telefon>)";
						os << "\n" << R"(<agd:Epostadress>valle.vadman@programmakarna.se</agd:Epostadress>)";
						os << "\n" << R"(<agd:Utdelningsadress1>Artillerigatan 11</agd:Utdelningsadress1>)";
						os << "\n" << R"(<agd:Utdelningsadress2>C/O Segemyhr</agd:Utdelningsadress2>)";
						os << "\n" << R"(<agd:Postnummer>62145</agd:Postnummer>)";
						os << "\n" << R"(<agd:Postort>Visby</agd:Postort>)";
					os << "\n" << R"(</agd:TekniskKontaktperson>)";
					os << "\n" << R"(<agd:Skapad>2021-01-30T07:42:25</agd:Skapad>)";
				os << "\n" << R"(</agd:Avsandare>)";
				os << "\n" << R"(<agd:Blankettgemensamt>)";
					os << "\n" << R"(<agd:Arbetsgivare>)";
						os << "\n" << R"(<agd:AgRegistreradId>165560269986</agd:AgRegistreradId>)";
						os << "\n" << R"(<agd:Kontaktperson>)";
							os << "\n" << R"(<agd:Namn>Ville Vessla</agd:Namn>)";
							os << "\n" << R"(<agd:Telefon>555-244454</agd:Telefon>)";
							os << "\n" << R"(<agd:Epostadress>ville.vessla@foretaget.se</agd:Epostadress>)";
							os << "\n" << R"(<agd:Sakomrade>skruv-avdelningens anställda</agd:Sakomrade>)";
						os << "\n" << R"(</agd:Kontaktperson>)";
						os << "\n" << R"(<agd:Kontaktperson>)";
							os << "\n" << R"(<agd:Namn>Maria Olsson</agd:Namn>)";
							os << "\n" << R"(<agd:Telefon>555-244121</agd:Telefon>)";
							os << "\n" << R"(<agd:Epostadress>maria.olsson@foretaget.se</agd:Epostadress>)";
							os << "\n" << R"(<agd:Sakomrade>mutter-avdelningens anställda</agd:Sakomrade>)";
						os << "\n" << R"(</agd:Kontaktperson>)";
					os << "\n" << R"(</agd:Arbetsgivare>)";
				os << "\n" << R"(</agd:Blankettgemensamt>)";

				os << "\n" << R"(<!-- Uppgift 1 HU -->)";
				os << "\n" << R"(<agd:Blankett>)";
					os << "\n" << R"(<agd:Arendeinformation>)";
						os << "\n" << R"(<agd:Arendeagare>165560269986</agd:Arendeagare>)";
						os << "\n" << R"(<agd:Period>202101</agd:Period>)";
					os << "\n" << R"(</agd:Arendeinformation>)";
					os << "\n" << R"(<agd:Blankettinnehall>)";
						os << "\n" << R"(<agd:HU>)";
							os << "\n" << R"(<agd:ArbetsgivareHUGROUP>)";
								os << "\n" << R"(<agd:AgRegistreradId faltkod="201">165560269986</agd:AgRegistreradId>)";
							os << "\n" << R"(</agd:ArbetsgivareHUGROUP>)";
							os << "\n" << R"(<agd:RedovisningsPeriod faltkod="006">202101</agd:RedovisningsPeriod>)";
							os << "\n" << R"(<agd:SummaArbAvgSlf faltkod="487">0</agd:SummaArbAvgSlf>)";
							os << "\n" << R"(<agd:SummaSkatteavdr faltkod="497">0</agd:SummaSkatteavdr>)";
						os << "\n" << R"(</agd:HU>)";
					os << "\n" << R"(</agd:Blankettinnehall>)";
				os << "\n" << R"(</agd:Blankett>)";

				os << "\n" << R"(<!-- Uppgift 1 IU -->)";
				os << "\n" << R"(<agd:Blankett>)";
					os << "\n" << R"(<agd:Arendeinformation>)";
						os << "\n" << R"(<agd:Arendeagare>165560269986</agd:Arendeagare>)";
						os << "\n" << R"(<agd:Period>202101</agd:Period>)";
					os << "\n" << R"(</agd:Arendeinformation>)";
					os << "\n" << R"(<agd:Blankettinnehall>)";
						os << "\n" << R"(<agd:IU>)";
							os << "\n" << R"(<agd:ArbetsgivareIUGROUP>)";
								os << "\n" << R"(<agd:AgRegistreradId faltkod="201">165560269986</agd:AgRegistreradId>)";
							os << "\n" << R"(</agd:ArbetsgivareIUGROUP>)";
							os << "\n" << R"(<agd:BetalningsmottagareIUGROUP>)";
								os << "\n" << R"(<agd:BetalningsmottagareIDChoice>)";
									os << "\n" << R"(<agd:BetalningsmottagarId faltkod="215">198202252386</agd:BetalningsmottagarId>)";
								os << "\n" << R"(</agd:BetalningsmottagareIDChoice>)";
							os << "\n" << R"(</agd:BetalningsmottagareIUGROUP>)";
							os << "\n" << R"(<agd:RedovisningsPeriod faltkod="006">202101</agd:RedovisningsPeriod>)";
							os << "\n" << R"(<agd:Specifikationsnummer faltkod="570">001</agd:Specifikationsnummer>)";
							os << "\n" << R"(<agd:AvdrPrelSkatt faltkod="001">0</agd:AvdrPrelSkatt>)";
						os << "\n" << R"(</agd:IU>)";
					os << "\n" << R"(</agd:Blankettinnehall>)";
				os << "\n" << R"(</agd:Blankett>)";
			os << "\n" << R"(</Skatteverket>)";

			return static_cast<bool>(os);
		}
	} // namespace XML
} // namespace SKV

namespace tokenize {
	// returns s split into first,second on provided delimiter delim.
	// split fail returns first = "" and second = s
	std::pair<std::string,std::string> split(std::string s,char delim) {
		auto pos = s.find(delim);
		if (pos<s.size()) return {s.substr(0,pos),s.substr(pos+1)};
		else return {"",s}; // split failed (return right = the unsplit string)
	}

	enum class eAllowEmptyTokens {
		unknown
		,no
		,yes
		,undefined
	};

	std::vector<std::string> splits(std::string s,char delim,eAllowEmptyTokens allow_empty_tokens = eAllowEmptyTokens::no) {
		std::vector<std::string> result;
		// TODO: Refactor code so default is allowing for split into empty tokens and make skip whitespace a special case
		if (allow_empty_tokens == eAllowEmptyTokens::no) {
			auto head_tail = split(s,delim);
			// std::cout << "\nhead" << head_tail.first << " tail:" << head_tail.second;
			while (head_tail.first.size()>0) {
				auto const& [head,tail] = head_tail;
				result.push_back(head);
				head_tail = split(tail,delim);
				// std::cout << "\nhead" << head_tail.first << " tail:" << head_tail.second;
			}
			if (head_tail.second.size()>0) {
				result.push_back(head_tail.second);
				// std::cout << "\ntail:" << head_tail.second;
			}
		}
		else {
			size_t first{},delim_pos{};
			do {
				delim_pos = s.find(delim,first);
				result.push_back(s.substr(first,delim_pos-first));
				first = delim_pos+1;
			} while (delim_pos<s.size());
		}
		return result;
	}

	std::vector<std::string> splits(std::string const& s) {
		std::vector<std::string> result;
		std::istringstream is{s};
		std::string token{};
		while (is >> std::quoted(token)) result.push_back(token);
		return result; 
	}

	enum class SplitOn {
		Undefined
		,TextAndAmount
		,TextAmountAndDate
		,Unknown
	};

	enum class TokenID {
		Undefined
		,Caption
		,Amount
		,Date
		,Unknown
	};

	TokenID token_id_of(std::string const& s) {
		const std::regex date_regex("[2-9]\\d{3}([0]\\d|[1][0-2])([0-2]\\d|[3][0-1])"); // YYYYmmdd
		if (std::regex_match(s,date_regex)) return TokenID::Date;
		const std::regex amount_regex("^-?\\d+([.,]\\d\\d?)?$"); // 123 123,1 123.1 123,12 123.12
		if (std::regex_match(s,amount_regex)) return TokenID::Amount;
		const std::regex caption_regex("[ -~]+");
		if (std::regex_match(s,caption_regex)) return TokenID::Caption;
		return TokenID::Unknown; 
	}

	std::vector<std::string> splits(std::string const& s,SplitOn split_on) {
		// std::cout << "\nsplits(std::string const& s,SplitOn split_on)";
		std::vector<std::string> result{};
		auto spaced_tokens = splits(s);
		std::vector<TokenID> ids{};
		for (auto const& s : spaced_tokens) {
			ids.push_back(token_id_of(s));
		}
		for (int i=0;i<spaced_tokens.size();++i) {
			// std::cout << "\n" << spaced_tokens[i] << " id:" << static_cast<int>(ids[i]);
		}
		switch (split_on) {
			case SplitOn::TextAmountAndDate: {
				std::vector<TokenID> expected_id{TokenID::Caption,TokenID::Amount,TokenID::Date};
				int state{0};
				std::string s{};
				for (int i=0;i<ids.size();) {
					if (ids[i] == expected_id[state]) {
						if (s.size() > 0) s += " ";
						s += spaced_tokens[i++];
					}
					else {
						if (s.size()>0) result.push_back(s);
						++state;	
						s.clear();					
					}
				}
				if (s.size()>0) result.push_back(s);
			} break;
			default: {
				std::cerr << "\nERROR - Unknown split_on value " << static_cast<int>(split_on);
			} break;
		}
		return result;
	}
} // namespace tokenize

namespace parse {
    using In = std::string_view;

    template <typename P>
    using ParseResult = std::optional<std::pair<P,In>>;

    template <typename P>
    class Parse {
    public:
        virtual ParseResult<P> operator()(In const& in) const = 0;
    };

    template <typename T>
    auto parse(T const& p,In const& in) {
        return p(in);
    }
} // namespace parse

/*

	How can we model bookkeeping of a company?

	It seems we need to be able to model the following aspects.

	One or more Journals with details of each financial transaction, what document is proof of the transaction and how the transaction is accounted for.
	The Ledger in which all Journal transactions are summarised to.

	So, it seems a Journal contains records of how each transaction is accounted for.
	And the Ledger contains transactions as recorded to accounts?

	Can we thus say that an SIE-file is in fact a representation of a Journal?
	And when we read in an SIE file we create the Ledger?

	The Swedish terms can thus be as follows.

	"Bokföring" and "Verifikation" is about entering trasnactions into a Journal?
	"Huvudbok" is the Ledger?

	So then we should implement a model with one or more Journals linked to a single Ledger?

	Journal 1											Ledger
	Journal 2												Account
	Journal 3												Account
	...														Account
	   Journal Entry {										Account
		   Transaction Account <-> Account					   Transaction
		   Transaction Account <-> Account
		   ...
	   }

	From Wikipedia

	*  A document is produced each time a transaction occurs
	*  Bookkeeping first involves recording the details of all of these source documents into multi-column journals.
	*  Each column in a journal normally corresponds to an account.
	*  After a certain period ... each column in each journal .. are then transferred to their respective accounts in the ledger...

*/

// Helper for stream output of otpional string
std::ostream& operator<<(std::ostream& os,std::optional<std::string> const& opt_s) {
	if (opt_s) {
		os << std::quoted(*opt_s);
	}
	return os;
}

std::optional<std::filesystem::path> path_to_existing_file(std::string const& s) {
	std::optional<std::filesystem::path> result{};
	std::filesystem::path path{s};
	if (std::filesystem::exists(path)) result = path;
	return result;
}

using Amount= float;
using optional_amount = std::optional<Amount>;
optional_amount to_amount(std::string const& sAmount) {
	// std::cout << "\nto_amount " << std::quoted(sAmount);
	optional_amount result{};
	Amount amount{};
	std::istringstream is{sAmount};
	if (auto pos = sAmount.find(','); pos != std::string::npos) {
		// Handle 123,45 ==> 123.45
		result = to_amount(std::accumulate(sAmount.begin(),sAmount.end(),std::string{},[](auto acc,char ch){
			acc += (ch==',')?'.':ch;
			return acc;
		}));
	}
	else if (is >> amount) {
		result = amount;
	}
	else {
		// handle integer (no decimal point)
		try {
			auto int_amount = std::stoi(sAmount);
			result = static_cast<Amount>(int_amount);
		}
		catch (std::exception const& e) { /* failed - do nothing */}
	}
	// if (result) std::cout << "\nresult " << *result;
	return result;
}

using optional_year_month_day = std::optional<std::chrono::year_month_day>;

std::ostream& operator<<(std::ostream& os, std::chrono::year_month_day const& yyyymmdd) {
	// TODO: Remove when support for ostream << chrono::year_month_day (g++11 stdlib seems to lack support)
	os << std::setfill('0') << std::setw(4) << static_cast<int>(yyyymmdd.year());
	os << std::setfill('0') << std::setw(2) << static_cast<unsigned>(yyyymmdd.month());
	os << std::setfill('0') << std::setw(2) << static_cast<unsigned>(yyyymmdd.day());
	return os;
}
std::string to_string(std::chrono::year_month_day const& yyyymmdd) {
		std::ostringstream os{};
		os << yyyymmdd;
		return os.str();
}

optional_year_month_day to_date(std::string const& sYYYYMMDD) {
	// std::cout << "\nto_date(" << sYYYYMMDD << ")";
	optional_year_month_day result{};
	if (sYYYYMMDD.size()==8) {
		result = std::chrono::year_month_day(
			std::chrono::year{std::stoi(sYYYYMMDD.substr(0,4))}
			,std::chrono::month{static_cast<unsigned>(std::stoul(sYYYYMMDD.substr(4,2)))}
			,std::chrono::day{static_cast<unsigned>(std::stoul(sYYYYMMDD.substr(6,2)))});
	}
	else {
		// Handle "YYYY-MM-DD" "YYYY MM DD" etc.
		std::string sDate{};
		std::copy_if(sYYYYMMDD.begin(),sYYYYMMDD.end(),std::back_inserter(sDate),[](char ch){
			return std::isdigit(ch);
		});
		if (sDate.size()==8) result = to_date(sDate);
	}
	// if (result) std::cout << " = " << *result;
	// else std::cout << " = null";
	return result;
}

using BASAccountNo = unsigned int;
using OptionalBASAccountNo = std::optional<BASAccountNo>;

bool do_share_tokens(std::string const& s1,std::string const& s2) {
	bool result{false};
	auto had_heading_words = tokenize::splits(s1);
	auto je_heading_words = tokenize::splits(s2);
	for (std::string hadw : had_heading_words) {
		for (std::string jew : je_heading_words) {
			std::transform(hadw.begin(),hadw.end(),hadw.begin(),::toupper);
			std::transform(jew.begin(),jew.end(),jew.begin(),::toupper);
			// std::cout << "\ncompare " << std::quoted(hadw) << " with " << std::quoted(jew);
			if (hadw == jew) {
				result = true;
			}
		}
	}
	return result;
}

namespace SRU {
	using AccountNo = unsigned int;
	using OptionalAccountNo = std::optional<AccountNo>;
}

namespace BAS {

	enum class AccountType {
		// Account type is specified as T, S, K or I (asset, debt, cost or income)
		// Swedish: tillgång, skuld, kostnad eller intäkt
		Unknown
		,Asset
		,Debt
		,Cost
		,Income
		,Undefined
	};

	using OptionalAccountType = std::optional<AccountType>;

	struct AccountMeta {
		std::string name{};
		OptionalAccountType account_type{};
		SRU::OptionalAccountNo sru_code{};
	};
	using AccountMetas = std::map<BASAccountNo,BAS::AccountMeta>;

	struct AccountTransaction {
		BASAccountNo account_no;
		std::optional<std::string> transtext{};
		Amount amount;
	};
	using AccountTransactions = std::vector<AccountTransaction>;
	using OptionalAccountTransaction = std::optional<AccountTransaction>;

	namespace anonymous {
		struct JournalEntry {
			std::string caption{};
			std::chrono::year_month_day date{};
			AccountTransactions account_transactions;
		};
		using OptionalJournalEntry = std::optional<JournalEntry>;
		using JournalEntries = std::vector<JournalEntry>;
	} // namespace anonymous

	using VerNo = unsigned int;
	using OptionalVerNo = std::optional<VerNo>;
	using Series = char;

	struct JournalEntry {
		Series series;
		OptionalVerNo verno;
		anonymous::JournalEntry entry;
	};
	using OptionalJournalEntry = std::optional<JournalEntry>;
	using JournalEntries = std::vector<JournalEntry>;

	struct MetaEntry {
		bool is_unposted;
		BAS::Series series;
		BAS::VerNo verno;
		BAS::anonymous::JournalEntry aje;
	};

	using MatchesMetaEntry = std::function<bool(BAS::MetaEntry const& me)>;

	OptionalBASAccountNo to_account_no(std::string const& s) {
		OptionalBASAccountNo result{};
		if (s.size()==4) {
			if (std::all_of(s.begin(),s.end(),::isalnum)) {
				auto account_no = std::stoi(s);
				if (account_no >= 1000) result = account_no;
			}
		}
		return result;
	}

	struct JournalEntryMeta {
		Series series;
		VerNo verno;
	};

	using OptionalJournalEntryMeta = std::optional<JournalEntryMeta>;

	OptionalJournalEntryMeta to_journal_meta(std::string const& s) {
		OptionalJournalEntryMeta result{};
		const std::regex meta_regex("[A-Z]\\d+"); // series followed by verification number
		if (std::regex_match(s,meta_regex)) result = JournalEntryMeta{s[0],static_cast<VerNo>(std::stoi(s.substr(1)))};
		return result;
	}

	namespace filter {
		struct is_series {
			BAS::Series required_series;
			bool operator()(MetaEntry const& me) {
				return (me.series == required_series);
			}
		};

		struct is_unposted {
			bool operator()(MetaEntry const& me) {
				return me.is_unposted;
			}
		};

		struct contains_account {
			BASAccountNo account_no;
			bool operator()(MetaEntry const& me) {
				return std::any_of(me.aje.account_transactions.begin(),me.aje.account_transactions.end(),[this](auto const& at){
					return (this->account_no == at.account_no);
				});
			}
		};

		struct matches_meta {
			JournalEntryMeta entry_meta;
			bool operator()(MetaEntry const& me) {
				return (me.series == entry_meta.series and me.verno == entry_meta.verno);
			}
		};

		struct matches_amount {
			Amount amount;
			bool operator()(MetaEntry const& me) {
				return std::any_of(me.aje.account_transactions.begin(),me.aje.account_transactions.end(),[this](auto const& at){
					return (this->amount == at.amount);
				});
			}
		};

		struct matches_heading {
			std::string user_search_string;
			bool operator()(MetaEntry const& me) {
				bool result{false};
				result = do_share_tokens(user_search_string,me.aje.caption);
				if (!result) {
					result = std::any_of(me.aje.account_transactions.begin(),me.aje.account_transactions.end(),[this](auto const& at){
						if (at.transtext) return do_share_tokens(user_search_string,*at.transtext);
						return false;
					});
				}
				return result;
			}
		};


		struct matches_user_search_criteria{
			std::string user_search_string;
			bool operator()(MetaEntry const& me) {
				bool result{false};
				if (!result and user_search_string.size()==1) {
					result = is_series{user_search_string[0]}(me);
				}
				if (auto account_no = to_account_no(user_search_string);!result and account_no) {
					result = contains_account{*account_no}(me);
				}
				if (auto meta = to_journal_meta(user_search_string);!result and meta) {
					result = matches_meta{*meta}(me);
				}
				if (auto amount = to_amount(user_search_string);!result and amount) {
					result = matches_amount{*amount}(me);
				}
				if (!result) {
					result = matches_heading{user_search_string}(me);
				}
				return result;
			}
		};

	}
} // namespace BAS

using BASAccountNos = std::vector<BASAccountNo>;
using Sru2BasMap = std::map<SRU::AccountNo,BASAccountNos>;

Sru2BasMap sru_to_bas_map(BAS::AccountMetas const& metas) {
	Sru2BasMap result{};
	for (auto const& [bas_account_no,am] : metas) {
		if (am.sru_code) result[*am.sru_code].push_back(bas_account_no);
	}
	return result;
}

// Name, Heading + Amount account transaction
struct NameHeadingAmountAT {
	std::string account_name;
	std::optional<std::string> trans_text{};
	Amount amount;
};
using OptionalNameHeadingAmountAT = std::optional<NameHeadingAmountAT>;

OptionalNameHeadingAmountAT to_name_heading_amount(std::vector<std::string> const& ast) {
	OptionalNameHeadingAmountAT result{};
	switch (ast.size()) {
		case 2: {
			if (auto amount = to_amount(ast[1])) {
				result = NameHeadingAmountAT{.account_name=ast[0],.amount=*amount};
			}
		} break;
		case 3: {
			if (auto amount = to_amount(ast[2])) {
				result = NameHeadingAmountAT{.account_name=ast[0],.trans_text=ast[1],.amount=*amount};
			}		
		} break;
		default:;
	}
	return result;
}

BAS::OptionalAccountTransaction to_bas_account_transaction(std::vector<std::string> const& ast) {
	BAS::OptionalAccountTransaction result{};
	if (ast.size() > 1) {
			if (auto account_no = BAS::to_account_no(ast[0])) {
				switch (ast.size()) {
					case 2: {
						if (auto amount = to_amount(ast[1])) {
							result = BAS::AccountTransaction{.account_no=*account_no,.transtext=std::nullopt,.amount=*amount};
						}
					} break;
					case 3: {
						if (auto amount = to_amount(ast[2])) {
							result = BAS::AccountTransaction{.account_no=*account_no,.transtext=ast[1],.amount=*amount};
						}
					} break;
					default:;
				}
			}
	}
	return result;
}

std::ostream& operator<<(std::ostream& os,BAS::AccountTransaction const& at) {
	os << at.account_no;
	os << " " << at.transtext;
	os << " " << at.amount;
	return os;
};

std::string to_string(BAS::AccountTransaction const& at) {
	std::ostringstream os{};
	os << at;
	return os.str();
};

std::ostream& operator<<(std::ostream& os,BAS::anonymous::JournalEntry const& je) {
	os << std::quoted(je.caption) << " " << je.date;
	for (auto const& at : je.account_transactions) {
		os << "\n\t" << to_string(at); 
	}
	return os;
};

std::ostream& operator<<(std::ostream& os,BAS::OptionalVerNo const& verno) {
	if (verno and *verno!=0) os << *verno;
	else os << " _ ";
	return os;
}


std::ostream& operator<<(std::ostream& os,BAS::JournalEntry const& je) {
	os << je.series << je.verno << " " << je.entry;
	return os;
};

std::ostream& operator<<(std::ostream& os,BAS::MetaEntry const& me) {
	if (me.is_unposted) os << "*";
	else os << " ";
	os << me.series << me.verno;
	os << me.aje;
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::JournalEntries const& jes) {
	for (auto const& je : jes) {
		os << "\n" << je;
	}
	return os;
};

std::string to_string(BAS::anonymous::JournalEntry const& je) {
	std::ostringstream os{};
	os << je;
	return os.str();
};

std::string to_string(BAS::JournalEntry const& je) {
	std::ostringstream os{};
	os << je;
	return os.str();
};

using BASJournal = std::map<BAS::VerNo,BAS::anonymous::JournalEntry>;
using BASJournals = std::map<char,BASJournal>; // Swedish BAS Journals named "Series" and labeled A,B,C,...

struct HeadingAmountDateTransEntryMeta {
	OptionalBASAccountNo transaction_amount_account{};
};

struct HeadingAmountDateTransEntry {
	std::string heading{};
	Amount amount;
	std::chrono::year_month_day date{};
	BAS::OptionalJournalEntry current_candidate{};
};

std::ostream& operator<<(std::ostream& os,HeadingAmountDateTransEntry const& had) {
	os << had.heading;
	os << " " << had.amount;
	os << " " << to_string(had.date); 
	return os;
}

using OptionalHeadingAmountDateTransEntry = std::optional<HeadingAmountDateTransEntry>;

using HeadingAmountDateTransEntries = std::vector<HeadingAmountDateTransEntry>;

Amount entry_transaction_amount(BAS::anonymous::JournalEntry const& je) {
	Amount result = std::accumulate(je.account_transactions.begin(),je.account_transactions.end(),Amount{},[](Amount acc,BAS::AccountTransaction const& account_transaction){
		acc += (account_transaction.amount>0)?account_transaction.amount:0;
		return acc;
	});
	return result;
}

struct AccountTransactionTemplate {
	AccountTransactionTemplate(BASAccountNo account_no,Amount gross_amount,Amount account_amount) 
		:  m_account_no{account_no}
			,m_percent{static_cast<int>(std::round(account_amount*100 / gross_amount))}  {}
	BASAccountNo m_account_no;
	int m_percent;
	BAS::AccountTransaction operator()(Amount amount) const {
		// BAS::AccountTransaction result{.account_no = m_account_no,.transtext="",.amount=amount*m_factor};
		BAS::AccountTransaction result{
			 .account_no = m_account_no
			,.transtext=""
			,.amount=static_cast<Amount>(std::round(amount*m_percent)/100.0)};
		return result;
	}
};
using AccountTransactionTemplates = std::vector<AccountTransactionTemplate>;

class JournalEntryTemplate {
public:

	JournalEntryTemplate(BAS::Series series,BAS::JournalEntry const& bje) : m_series{series} {
		auto gross_amount = entry_transaction_amount(bje.entry);
		if (gross_amount >= 0.01) {
			std::transform(bje.entry.account_transactions.begin(),bje.entry.account_transactions.end(),std::back_inserter(templates),[gross_amount](BAS::AccountTransaction const& account_transaction){
				AccountTransactionTemplate result(
					 account_transaction.account_no
					,gross_amount
					,account_transaction.amount
				);
				return result;
			});
			std::sort(this->templates.begin(),this->templates.end(),[](auto const& e1,auto const& e2){
				return (std::abs(e1.m_percent) > std::abs(e2.m_percent)); // greater to lesser
			});
		}
	}

	BAS::Series series() const {return m_series;}

	BAS::AccountTransactions operator()(Amount amount) const {
		BAS::AccountTransactions result{};
		std::transform(templates.begin(),templates.end(),std::back_inserter(result),[amount](AccountTransactionTemplate const& att){
			return att(amount);
		});
		return result;
	}
	friend std::ostream& operator<<(std::ostream&, JournalEntryTemplate const&);
private:
	BAS::Series m_series;
	AccountTransactionTemplates templates{};
}; // class JournalEntryTemplate

// ==================================================================================
// Had -> journal_entry -> Template

using JournalEntryTemplateList = std::vector<JournalEntryTemplate>;
using OptionalJournalEntryTemplate = std::optional<JournalEntryTemplate>;

OptionalJournalEntryTemplate to_template(BAS::JournalEntry const& je) {
	OptionalJournalEntryTemplate result({je.series,je});
	return result;
}

BAS::JournalEntry to_journal_entry(HeadingAmountDateTransEntry const& had,JournalEntryTemplate const& jet) {
	BAS::JournalEntry result{};
	result.series = jet.series();
	result.entry.caption = had.heading;
	result.entry.date = had.date;
	result.entry.account_transactions = jet(std::abs(had.amount)); // Ignore sign to have template apply its sign
	return result;
}

std::ostream& operator<<(std::ostream& os,JournalEntryTemplate const& entry) {
	os << "template: series " << entry.series();
	std::for_each(entry.templates.begin(),entry.templates.end(),[&os](AccountTransactionTemplate const& t){
		os << "\n\t" << t.m_account_no << " " << t.m_percent;
	});
	return os;
}

bool had_matches_trans(HeadingAmountDateTransEntry const& had,BAS::anonymous::JournalEntry const& je) {
	return do_share_tokens(had.heading,je.caption);
}

BAS::JournalEntries template_candidates(BASJournals const& journals,auto const& matches) {
	// std::cout << "\ntemplate_candidates";
	BAS::JournalEntries result{};
	for (auto const& je : journals) {
		auto const& [series,journal] = je;
		for (auto const& [verno,entry] : journal) {								
			if (matches(entry)) result.push_back({series,verno,entry});
		}
	}
	std::sort(result.begin(),result.end(),[](auto const& je1,auto const& je2){
		return (je1.entry.date < je2.entry.date);
	});
	return result;
}

// ==================================================================================

bool same_whole_units(Amount const& a1,Amount const& a2) {
	return (std::round(a1) == std::round(a2));
}

BAS::JournalEntry updated_entry(BAS::JournalEntry const& je,BAS::AccountTransaction const& at) {
	BAS::JournalEntry result{je};
	std::sort(result.entry.account_transactions.begin(),result.entry.account_transactions.end(),[](auto const& e1,auto const& e2){
		return (std::abs(e1.amount) > std::abs(e2.amount)); // greater to lesser
	});
	auto iter = std::find_if(result.entry.account_transactions.begin(),result.entry.account_transactions.end(),[&at](auto const& entry){
		return (entry.account_no == at.account_no);
	});
	if (iter == result.entry.account_transactions.end()) {
		result.entry.account_transactions.push_back(at);
		return updated_entry(result,at); // recurse with added entry
	}
	else if (je.entry.account_transactions.size()==4) {
		// std::cout << "\n4 OK";
		// Assume 0: Transaction Amount, 1: Amount no VAT, 3: VAT, 4: rounding amount
		auto& trans_amount = result.entry.account_transactions[0].amount;
		auto& ex_vat_amount = result.entry.account_transactions[1].amount;
		auto& vat_amount = result.entry.account_transactions[2].amount;
		auto& round_amount = result.entry.account_transactions[3].amount;

		auto abs_trans_amount = std::abs(trans_amount);
		auto abs_ex_vat_amount = std::abs(ex_vat_amount);
		auto abs_vat_amount = std::abs(vat_amount);
		auto abs_round_amount = std::abs(round_amount);
		auto abs_at_amount = std::abs(at.amount);

		auto trans_amount_sign = static_cast<int>(trans_amount / std::abs(trans_amount));
		auto vat_sign = static_cast<int>(vat_amount/abs_vat_amount); // +-1
		auto at_sign = static_cast<int>(at.amount/abs_at_amount);

		auto vat_rate = static_cast<int>(std::round(abs_vat_amount*100/abs_ex_vat_amount));
		switch (vat_rate) {
			case 25:
			case 12:
			case 6: {
				// std::cout << "\nVAT OK";
				switch (std::distance(result.entry.account_transactions.begin(),iter)) {
					case 0: {
						// Assume update transaction amount
						std::cout << "\nUpdate Transaction Amount";
					} break;
					case 1: {
						// Assume update amount ex VAT
						// std::cout << "\n Update Amount Ex VAT";
						abs_ex_vat_amount = abs_at_amount;
						abs_vat_amount = std::round(vat_rate*abs_ex_vat_amount)/100;

						ex_vat_amount = vat_sign*abs_ex_vat_amount;
						vat_amount = vat_sign*abs_vat_amount;
						round_amount = -1*std::round(100*(trans_amount + ex_vat_amount + vat_amount))/100;
					} break;
					case 2: {
						// Assume update VAT amount
						std::cout << "\n Update VAT";
					} break;
					case 3: {
						// Assume update the rounding amount
						// std::cout << "\n Update Rounding";
						if (at.amount < 1.0) {
							// Assume a cent rounding amount
							// Automate the following algorithm.
							// 1. Assume the (Transaction amount + at.amount) is the "actual" adjusted (rounded) transaction amount
							// 4. Adjust Amount ex VAT and VAT to fit adjusted transaction amount
							round_amount = at.amount;
							// std::cout << "\ntrans_amount_sign " << trans_amount_sign;
							// std::cout << "\nvat_sign " << vat_sign; 
							auto adjusted_trans_amount = std::abs(trans_amount+round_amount);
							// std::cout << "\nadjusted_trans_amount " << trans_amount_sign*adjusted_trans_amount;
							// std::cout << "\nvat_rate " << vat_rate;
							auto reverse_vat_factor = vat_rate*1.0/(100+vat_rate); // E.g. 25/125 = 0.2
							// std::cout << "\nreverse_vat_factor " << reverse_vat_factor;
							vat_amount = vat_sign * std::round(reverse_vat_factor*100*adjusted_trans_amount)/100.0; // round to cents
							// std::cout << "\nvat_amount " << vat_amount;
							ex_vat_amount = vat_sign * std::round((1.0 - reverse_vat_factor)*100*adjusted_trans_amount)/100.0; // round to cents
							// std::cout << "\nex_vat_amount " << ex_vat_amount;
						}
					} break;
				}
			} break;
			default: {
				// Unknown VAT rate
				// Do no adjustment
				// Leave to user to deal with this update
			}
		}
	}
	else {
		// Todo: Future needs may require adjusting transaction amounts to still sum upp to the transaction amount?
	}
	return result;
}

class SIEEnvironment {
public:
	BASJournals& journals() {return m_journals;}
	BASJournals const& journals() const {return m_journals;}
	bool is_unposted(BAS::Series series, BAS::VerNo verno) const {
		bool result{true};
		if (verno_of_last_posted_to.contains(series)) result = verno > this->verno_of_last_posted_to.at(series);
		return result;
	}
	void post(BAS::JournalEntry const& je) {
		if (je.verno) {
			m_journals[je.series][*je.verno] = je.entry;
			verno_of_last_posted_to[je.series] = *je.verno;
		}
		else {
			std::cerr << "\nSIEEnvironment::post failed - cant post an entry with null verno";
		}
	}
	std::optional<BAS::JournalEntry> stage(BAS::JournalEntry const& entry) {
		std::optional<BAS::JournalEntry> result{};
		if (this->already_in_posted(entry) == false) result = this->add(entry);
		return result;
	}

	BAS::JournalEntries stage(SIEEnvironment const& staged_sie_environment) {
		BAS::JournalEntries result{};
		for (auto const& [series,journal] : staged_sie_environment.journals()) {
			for (auto const& [verno,aje] : journal) {
				auto je = this->stage({series,verno,aje});
				if (!je) {
					result.push_back({series,verno,aje}); // no longer staged
				}
			}
		}
		return result;
	}
	BAS::JournalEntries unposted() const {
		// std::cout << "\nunposted()";
		BAS::JournalEntries result{};
		for (auto const& [series,verno] : this->verno_of_last_posted_to) {
			// std::cout << "\n\tseries:" << series << " verno:" << verno;
			auto last = m_journals.at(series).find(verno);
			for (auto iter = ++last;iter!=m_journals.at(series).end();++iter) {
				// std::cout << "\n\tunposted:" << iter->first;
				BAS::JournalEntry bjer{
					.series = series
					,.verno = iter->first
					,.entry = iter->second
				};
				result.push_back(bjer);
			}
		}
		return result;
	}

	BAS::AccountMetas& account_metas() {return this->m_account_metas;}

private:
	BAS::AccountMetas m_account_metas{};
	BASJournals m_journals{};
	std::map<char,BAS::VerNo> verno_of_last_posted_to{};
	BAS::JournalEntry add(BAS::JournalEntry const& je) {
		BAS::JournalEntry result{je};
		// Assign "actual" sequence number
		auto verno = largest_verno(je.series) + 1;
		result.verno = verno;
		m_journals[je.series][verno] = je.entry;
		return result;
	}
	BAS::VerNo largest_verno(BAS::Series series) {
		auto const& journal = m_journals[series];
		return std::accumulate(journal.begin(),journal.end(),unsigned{},[](auto acc,auto const& entry){
			return (acc<entry.first)?entry.first:acc;
		});
	}
	bool already_in_posted(BAS::JournalEntry const& entry_to_stage) {
		bool result{false};
		if (entry_to_stage.verno and *entry_to_stage.verno > 0) {
			auto journal_iter = m_journals.find(entry_to_stage.series);
			if (journal_iter != m_journals.end()) {
				if (entry_to_stage.verno) {
					auto entry_iter = journal_iter->second.find(*entry_to_stage.verno);
					result = (entry_iter != journal_iter->second.end());
				}
			}
		}
		return result;
	}
};

using OptionalSIEEnvironment = std::optional<SIEEnvironment>;

using SIEEnvironments = std::map<std::string,SIEEnvironment>;

void for_each_journal_entry(SIEEnvironments const& sie_envs,auto& f) {
	for (auto const& [year_id,sie_env] : sie_envs) {
		for (auto const& [journal_id,journal] : sie_env.journals()) {
			for (auto const& [verno,je] : journal) {
				f(je);
			}
		}
	}
}

void for_each_meta_entry(SIEEnvironments const& sie_envs,auto& f) {
	for (auto const& [year_id,sie_env] : sie_envs) {
		for (auto const& [series,journal] : sie_env.journals()) {
			for (auto const& [verno,aje] : journal) {
				f(BAS::MetaEntry{.series=series,.verno=verno,.aje=aje});
			}
		}
	}
}

BAS::OptionalAccountTransaction gross_account_transaction(BAS::anonymous::JournalEntry const& je) {
	BAS::OptionalAccountTransaction result{};
	auto trans_amount = entry_transaction_amount(je);
	auto iter = std::find_if(je.account_transactions.begin(),je.account_transactions.end(),[&trans_amount](auto const& at){
		return std::abs(at.amount) == trans_amount;
	});
	if (iter != je.account_transactions.end()) result = *iter;
	return result;
}

BAS::OptionalAccountTransaction net_account_transaction(BAS::anonymous::JournalEntry const& je) {
	BAS::OptionalAccountTransaction result{};
	auto trans_amount = entry_transaction_amount(je);
	auto iter = std::find_if(je.account_transactions.begin(),je.account_transactions.end(),[&trans_amount](auto const& at){
		return std::abs(at.amount) == 0.8*trans_amount;
	});
	if (iter != je.account_transactions.end()) result = *iter;
	return result;
}

BAS::OptionalAccountTransaction vat_account_transaction(BAS::anonymous::JournalEntry const& je) {
	BAS::OptionalAccountTransaction result{};
	auto trans_amount = entry_transaction_amount(je);
	auto iter = std::find_if(je.account_transactions.begin(),je.account_transactions.end(),[&trans_amount](auto const& at){
		return std::abs(at.amount) == 0.2*trans_amount;
	});
	if (iter != je.account_transactions.end()) result = *iter;
	return result;
}

struct GrossAccountTransactions {
	BAS::AccountTransactions result;
	void operator()(BAS::anonymous::JournalEntry const& je) {
		if (auto at = gross_account_transaction(je)) {
			result.push_back(*at);
		}
	}
};

struct NetAccountTransactions {
	BAS::AccountTransactions result;
	void operator()(BAS::anonymous::JournalEntry const& je) {
		if (auto at = net_account_transaction(je)) {
			result.push_back(*at);
		}
	}
};

struct VatAccountTransactions {
	BAS::AccountTransactions result;
	void operator()(BAS::anonymous::JournalEntry const& je) {
		if (auto at = vat_account_transaction(je)) {
			result.push_back(*at);
		}
	}
};

BAS::AccountTransactions gross_account_transactions(SIEEnvironments const& sie_envs) {
	GrossAccountTransactions ats{};
	for_each_journal_entry(sie_envs,ats);
	return ats.result;
}

BAS::AccountTransactions net_account_transactions(SIEEnvironments const& sie_envs) {
	NetAccountTransactions ats{};
	for_each_journal_entry(sie_envs,ats);
	return ats.result;
}

BAS::AccountTransactions vat_account_transactions(SIEEnvironments const& sie_envs) {
	VatAccountTransactions ats{};
	for_each_journal_entry(sie_envs,ats);
	return ats.result;
}

struct T2 {
	BAS::MetaEntry me;
	struct CounterTrans {
		BASAccountNo linking_account;
		BAS::MetaEntry me;
	};
	std::optional<CounterTrans> counter_trans{};
};
using T2s = std::vector<T2>;

using T2Entry = std::pair<BAS::MetaEntry,T2::CounterTrans>;
using T2Entries = std::vector<T2Entry>;

std::ostream& operator<<(std::ostream& os,T2Entry const& t2e) {
	os << "    " << t2e.first.series << t2e.first.verno;
	os << " <- " << t2e.second.linking_account << " ->";
	os << " " << t2e.second.me.series << t2e.second.me.verno;
	os << "\n 1:" << t2e.first;
	os << "\n 2:" << t2e.second.me;
	return os;
}

std::ostream& operator<<(std::ostream& os,T2Entries const& t2es) {
	for (auto const& t2e : t2es) os << "\n\n" << t2e;
	return os;
}

T2Entries to_t2_entries(T2s const& t2s) {
	T2Entries result{};
	for (auto const& t2 : t2s) {
		if (t2.counter_trans) result.push_back({t2.me,*t2.counter_trans});
	}
	return result;
}

struct CollectT2s {
	T2s t2s{};
	T2Entries result() const { 
		return to_t2_entries(t2s);
	}
	void operator() (BAS::MetaEntry const& me) {
		auto t2_iter = t2s.begin();
		for (;t2_iter != t2s.end();++t2_iter) {
			if (!t2_iter->counter_trans) {
				// No counter trans found yet
				auto at_iter1 = std::find_if(me.aje.account_transactions.begin(),me.aje.account_transactions.end(),[&t2_iter](BAS::AccountTransaction const& at1){
					auto  at_iter2 = std::find_if(t2_iter->me.aje.account_transactions.begin(),t2_iter->me.aje.account_transactions.end(),[&at1](BAS::AccountTransaction const& at2){
						return (at1.account_no == at2.account_no) and (at1.amount == -at2.amount);
					});
					return (at_iter2 != t2_iter->me.aje.account_transactions.end());
				});
				if (at_iter1 != me.aje.account_transactions.end()) {
					// iter refers to an account transaction in me to the same account but a counter amount as in t2.je
					T2::CounterTrans counter_trans{.linking_account = at_iter1->account_no,.me = me};
					t2_iter->counter_trans = counter_trans;
					break;
				}
			}
		}
		if (t2_iter == t2s.end()) {
			t2s.push_back(T2{.me = me});
		}
	}
};

T2Entries t2_entries(SIEEnvironments const& sie_envs) {
	CollectT2s collect_t2s{};
	for_each_meta_entry(sie_envs,collect_t2s);
	return collect_t2s.result();
}

using EnvironmentValue = std::map<std::string,std::string>;
using Environment = std::multimap<std::string,EnvironmentValue>;

OptionalJournalEntryTemplate template_of(OptionalHeadingAmountDateTransEntry const& had,SIEEnvironment const& sie_environ) {
	OptionalJournalEntryTemplate result{};
	if (had) {
		BAS::JournalEntries candidates{};
		for (auto const& je : sie_environ.journals()) {
			auto const& [series,journal] = je;
			for (auto const& [verno,entry] : journal) {								
				if (entry.caption.find(had->heading) != std::string::npos) {
					candidates.push_back({series,verno,entry});
				}
			}
		}
		// select the entry with the latest date
		std::nth_element(candidates.begin(),candidates.begin(),candidates.end(),[](auto const& je1, auto const& je2){
			return (je1.entry.date > je2.entry.date);
		});
		result = to_template(candidates.front());
	}
	return result;
}

std::ostream& operator<<(std::ostream& os,EnvironmentValue const& ev) {
	bool not_first{false};
	std::for_each(ev.begin(),ev.end(),[&not_first,&os](auto const& entry){
		if (not_first) {
			os << ";"; // separator
		}
		os <<  entry.first;
		os <<  "=";
		os <<  entry.second;
		not_first = true;
	});
	return os;
}

// "belopp=1389,50;datum=20221023;rubrik=Klarna"
std::string to_string(EnvironmentValue const& ev) {
	std::ostringstream os{};
	os << ev;
	return os.str();
}

std::ostream& operator<<(std::ostream& os,Environment::value_type const& entry) {
	os << entry.first << " " << std::quoted(to_string(entry.second));
	return os;
}

std::ostream& operator<<(std::ostream& os,Environment const& env) {
	for (auto const& entry : env) {
		os << "\n\t" << entry;
	}	
	return os;
}

// "belopp=1389,50;datum=20221023;rubrik=Klarna"
EnvironmentValue to_environment_value(std::string const s) {
	EnvironmentValue result{};
	auto kvps = tokenize::splits(s,';');
	for (auto const& kvp : kvps) {
		auto const& [name,value] = tokenize::split(kvp,'=');
		result[name] = value;
	}
	return result;
}
std::string to_string(Environment::value_type const& entry) {
	std::ostringstream os{};
	os << entry;
	return os.str();
}

EnvironmentValue to_environment_value(HeadingAmountDateTransEntry const had) {
	// std::cout << "\nto_environment_value: had.amount" << had.amount << " had.date" << had.date;
	std::ostringstream os{};
	os << had.amount;
	EnvironmentValue ev{};
	ev["rubrik"] = had.heading;
	ev["belopp"] = os.str();
	ev["datum"] = to_string(had.date);
	return ev;
}

OptionalHeadingAmountDateTransEntry to_had(EnvironmentValue const& ev) {
	OptionalHeadingAmountDateTransEntry result{};
	HeadingAmountDateTransEntry had{};
	while (true) {
		if (auto iter = ev.find("rubrik");iter != ev.end()) had.heading = iter->second;
		else break;
		if (auto iter = ev.find("belopp");iter != ev.end()) had.amount = *to_amount(iter->second); // assume success
		else break;
		if (auto iter = ev.find("datum");iter != ev.end()) had.date = *to_date(iter->second); // assume success
		else break;
		result = had;
		break;
	}
	return result;
}

namespace CSV {
	namespace NORDEA {
		struct istream {
			std::istream& is;
			operator bool() {return static_cast<bool>(is);}
		};

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

		CSV::NORDEA::istream& operator>>(CSV::NORDEA::istream& in,HeadingAmountDateTransEntry& had) {
			//       0         1      2         3         4    5        6          7              8      9
			// Bokföringsdag;Belopp;Avsändare;Mottagare;Namn;Rubrik;Meddelande;Egna anteckningar;Saldo;Valuta
			//       0         1      2      3            4                             5                                 6                78 9
			// 2021-12-15;-419,65;51 86 87-9;;KORT             BEANSTALK APP   26;KORT             BEANSTALK APP   26;BEANSTALK APP   2656;;;SEK
			std::string sEntry{};
			if (std::getline(in.is,sEntry)) {
				auto tokens = tokenize::splits(sEntry,';',tokenize::eAllowEmptyTokens::yes);
				// LOG
				if (false) {
					std::cout << "\ncsv count: " << tokens.size(); // Expected 10
					for (int i=0;i<tokens.size();++i) {
						std::cout << "\n\t" << i << " " << tokens[i];
					}
				}
				if (tokens.size()==10) {
					while (true) {
						std::string heading{tokens[element::Rubrik]};
						had.heading = heading;
						if (auto amount = to_amount(tokens[element::Belopp])) had.amount = *amount;
						else {in.is.setstate(std::istream::failbit);break;}
						if (auto date = to_date(tokens[element::Bokforingsdag])) had.date = *date;
						else {in.is.setstate(std::istream::failbit);break;}
						break;
					}
				}
			}
			return in;
		}
	} // namespace NORDEA

	using CSVParseResult = std::optional<HeadingAmountDateTransEntry>;

	CSVParseResult parse_TRANS(auto& in) {
		CSVParseResult result{};
		auto pos = in.is.tellg();
		HeadingAmountDateTransEntry had{};
		if (in >> had) {
			// std::cout << "\nfrom csv: " << had;
			result = had;
			pos = in.is.tellg(); // accept new stream position
		}
		in.is.seekg(pos); // Reset position in case of failed parse
		in.is.clear(); // Reset failbit to allow try for other parse
		return result;		
	}

	HeadingAmountDateTransEntries from_stream(auto& in) {
		HeadingAmountDateTransEntries result{};
		parse_TRANS(in); // skip first line with field names
		while (auto had = parse_TRANS(in)) {
			result.push_back(*had);
		}
		return result;
	}
} // namespace CSV

enum class PromptState {
	Undefined
	,Root
	,HADIndex
	,JEIndex
	,AccountIndex
	,Amount
	,CounterAccountsEntry
	,Unknown
};

class ConcreteModel {
public:	
	std::string user_input{};
	PromptState prompt_state{PromptState::Root};
	size_t had_index{};
	BAS::JournalEntries template_candidates{};
	// BAS::JournalEntry current_candidate{};
	BAS::AccountTransactions at_candidates{};
	BAS::AccountTransaction at{};
  std::string prompt{};
	bool quit{};
	std::map<std::string,std::filesystem::path> sie_file_path{};
	SIEEnvironments sie{};
	HeadingAmountDateTransEntries heading_amount_date_entries{};
	std::filesystem::path staged_sie_file_path{"cratchit.se"};
	HeadingAmountDateTransEntries::iterator selected_had_iter() {
		auto had_iter = this->heading_amount_date_entries.begin();
		auto end = this->heading_amount_date_entries.end();
		std::advance(had_iter,this->had_index);
		return had_iter;
	}
	OptionalHeadingAmountDateTransEntry selected_had() {
		OptionalHeadingAmountDateTransEntry result{};
		auto had_iter = this->selected_had_iter();
		if (had_iter != heading_amount_date_entries.end()) {
			result = *had_iter;
		}
		return result;
	}

	BAS::OptionalAccountTransaction selected_had_at(int at_index) {
		BAS::OptionalAccountTransaction result{};
		if (auto had = this->selected_had()) {
			if (had->current_candidate) {
				auto iter = had->current_candidate->entry.account_transactions.begin();
				auto end = had->current_candidate->entry.account_transactions.end();
				std::advance(iter,at_index);
				if (iter != end) result = *iter;
			}
		}
		return result;
	}
	void erease_selected_had() {
		auto iter = this->selected_had_iter();
		if (iter != heading_amount_date_entries.end()) {
			this->heading_amount_date_entries.erase(iter);
			this->had_index = this->heading_amount_date_entries.size()-1; // default to select the now last one
		}							
	}

private:
};

using Model = std::unique_ptr<ConcreteModel>; // "as if" immutable (pass around the same instance)

struct Key {char value;};
using Command = std::string;
struct Quit {};
struct Nop {};

using Msg = std::variant<Nop,Key,Quit,Command>;
struct Cmd {std::optional<Msg> msg{};};
using Ux = std::vector<std::string>;

Cmd to_cmd(std::string const& user_input) {
	Cmd result{Nop{}};
	if (user_input == "quit" or user_input=="q") result.msg = Quit{};
	else if (user_input.size()>0) result.msg = Command{user_input};
	return result;
}

std::vector<std::string> help_for(PromptState const& prompt_state) {
	std::vector<std::string> result{};
	result.push_back("Available Entry Options>");
	switch (prompt_state) {
		case PromptState::Root: {
			result.push_back("<Heading> <Amount> <Date> : Entry of new Heading Amount Date (HAD) Transaction entry");
			result.push_back("-had : lists current Heading Amount Date (HAD) entries");
			result.push_back("-sie <sie file path> : imports a new input sie file");
			result.push_back("-sie : lists transactions in input sie-file");
			result.push_back("-env : lists cratchit environment");
			result.push_back("-csv <csv file path> : Imports Comma Seperated Value file of Web bank account transactions");
			result.push_back("                       Stores them as Heading Amount Date (HAD) entries.");			
			result.push_back("'q' or 'Quit'");
		} break;
		case PromptState::HADIndex:
		case PromptState::JEIndex:
		case PromptState::AccountIndex: {
			result.push_back("Please Enter the index of the entry to edit");
		} break;
		case PromptState::Amount: {
			result.push_back("Please Enter Amount");
		}
		default: {
		}
	}
	return result;
}

namespace SIE {

	extern std::map<char,char16_t> cp435ToUnicodeMap;

	char16_t cp435ToUnicode(char ch435) {
		return cp435ToUnicodeMap[ch435];
	}

	using UTF16String = std::wstring;

	UTF16String cp435ToUnicode(std::string s435) {
		UTF16String result{};
		std::transform(s435.begin(),s435.end(),std::back_inserter(result),[](char ch){
			return cp435ToUnicode(ch);
		});
		return result;
	}

	namespace UTF8 {
    // Unicode constants
    // Leading (high) surrogates: 0xd800 - 0xdbff
    // Trailing (low) surrogates: 0xdc00 - 0xdfff
    const char32_t LEAD_SURROGATE_MIN  = 0x0000d800;
    const char32_t LEAD_SURROGATE_MAX  = 0x0000dbff;
    const char32_t TRAIL_SURROGATE_MIN = 0x0000dc00;
    const char32_t TRAIL_SURROGATE_MAX = 0x0000dfff;

    // Maximum valid value for a Unicode code point
    const char32_t CODE_POINT_MAX      = 0x0010ffff;

		inline bool is_surrogate(char32_t cp) {
					return (cp >= LEAD_SURROGATE_MIN && cp <= TRAIL_SURROGATE_MAX);
		}

		bool is_valid_unicode(char32_t cp) {
			return (cp <= CODE_POINT_MAX && !is_surrogate(cp));
		}

		struct Stream {
			std::ostream& os;
		};

		UTF8::Stream& operator<<(UTF8::Stream& os,char32_t cp) {
			// Thanks to https://sourceforge.net/p/utfcpp/code/HEAD/tree/v3_0/src/utf8.h
			if (!is_valid_unicode(cp)) {
				os.os << '?';
			}
			else if (cp < 0x80)                        // one octet
					os.os << static_cast<char>(cp);
			else if (cp < 0x800) {                // two octets
					os.os << static_cast<char>((cp >> 6)            | 0xc0);
					os.os << static_cast<char>((cp & 0x3f)          | 0x80);
			}
			else if (cp < 0x10000) {              // three octets
					os.os << static_cast<char>((cp >> 12)           | 0xe0);
					os.os << static_cast<char>(((cp >> 6) & 0x3f)   | 0x80);
					os.os << static_cast<char>((cp & 0x3f)          | 0x80);
			}
			else {                                // four octets
					os.os << static_cast<char>((cp >> 18)           | 0xf0);
					os.os << static_cast<char>(((cp >> 12) & 0x3f)  | 0x80);
					os.os << static_cast<char>(((cp >> 6) & 0x3f)   | 0x80);
					os.os << static_cast<char>((cp & 0x3f)          | 0x80);
			}
			return os;
		}

		std::string unicode_to_utf8(UTF16String const& s) {
			std::ostringstream os{};
			UTF8::Stream utf8_os{os};
			for (auto cp : s) utf8_os << cp;
			return os.str();
		}
	}

	using Stream = std::istream;

	struct Tag {
		std::string const expected;
	};
	// #KONTO 8422 "Dr?jsm?lsr?ntor f?r leverant?rsskulder"
	struct Konto {
		std::string tag;
		BASAccountNo account_no;
		std::string name;
	};

	struct Sru {
		std::string tag;
		BASAccountNo bas_account_no;
		SRU::AccountNo sru_account_no;
	};

	struct Trans {
		// Spec: #TRANS account no {object list} amount transdate transtext quantity sign
		// Ex:   #TRANS 1920 {} 802 "" "" 0
		std::string tag;
		BASAccountNo account_no;
		std::string object_list{};
		float amount;
		std::optional<std::chrono::year_month_day> transdate{};
		std::optional<std::string> transtext{};
		std::optional<float> quantity{};
		std::optional<std::string> sign{};
	};
	struct Ver {
		// Spec: #VER series verno verdate vertext regdate sign
		// Ex:   #VER A 3 20210510 "Beanstalk" 20210817
		std::string tag;
		BAS::Series series;
		BAS::VerNo verno;
		std::chrono::year_month_day verdate;
		std::string vertext;
		std::optional<std::chrono::year_month_day> regdate{};
		std::optional<std::string> sign{};
		std::vector<Trans> transactions{};
	};
	struct AnonymousLine {};
	using SIEFileEntry = std::variant<Konto,Sru,Ver,Trans,AnonymousLine>;
	using SIEParseResult = std::optional<SIEFileEntry>;
	std::istream& operator>>(std::istream& in,Tag const& tag) {
		if (in.good()) {
			// std::cout << "\n>>Tag[expected:" << tag.expected;
			auto pos = in.tellg();
			std::string token{};
			if (in >> token) {
				// std::cout << ",read:" << token << "]";
				if (token != tag.expected) {
					in.seekg(pos); // Reset position for failed tag
					in.setstate(std::ios::failbit); // failed to read expected tag
					// std::cout << " rejected";
				}
			}
			else {
				// std::cout << "null";
			}
		}		
		return in;
	}

	struct optional_YYYYMMdd_parser {
		std::optional<std::chrono::year_month_day>& date;
	};
	struct YYYYMMdd_parser {
		std::chrono::year_month_day& date;
	};
	std::istream& operator>>(std::istream& in,optional_YYYYMMdd_parser& p) {
		if (in.good()) {
			auto pos = in.tellg();
			try {
				// std::chrono::from_stream(in,p.fmt,p.date); // Not in g++11 libc++?
				std::string sDate{};
				in >> std::quoted(sDate);
				if (sDate.size()==0) {
					pos = in.tellg(); // Accept this parse position (e.g., "" is to be accepted)
				}
				else if (auto date = to_date(sDate);date) {
					p.date = *date;
					pos = in.tellg(); // Accept this parse position (an actual date was parsed)
				}
			}
			catch (std::exception const& e) { /* swallow all failuires silently for optional parse */}
			in.seekg(pos); // Reset position in case of failed parse
			in.clear(); // Reset failbit to allow try for other parse
		}
		return in;
	}
	std::istream& operator>>(std::istream& in,YYYYMMdd_parser& p) {
		if (in.good()) {
			std::optional<std::chrono::year_month_day> od{};
			optional_YYYYMMdd_parser op{od};
			in >> op;
			if (op.date) {
				p.date = *op.date;
			}
			else {
				in.setstate(std::ios::failbit);
			}
		}
		return in;
	}
	struct optional_Text_parser {
		std::optional<std::string>& text;
	};
	std::istream& operator>>(std::istream& in,optional_Text_parser& p) {
		if (in.good()) {
			auto pos = in.tellg();
			std::string text{};
			if (in >> std::quoted(text)) {
				p.text = text;
				pos = in.tellg(); // accept this parse pos
			}
			in.clear(); // Reset failbit to allow try for other parse
			in.seekg(pos); // Reset position in case of failed parse
		}
		return in;
	}
	struct Scraps {};
	std::istream& operator>>(std::istream& in,Scraps& p) {
		if (in.good()) {
			std::string scraps{};
			std::getline(in,scraps);
			// std::cout << "\nscraps:" << scraps;
		}
		return in;		
	}

	SIEParseResult parse_KONTO(Stream& in,std::string const& konto_tag) {
	// // #KONTO 8422 "Dr?jsm?lsr?ntor f?r leverant?rsskulder"
	// struct Konto {
	// 	std::string tag;
	// 	BASAccountNo account_no;
	// 	std::string name;
	// };
		SIEParseResult result{};
		SIE::Konto konto{};
		auto pos = in.tellg();
		if (in >> Tag{konto_tag} >> konto.account_no >> std::quoted(konto.name)) {

			// Convert from code point 437 to utf-8
			auto s437 = konto.name;
			auto sUnicode = cp435ToUnicode(s437);
			konto.name = UTF8::unicode_to_utf8(sUnicode);

			result = konto;
			pos = in.tellg(); // accept new stream position
		}
		in.seekg(pos); // Reset position in case of failed parse
		in.clear(); // Reset failbit to allow try for other parse
		return result;		
	}

	SIEParseResult parse_SRU(Stream& in,std::string const& sru_tag) {
		SIEParseResult result{};
		SIE::Sru sru{};
		auto pos = in.tellg();
		if (in >> Tag{sru_tag} >> sru.bas_account_no >> sru.sru_account_no) {
			result = sru;
			pos = in.tellg(); // accept new stream position
		}
		in.seekg(pos); // Reset position in case of failed parse
		in.clear(); // Reset failbit to allow try for other parse
		return result;		
	}

	SIEParseResult parse_TRANS(Stream& in,std::string const& trans_tag) {
		SIEParseResult result{};
		Scraps scraps{};
		auto pos = in.tellg();
		// #TRANS 2610 {} 25900 "" "" 0
		SIE::Trans trans{};
		optional_YYYYMMdd_parser optional_transdate_parser{trans.transdate};
		optional_Text_parser optional_transtext_parser{trans.transtext};
		if (in >> Tag{trans_tag} >> trans.account_no >> trans.object_list >> trans.amount >> optional_transdate_parser >> optional_transtext_parser >> scraps) {
			// std::cout << trans_tag << trans.account_no << " " << trans.amount;

			if (trans.transtext) {
				// Convert from code point 437 to utf-8
				auto s437 = *trans.transtext;
				auto sUnicode = cp435ToUnicode(s437);
				trans.transtext = UTF8::unicode_to_utf8(sUnicode);
			}

			result = trans;
			pos = in.tellg(); // accept new stream position
		}
		in.seekg(pos); // Reset position in case of failed parse
		in.clear(); // Reset failbit to allow try for other parse
		return result;		
	}

	SIEParseResult parse_Tag(Stream& in,std::string const& tag) {
		SIEParseResult result{};
		Scraps scraps{};
		auto pos = in.tellg();
		if (in >> Tag{tag}) {
			result = AnonymousLine{}; // Success but not data
			pos = in.tellg(); // accept new stream position
		}
		in.seekg(pos); // Reset position in case of failed parse
		in.clear(); // Reset failbit to allow try for other parse
		return result;		
	}

	SIEParseResult parse_ver(Stream& in) {
		SIEParseResult result{};
		Scraps scraps{};
		auto pos = in.tellg();
		// #VER A 1 20210505 "M�nadsavgift PG" 20210817
		SIE::Ver ver{};
		YYYYMMdd_parser verdate_parser{ver.verdate};
		if (in >> Tag{"#VER"} >> ver.series >> ver.verno >> verdate_parser >> std::quoted(ver.vertext) >> scraps >> scraps) {
			if (true) {
				// std::cout << "\nVer: " << ver.series << " " << ver.verno << " " << ver.vertext;
			}
			// Convert from code point 437 to utf-8
			auto s437 = ver.vertext;
			auto sUnicode = cp435ToUnicode(s437);
			ver.vertext = UTF8::unicode_to_utf8(sUnicode);

			while (true) {
				if (auto entry = parse_TRANS(in,"#TRANS")) {
					// std::cout << "\nTRANS :)";	
					ver.transactions.push_back(std::get<Trans>(*entry));					
				}
				else if (auto entry = parse_TRANS(in,"#BTRANS")) {
					// Ignore
					// std::cout << " Ignored";
				}
				else if (auto entry = parse_TRANS(in,"#RTRANS")) {
					// Ignore
					// std::cout << " Ignored";
				}
				else if (auto entry = parse_Tag(in,"}")) {
					break;
				}
				else {
					std::cerr << "\nERROR - Unexpected input while parsing #VER SIE entry";
					break;
				}
			}
			result = ver;
			pos = in.tellg(); // accept new stream position
		}
		in.seekg(pos); // Reset position in case of failed parse
		in.clear(); // Reset failbit to allow try for other parse
		return result;
	}

	SIEParseResult parse_any_line(Stream& in) {
		if (in.fail()) {
			// std::cout << "\nparse_any_line in==fail";
		}
		auto pos = in.tellg();
		std::string line{};
		if (std::getline(in,line)) {
			// std::cout << "\n\tany=" << line;
			return AnonymousLine{};
		}
		else {
			// std::cout << "\n\tany null";
			in.seekg(pos);
			return {};
		}
	}

	// ===============================================================
	// BEGIN operator<< framework for SIE::T stream to text stream in SIE file representation
	// ===============================================================
	struct ostream {
		std::ostream& os;
	};

	SIE::ostream& operator<<(SIE::ostream& sieos,SIE::Trans const& trans) {
		// #TRANS account no {object list} amount transdate transtext quantity sign
		//                                           o          o        o      o
		// #TRANS 1920 {} -890 "" "" 0
		sieos.os << "\n#TRANS"
		<< " " << trans.account_no
		<< " " << "{}"
		<< " " << trans.amount
		<< " " << std::quoted("");
		if (trans.transtext) sieos.os << " " << std::quoted(*trans.transtext);
		else sieos.os << " " << std::quoted("");
		return sieos;
	}
	
	SIE::ostream& operator<<(SIE::ostream& sieos,SIE::Ver const& ver) {
		// #VER A 1 20210505 "M�nadsavgift PG" 20210817	
		sieos.os << "\n#VER" 
		<< " " << ver.series 
		<< " " << ver.verno
		<< " " << to_string(ver.verdate) // TODO: make compiler find operator<< above (why can to_string use it but we can't?)
		<< " " << std::quoted(ver.vertext);
		sieos.os << "\n{";
		for (auto const& trans : ver.transactions) {
			sieos << trans;
		}
		sieos.os << "\n}";
		return sieos;
	}

	// ===============================================================
	// END operator<< framework for SIE::T stream to text stream in SIE file representation
	// ===============================================================

} // namespace SIE

SIE::Trans to_sie_t(BAS::AccountTransaction const& trans) {
	SIE::Trans result{
		.account_no = trans.account_no
		,.amount = trans.amount
		,.transtext = trans.transtext
	};
	return result;
}

SIE::Ver to_sie_t(BAS::JournalEntry const& jer) {
		/*
		Series series;
		BAS::VerNo verno;
		std::chrono::year_month_day verdate;
		std::string vertext;
		*/

	SIE::Ver result{
		.series = jer.series
		,.verno = (jer.verno)?*jer.verno:0
		,.verdate = jer.entry.date
		,.vertext = jer.entry.caption};
	for (auto const& trans : jer.entry.account_transactions) {
		result.transactions.push_back(to_sie_t(trans));
	}
	return result;
}

class FilteredSIEEnvironment {
public:
	FilteredSIEEnvironment(SIEEnvironment const& sie_environment,BAS::MatchesMetaEntry matches_meta_entry)
		:  m_sie_environment{sie_environment}
			,m_matches_meta_entry{matches_meta_entry} {}

	void for_each(auto const& f) const {
		for (auto const& [series,journal] : m_sie_environment.journals()) {
			for (auto const& [verno,aje] : journal) {
				BAS::MetaEntry me{this->m_sie_environment.is_unposted(series,verno),series,verno,aje};
				if (this->m_matches_meta_entry(me)) f(me);
			}
		}
	}
private:
	SIEEnvironment const& m_sie_environment;
	BAS::MatchesMetaEntry m_matches_meta_entry{};
};

std::ostream& operator<<(std::ostream& os,FilteredSIEEnvironment const& filtered_sie_environment) {
	struct stream_entry_to {
		std::ostream& os;
		void operator()(BAS::MetaEntry const& me) const {
			os << "\n";
			if (me.is_unposted) os << "*";
			else os << " ";
			os << " " << me.series << me.verno << " " << me.aje;
		}
	};
	os << "\n*Filter BEGIN*";
	filtered_sie_environment.for_each(stream_entry_to{os});
	os << "\n*Filter end*";
	return os;
}

std::ostream& operator<<(std::ostream& os,SIEEnvironment const& sie_environment) {
	for (auto const& je : sie_environment.journals()) {
		auto& [series,journal] = je;
		for (auto const& [verno,entry] : journal) {
			os << "\n";
			if (sie_environment.is_unposted(series,verno)) os << "*";
			else os << " ";
			os << " " << series << verno << " " << to_string(entry);
		}
	}
	return os;
}

BAS::JournalEntry to_entry(SIE::Ver const& ver) {
	BAS::JournalEntry result{
		.series = ver.series
		,.verno = ver.verno
	};
	result.entry.caption = ver.vertext;
	result.entry.date = ver.verdate;
	for (auto const& trans : ver.transactions) {								
		result.entry.account_transactions.push_back(BAS::AccountTransaction{
			.account_no = trans.account_no
			,.transtext = trans.transtext
			,.amount = trans.amount
		});
	}
	return result;
}

OptionalSIEEnvironment from_sie_file(std::filesystem::path const& sie_file_path) {
	OptionalSIEEnvironment result{};
	std::ifstream in{sie_file_path};
	if (in) {
		SIEEnvironment sie_environment{};
		while (true) {
			// std::cout << "\nparse";
			if (auto opt_entry = SIE::parse_KONTO(in,"#KONTO")) {
				SIE::Konto konto = std::get<SIE::Konto>(*opt_entry);
				sie_environment.account_metas()[konto.account_no].name = konto.name;
			}
			else if (auto opt_entry = SIE::parse_SRU(in,"#SRU")) {
				SIE::Sru sru = std::get<SIE::Sru>(*opt_entry);
				sie_environment.account_metas()[sru.bas_account_no].sru_code = sru.sru_account_no;
			}
			else if (auto opt_entry = SIE::parse_ver(in)) {
				SIE::Ver ver = std::get<SIE::Ver>(*opt_entry);
				// std::cout << "\n\tVER!";
				auto je = to_entry(ver);
				sie_environment.post(je);
			}
			else if (auto opt_entry = SIE::parse_any_line(in)) {
				// std::cout << "\n\tANY";
			}
			else break;
		}
		result = std::move(sie_environment);
	}
	return result;
}

void unposted_to_sie_file(SIEEnvironment const& sie,std::filesystem::path const& p) {
	// std::cout << "\nunposted_to_sie_file " << p;
	std::ofstream os{p};
	SIE::ostream sieos{os};
	// auto now = std::chrono::utc_clock::now();
	auto now = std::chrono::system_clock::now();
	auto now_timet = std::chrono::system_clock::to_time_t(now);
	auto now_local = localtime(&now_timet);
	sieos.os << "\n" << "#GEN " << std::put_time(now_local, "%Y%m%d");
	for (auto const& entry : sie.unposted()) {
		// std::cout << entry; 
		sieos << to_sie_t(entry);
	}
}

std::vector<std::string> quoted_tokens(std::string const& cli) {
	std::vector<std::string> result{};
	std::istringstream in{cli};
	std::string token{};
	while (in >> std::quoted(token)) result.push_back(token);
	return result;
}

std::string prompt_line(PromptState const& prompt_state) {
	std::ostringstream prompt{};
	prompt << "\ncratchit";
	switch (prompt_state) {
		case PromptState::Root: {
			prompt << ":";
		} break;
		case PromptState::HADIndex: {
			prompt << ":had";
		} break;
		case PromptState::JEIndex: {
			prompt << ":had:je";
		} break;
		case PromptState::AccountIndex: {
			prompt << ":had:je:account";
		} break;
		case PromptState::Amount: {
			prompt << ":had:je:account:amount";
		} break;
		case PromptState::CounterAccountsEntry: {
			prompt << "had:je:at";
		} break;
		default: {
			prompt << ":??";
		}
	}
	prompt << ">";
	return prompt.str();
}

auto falling_date = [](auto const& had1,auto const& had2){
	return (had1.date > had2.date);
};

class Updater {
public:
	Model model;
	Cmd operator()(Key const& key) {
		// std::cout << "\noperator(Key)";
		if (model->user_input.size()==0) model->prompt = prompt_line(model->prompt_state);
		Cmd cmd{};
		if (std::isprint(key.value)) {
			model->user_input += key.value;
		}
		else {
			cmd = to_cmd(model->user_input);
			model->user_input.clear();
		}
		return cmd;
	}
	Cmd operator()(Command const& command) {
		// std::cout << "\noperator(Command)";
		std::ostringstream prompt{};
		auto ast = quoted_tokens(command);
		if (ast.size() > 0) {			
			int signed_ix{};
			std::istringstream is{ast[0]};
			if (model->prompt_state != PromptState::Amount and is >> signed_ix) {
				size_t ix = std::abs(signed_ix);
				bool do_remove = (signed_ix<0);
				// Act on prompt state index input				
				switch (model->prompt_state) {
					case PromptState::Root: {

					} break;
					case PromptState::HADIndex: {
						auto iter = model->heading_amount_date_entries.begin();
						auto end = model->heading_amount_date_entries.end();
						std::advance(iter,ix);
						if (iter != end) {
							auto had = *iter;
							prompt << "\n" << had;
							if (do_remove) {
								model->heading_amount_date_entries.erase(iter);
								prompt << " REMOVED";
								model->prompt_state = PromptState::Root;
							}
							else {
								model->had_index = ix;
								// model->template_candidates = template_candidates(model->sie["current"].journals(),[had](BAS::anonymous::JournalEntry const& je){
								// 	return had_matches_trans(had,je);
								// });
								model->template_candidates = this->all_years_template_candidates([had](BAS::anonymous::JournalEntry const& je){
									return had_matches_trans(had,je);
								});
								for (int i = 0; i < model->template_candidates.size(); ++i) {
									prompt << "\n    " << i << " " << model->template_candidates[i];
								}
								model->prompt_state = PromptState::JEIndex;
							}
						}
						else {
							prompt << "\nplease enter a valid index";
						}
					} break;
					case PromptState::JEIndex: {
						auto had_iter = model->heading_amount_date_entries.begin();
						auto end = model->heading_amount_date_entries.end();
						std::advance(had_iter,model->had_index);
						if (had_iter != end) {
							auto had = *had_iter;
							if (auto account_no = BAS::to_account_no(command)) {
								// Assume user entered an account number for a Gross + 1..n <Ex vat, Vat> account entries
								std::cout << "\nGross Account detected";
								BAS::JournalEntry je{};
								je.entry.caption = had.heading;
								je.entry.date = had.date;
								je.entry.account_transactions.emplace_back(BAS::AccountTransaction{.account_no=*account_no,.amount=had.amount});
								had.current_candidate = je;
								// List the options to the user
								unsigned int i{};
								prompt << "\n" << had.current_candidate->entry.caption << " " << had.current_candidate->entry.date;
								for (auto const& at : had.current_candidate->entry.account_transactions) {
									prompt << "\n  " << i++ << " " << at;
								}				
								model->prompt_state = PromptState::CounterAccountsEntry;
							}
							else {
								// Assume user selected an entry as base for a template
								auto je_iter = model->template_candidates.begin();
								auto end = model->template_candidates.end();
								std::advance(je_iter,ix);
								if (je_iter != end) {
									auto tp = to_template(*je_iter);
									if (tp) {
										auto je = to_journal_entry(had,*tp);
										// auto je = to_journal_entry(model->selected_had,*tp);
										if (std::any_of(je.entry.account_transactions.begin(),je.entry.account_transactions.end(),[](BAS::AccountTransaction const& at){
											return std::abs(at.amount) < 1.0;
										})) {
											// Assume we need to specify rounding
											unsigned int i{};
											std::for_each(je.entry.account_transactions.begin(),je.entry.account_transactions.end(),[&i,&prompt](auto const& at){
												prompt << "\n  " << i++ << " " << at;
											});
											had.current_candidate = je;
											model->prompt_state = PromptState::AccountIndex;
										}
										else {
											// Assume we can apply template as-is and stage the generated journal entry
											auto staged_je = model->sie["current"].stage(je);
											if (staged_je) {
												prompt << "\n" << *staged_je << " STAGED";
												model->heading_amount_date_entries.erase(had_iter);
												model->prompt_state = PromptState::HADIndex;
											}
											else {
												prompt << "\nSORRY - Failed to stage entry";
												model->prompt_state = PromptState::Root;
											}
										}
									}
								}
								else {
									prompt << "\nPlease enter a valid index";
								}
							}
						}
					} break;

					case PromptState::AccountIndex: {
						if (auto at = model->selected_had_at(ix)) {
							model->at = *at;
							model->prompt_state = PromptState::Amount;
						}
						else {
							prompt << "\nEntered index does not refer to an Account Trasnaction Entry in current Heading Amount Date entry";
						}
					} break;

					case PromptState::Amount:
					case PromptState::Undefined:
					case PromptState::Unknown:
						break;
				}
			}
			else if (ast[0] == "-sie") {
				// Import sie and add as base of our environment
				if (ast.size()==1) {
					// List current sie environment
					prompt << model->sie["current"];					
					// std::cout << model->sie["current"];
				}
				else if (ast.size()==2) {
					if (model->sie.contains(ast[1])) prompt << model->sie[ast[1]];
					else if (ast[1]=="*") {
						// List unposted sie entries
						FilteredSIEEnvironment filtered_sie{model->sie["current"],BAS::filter::is_unposted{}};
						prompt << filtered_sie;
					}
					else if (auto sie_file_path = path_to_existing_file(ast[1])) {
						if (auto sie_env = from_sie_file(*sie_file_path)) {
							model->sie_file_path["current"] = *sie_file_path;
							model->sie["current"] = std::move(*sie_env);
							if (auto sse = from_sie_file(model->staged_sie_file_path)) {
								auto unstaged = model->sie["current"].stage(*sse);
								for (auto const& je : unstaged) {
									prompt << "\nnow posted " << je; 
								}
							}							
						}
						else {
							// failed to parse sie-file into an SIE Environment 
							prompt << "\nERROR - Failed to import sie file " << *sie_file_path;
						}
					}
					else {
						// assume user search criteria on transaction heading and comments
						FilteredSIEEnvironment filtered_sie{model->sie["current"],BAS::filter::matches_user_search_criteria{ast[1]}};
						prompt << filtered_sie;
					}
				}
				else if (ast.size()==3) {
					auto year_key = ast[1];
					if (auto sie_file_path = path_to_existing_file(ast[2])) {
						if (auto sie_env = from_sie_file(*sie_file_path)) {
							model->sie_file_path[year_key] = *sie_file_path;
							model->sie[year_key] = std::move(*sie_env);
						}
						else {
							// failed to parse sie-file into an SIE Environment 
							prompt << "\nERROR - Failed to import sie file " << *sie_file_path;
						}
					}
					else {
						// assume user search criteria on transaction heading and comments
						if (model->sie.contains(year_key)) {
							FilteredSIEEnvironment filtered_sie{model->sie[year_key],BAS::filter::matches_user_search_criteria{ast[2]}};
							prompt << filtered_sie;
						}
						else {
							prompt << "\nYear identifier " << year_key << " is not associated with any data";
						}
					}
				}
			}
			else if (ast[0] == "-had") {
				if (ast.size()==1) {
					// Expose current hads (Heading Amount Date transaction entries) to the user
					std::sort(model->heading_amount_date_entries.begin(),model->heading_amount_date_entries.end(),falling_date);
					auto& hads = model->heading_amount_date_entries;
					unsigned int index{0};
					std::vector<std::string> sHads{};
					std::transform(hads.begin(),hads.end(),std::back_inserter(sHads),[&index](auto const& had){
						std::stringstream os{};
						os << index++ << " " << had;
						return os.str();
					});
					prompt << std::accumulate(sHads.begin(),sHads.end(),std::string{"Please select:"},[](auto acc,std::string const& entry) {
						acc += "\n  " + entry;
						return acc;
					});
					model->prompt_state = PromptState::HADIndex;
				}
			}
			else if (ast[0] == "-meta") {
				for (auto const& [account_no,am] : model->sie["current"].account_metas()) {
					prompt << "\n  " << account_no << " " << std::quoted(am.name);
					if (am.sru_code) prompt << " SRU:" << *am.sru_code;
				}
			}
			else if (ast[0] == "-sru") {
				// List SRU Accounts mapped to BAS Accounts
				auto const& account_metas = model->sie["current"].account_metas();
				auto sru_map = sru_to_bas_map(account_metas);
				for (auto const& [sru_account,bas_accounts] : sru_map) {
					prompt << "\nSRU:" << sru_account;
					for (auto const& bas_account_no : bas_accounts) {
						prompt << "\n  BAS:" << bas_account_no << " " << std::quoted(account_metas.at(bas_account_no).name);
					}
				}
			}
			else if (ast[0] == "-gross") {
				auto gats = gross_account_transactions(model->sie);
				for (auto const& gat : gats) {
					prompt << "\n" << gat;
				}				
			}
			else if (ast[0] == "-net") {
				auto nats = net_account_transactions(model->sie);
				for (auto const& nat : nats) {
					prompt << "\n" << nat;
				}				
			}
			else if (ast[0] == "-vat") {
				auto vats = vat_account_transactions(model->sie);
				for (auto const& vat : vats) {
					prompt << "\n" << vat;
				}				
			}
			else if (ast[0] == "-t2") {
				auto t2s = t2_entries(model->sie);
				prompt << t2s;
			}
			else if (ast[0] == "-skv") {
				std::filesystem::path skv_file_path{"to_skv.xml"};
				std::ofstream skv_file{skv_file_path};
				if (SKV::XML::to_employer_contributions_and_PAYE_tax_return_file(skv_file)) {
					prompt << "\nCreated " << skv_file_path;
				}
				else {
					prompt << "\nSorry, filed to create " << skv_file_path;
				}
			}
			else if (ast[0] == "-csv") {
				// std::cout << "\nCSV :)";
				// Assume Finland located bank Nordea swedish web csv format of transactions to/from an account
				/*
				Bokföringsdag;Belopp;Avsändare;Mottagare;Namn;Rubrik;Meddelande;Egna anteckningar;Saldo;Valuta
				2021-12-15;-1394,00;51 86 87-9;824-3040;TELIA SVERIGE AB;TELIA SVERIGE AB;21457751218;Samtal Q4;56080,17;SEK
				2021-12-15;-419,65;51 86 87-9;;KORT             BEANSTALK APP   26;KORT             BEANSTALK APP   26;BEANSTALK APP   2656;;;SEK
				2021-12-13;-139,88;51 86 87-9;;KORT             BEANSTALK APP   26;KORT             BEANSTALK APP   26;BEANSTALK APP   2656;;57893,82;SEK
				2021-12-10;-109,00;51 86 87-9;;KORT             APPLE COM BILL  26;KORT             APPLE COM BILL  26;APPLE COM BILL  2656;;58033,70;SEK
				2021-12-09;-593,58;51 86 87-9;;KORT             PAYPAL *PADDLE  26;KORT             PAYPAL *PADDLE  26;PAYPAL *PADDLE  2656;;58142,70;SEK
				2021-12-03;-3,40;51 86 87-9;;PRIS ENL SPEC;PRIS ENL SPEC;;;58736,28;SEK
				*/
				if (ast.size()>1) {
					std::filesystem::path csv_file_path{ast[1]};
					// std::cout << "\ncsv file " << csv_file_path;
					std::ifstream ifs{csv_file_path};
					if (std::filesystem::exists(csv_file_path)) {
						// std::cout << "\ncsv file exists ok";
						CSV::NORDEA::istream in{ifs};
						auto hads = CSV::from_stream(in);
						std::copy(hads.begin(),hads.end(),std::back_inserter(model->heading_amount_date_entries));
					}
				}
				else {
					prompt << "\nERROR - Please provide a path to a file";
				}
			}
			else if (ast[0] == "-") {
				model->prompt_state = PromptState::Root;
			}
			else {
				if (model->prompt_state == PromptState::JEIndex) {
					// Assume the user has entered a new search criteria for template candidates
					model->template_candidates = this->all_years_template_candidates([&command](BAS::anonymous::JournalEntry const& je){
						return do_share_tokens(command,je.caption);
					});
					int i{0};
					for (; i < model->template_candidates.size(); ++i) {
						prompt << "\n    " << i << " " << model->template_candidates[i];
					}
					// Consider the user may have entered the name of a gross account to journal the transaction amount
					auto gats = gross_account_transactions(model->sie);
					model->at_candidates.clear();
					std::copy_if(gats.begin(),gats.end(),std::back_inserter(model->at_candidates),[&command,this](BAS::AccountTransaction const& at){
						bool result{false};
						if (at.transtext) result |= do_share_tokens(command,*at.transtext);
						auto const& meta = model->sie["current"].account_metas().at(at.account_no);
						if (model->sie["current"].account_metas().contains(at.account_no)) {
							result |= do_share_tokens(command,meta.name);
						}
						return result;
					});
					int ei{0};
					for (;ei < model->at_candidates.size();++ei) {
						prompt << "\n    " << i+ei << " " << model->at_candidates[ei];
					}
				}
				else if (model->prompt_state == PromptState::CounterAccountsEntry) {
					if (auto nha = to_name_heading_amount(ast)) {
						// List account candidates for the assumed "Name, Heading + Amount" entry by the user
						std::cout << "\nAccount:" << std::quoted(nha->account_name);
						if (nha->trans_text) std::cout << " text:" << std::quoted(*nha->trans_text);
						std::cout << " amount:" << nha->amount;
					}
					else {
						prompt << "\nPlease enter an account, and optional transaction text and an amount";
					}
					// List the new current options
					if (auto had = model->selected_had()) {
						if (had->current_candidate) {
							unsigned int i{};
							prompt << "\n" << had->current_candidate->entry.caption << " " << had->current_candidate->entry.date;
							for (auto const& at : had->current_candidate->entry.account_transactions) {
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
				else if (model->prompt_state == PromptState::Amount) {
					std::cout << "\nPromptState::Amount " << std::quoted(command);
					if (auto had = model->selected_had()) {
						if (auto amount = to_amount(command)) {
							prompt << "\nAmount " << *amount;
							model->at.amount = *amount;
							if (had->current_candidate) {
								had->current_candidate = updated_entry(*had->current_candidate,model->at);
								auto je = model->sie["current"].stage(*had->current_candidate);
								if (je) {
									prompt << "\n" << *je;
									// Erase the 'had' we just turned into journal entry and staged
									model->erease_selected_had();
								}
								else {
									prompt << "\n" << "Sorry - This transaction redeemed a duplicate";
								}
								model->prompt_state = PromptState::Root;
							}
							else {
								prompt << "\nPlease ascociate current Heading Amount Date entry with a Journal Entry candidate";
							}
						}
						else {
							prompt << "\nPlease enter an Amount";
						}

					}
					else {
						prompt << "\nPlease select a Heading Amount Date entry";
					}

					try {
						auto amount = to_amount(command);
						if (amount) {
						}
						else {
							prompt << "\nNot an amount - " << std::quoted(command);
						}
					}
					catch (std::exception const& e) {
						prompt << "\nERROR - " << e.what();
						prompt << "\nPlease enter a valid amount";
					}
				}
				else {
					// Assume Caption + Amount + Date
					auto tokens = tokenize::splits(command,tokenize::SplitOn::TextAmountAndDate);			
					if (tokens.size()==3) {
						HeadingAmountDateTransEntry had {
							.heading = tokens[0]
							,.amount = *to_amount(tokens[1]) // Assume success
							,.date = *to_date(tokens[2]) // Assume success
						};
						prompt << "\n" << had;
						model->heading_amount_date_entries.push_back(had);
						// Decided NOT to sort hads here to keep last listed had indexes intact (new -had command = sort and new indexes)
					}
					else {
						prompt << "\nERROR - Expected Caption + Amount + Date";
					}
				}
			}
		}
		prompt << prompt_line(model->prompt_state);
		model->prompt = prompt.str();
		return {};
	}
	Cmd operator()(Quit const& quit) {
		// std::cout << "\noperator(Quit)";
		std::ostringstream os{};
		os << "\nBy for now :)";
		model->prompt = os.str();
		model->quit = true;
		return {};
	}
	Cmd operator()(Nop const& nop) {
		// std::cout << "\noperator(Nop)";
		std::ostringstream prompt{};
		auto help = help_for(model->prompt_state);
		for (auto const& line : help) prompt << "\n" << line;
		prompt << prompt_line(model->prompt_state);
		model->prompt = prompt.str();
		return {};
	}	
private:
	BAS::JournalEntries all_years_template_candidates(auto const& matches) {
		BAS::JournalEntries result{};
		for (auto const& [year_key,sie] : model->sie) {
			auto jes = template_candidates(sie.journals(),matches);
			std::copy(jes.begin(),jes.end(),std::back_inserter(result));
		}
		std::sort(result.begin(),result.end(),[](auto const& je1,auto const& je2){
			return (je1.entry.date < je2.entry.date);
		});
		return result;
	}
};

class Cratchit {
public:
	Cratchit(std::filesystem::path const& p) 
		: cratchit_file_path{p} {}

	Model init(Command const& command) {		
		std::ostringstream prompt{};
		prompt << "\nInit from ";
		prompt << cratchit_file_path;
		auto environment = environment_from_file(cratchit_file_path);
		auto model = this->model_from_environment(environment);
		model->prompt_state = PromptState::Root;
		prompt << prompt_line(model->prompt_state);
		model->prompt = prompt.str();
		return model;
	}
	std::pair<Model,Cmd> update(Msg const& msg,Model&& model) {
		Updater updater{std::move(model)};
		auto cmd = std::visit(updater,msg);
		if (updater.model->quit) {
			auto model_environment = environment_from_model(updater.model);
			auto cratchit_environment = add_cratchit_environment(model_environment);
			this->environment_to_file(cratchit_environment,cratchit_file_path);
			unposted_to_sie_file(updater.model->sie["current"],updater.model->staged_sie_file_path);
		}
		return {std::move(updater.model),cmd};
	}
	Ux view(Model const& model) {
		Ux ux{};
		ux.push_back(model->prompt);
		return ux;
	}
private:
	std::filesystem::path cratchit_file_path{};
	HeadingAmountDateTransEntries hads_from_environment(Environment const& environment) {
		HeadingAmountDateTransEntries result{};
		auto [begin,end] = environment.equal_range("HeadingAmountDateTransEntry");
		std::transform(begin,end,std::back_inserter(result),[](auto const& entry){
			return *to_had(entry.second); // Assume success
		});
		return result;
	}
	bool is_value_line(std::string const& line) {
		return (line.size()==0)?false:(line.substr(0,2) != R"(//)");
	}
	Model model_from_environment(Environment const& environment) {
		Model model = std::make_unique<ConcreteModel>();
		if (auto val_iter = environment.find("sie_file");val_iter != environment.end()) {
			for (auto const& [year_key,sie_file_name] : val_iter->second) {
				std::filesystem::path sie_file_path{sie_file_name};
				if (auto sie_environment = from_sie_file(sie_file_path)) {
					model->sie[year_key] = std::move(*sie_environment);
					model->sie_file_path[year_key] = {sie_file_name};		
				}
			}
		}
		if (auto sse = from_sie_file(model->staged_sie_file_path)) {
			model->sie["current"].stage(*sse);
		}
		model->heading_amount_date_entries = this->hads_from_environment(environment);
		return model;
	}
	Environment environment_from_model(Model const& model) {
		Environment result{};
		for (auto const& entry :  model->heading_amount_date_entries) {
			result.insert({"HeadingAmountDateTransEntry",to_environment_value(entry)});
		}
		std::string sev = std::accumulate(model->sie_file_path.begin(),model->sie_file_path.end(),std::string{},[](auto acc,auto const& entry){
			std::ostringstream os{};
			if (acc.size()>0) os << acc << ";";
			os << entry.first << "=" << entry.second.string();
			return os.str();
		});
		sev += std::string{";"} + "path" + "=" + model->sie_file_path["current"].string();
		result.insert({"sie_file",to_environment_value(sev)});
		return result;
	}
	Environment add_cratchit_environment(Environment const& environment) {
		Environment result{environment};
		// Add cratchit environment values if/when there are any
		return result;
	}

	// HeadingAmountDateTransEntry "belopp=1389,50;datum=20221023;rubrik=Klarna"
	Environment environment_from_file(std::filesystem::path const& p) {
		Environment result{};
		try {
			std::ifstream in{p};
			std::string line{};
			while (std::getline(in,line)) {
				if (is_value_line(line)) {
					std::istringstream in{line};
					std::string key{},value{};
					in >> key >> std::quoted(value);
					result.insert({key,to_environment_value(value)});
				}
			}
		}
		catch (std::runtime_error const& e) {
			std::cerr << "\nERROR - Read from " << p << " failed. Exception:" << e.what();
		}
		return result;
	}
	void environment_to_file(Environment const& environment, std::filesystem::path const& p) {
		try {
			std::ofstream out{p};		
			for (auto iter = environment.begin(); iter != environment.end();++iter) {
				if (iter == environment.begin()) out << to_string(*iter);
				else out << "\n" << to_string(*iter);
			}
		}
		catch (std::runtime_error const& e) {
			std::cerr << "\nERROR - Write to " << p << " failed. Exception:" << e.what();
		}
	}
};

class REPL {
public:
    REPL(std::filesystem::path const& environment_file_path) : cratchit{environment_file_path} {}

	void run(Command const& command) {
        auto model = cratchit.init(command);
		while (true) {
			auto ux = cratchit.view(model);
			render_ux(ux);
			if (model->quit) break; // Done
			// process events (user input)
			if (in.size()>0) {
				auto msg = in.front();
				in.pop();
				// std::cout << "\nmsg[" << msg.index() << "]";
				// Turn Event (Msg) into updated model
				auto [updated_model,cmd] = cratchit.update(msg,std::move(model));
				model = std::move(updated_model);
				if (cmd.msg) in.push(*cmd.msg);
			}
			else {
				// Get more buffered user input
				std::string user_input{};
				std::getline(std::cin,user_input);
				std::for_each(user_input.begin(),user_input.end(),[this](char ch){
					this->in.push(Key{ch});
				});
				this->in.push(Key{'\n'});
			}
		}
	}
private:
    Cratchit cratchit;
		Ux cached_ux{};
		void render_ux(Ux const& ux) {
			if (ux != cached_ux) {
				cached_ux = ux;
				for (auto const&  row : ux) {
					if (row.size()>0) std::cout << row;
				}
			}
		}
    std::queue<Msg> in{};
};

int main(int argc, char *argv[])
{
	if (false) {
		// Log current locale and test charachter encoding.
		// TODO: Activate to adjust for cross platform handling 
			std::wcout << "\nDeafult (current) locale setting is " << std::locale().name().c_str();
			std::string sHello{"Hallå Åland! Ömsom ödmjuk. Ärligt äkta."}; // This source file expected to be in UTF-8
			std::cout << "\ncout:" << sHello; // macOS handles UTF-8 ok (how about other platforms?)
			for (auto const& ch : sHello) {
				std::cout << "\n" << ch << " " << std::hex << static_cast<int>(ch) << std::dec; // check stream and console encoding, std::locale behaviour
			}
			std::string sPATH{std::getenv("PATH")};
			// std::cout << "\nPATH=" << sPATH;
	}
	std::string command{};
	for (int i=1;i < argc;i++) command+= std::string{argv[i]} + " ";
	auto current_path = std::filesystem::current_path();
	auto environment_file_path = current_path / "cratchit.env";
	REPL repl{environment_file_path};
	repl.run(command);
	// std::cout << "\nBye for now :)";
	std::cout << std::endl;
	return 0;
}

namespace SIE {

	// from http://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/CP437.TXT
	std::map<char,char16_t> cp435ToUnicodeMap{
	{0x0,0x0}
	,{0x1,0x1}
	,{0x2,0x2}
	,{0x3,0x3}
	,{0x4,0x4}
	,{0x5,0x5}
	,{0x6,0x6}
	,{0x7,0x7}
	,{0x8,0x8}
	,{0x9,0x9}
	,{0xA,0xA}
	,{0xB,0xB}
	,{0xC,0xC}
	,{0xD,0xD}
	,{0xE,0xE}
	,{0xF,0xF}
	,{0x10,0x10}
	,{0x11,0x11}
	,{0x12,0x12}
	,{0x13,0x13}
	,{0x14,0x14}
	,{0x15,0x15}
	,{0x16,0x16}
	,{0x17,0x17}
	,{0x18,0x18}
	,{0x19,0x19}
	,{0x1A,0x1A}
	,{0x1B,0x1B}
	,{0x1C,0x1C}
	,{0x1D,0x1D}
	,{0x1E,0x1E}
	,{0x1F,0x1F}
	,{0x20,0x20}
	,{0x21,0x21}
	,{0x22,0x22}
	,{0x23,0x23}
	,{0x24,0x24}
	,{0x25,0x25}
	,{0x26,0x26}
	,{0x27,0x27}
	,{0x28,0x28}
	,{0x29,0x29}
	,{0x2A,0x2A}
	,{0x2B,0x2B}
	,{0x2C,0x2C}
	,{0x2D,0x2D}
	,{0x2E,0x2E}
	,{0x2F,0x2F}
	,{0x30,0x30}
	,{0x31,0x31}
	,{0x32,0x32}
	,{0x33,0x33}
	,{0x34,0x34}
	,{0x35,0x35}
	,{0x36,0x36}
	,{0x37,0x37}
	,{0x38,0x38}
	,{0x39,0x39}
	,{0x3A,0x3A}
	,{0x3B,0x3B}
	,{0x3C,0x3C}
	,{0x3D,0x3D}
	,{0x3E,0x3E}
	,{0x3F,0x3F}
	,{0x40,0x40}
	,{0x41,0x41}
	,{0x42,0x42}
	,{0x43,0x43}
	,{0x44,0x44}
	,{0x45,0x45}
	,{0x46,0x46}
	,{0x47,0x47}
	,{0x48,0x48}
	,{0x49,0x49}
	,{0x4A,0x4A}
	,{0x4B,0x4B}
	,{0x4C,0x4C}
	,{0x4D,0x4D}
	,{0x4E,0x4E}
	,{0x4F,0x4F}
	,{0x50,0x50}
	,{0x51,0x51}
	,{0x52,0x52}
	,{0x53,0x53}
	,{0x54,0x54}
	,{0x55,0x55}
	,{0x56,0x56}
	,{0x57,0x57}
	,{0x58,0x58}
	,{0x59,0x59}
	,{0x5A,0x5A}
	,{0x5B,0x5B}
	,{0x5C,0x5C}
	,{0x5D,0x5D}
	,{0x5E,0x5E}
	,{0x5F,0x5F}
	,{0x60,0x60}
	,{0x61,0x61}
	,{0x62,0x62}
	,{0x63,0x63}
	,{0x64,0x64}
	,{0x65,0x65}
	,{0x66,0x66}
	,{0x67,0x67}
	,{0x68,0x68}
	,{0x69,0x69}
	,{0x6A,0x6A}
	,{0x6B,0x6B}
	,{0x6C,0x6C}
	,{0x6D,0x6D}
	,{0x6E,0x6E}
	,{0x6F,0x6F}
	,{0x70,0x70}
	,{0x71,0x71}
	,{0x72,0x72}
	,{0x73,0x73}
	,{0x74,0x74}
	,{0x75,0x75}
	,{0x76,0x76}
	,{0x77,0x77}
	,{0x78,0x78}
	,{0x79,0x79}
	,{0x7A,0x7A}
	,{0x7B,0x7B}
	,{0x7C,0x7C}
	,{0x7D,0x7D}
	,{0x7E,0x7E}
	,{0x7F,0x7F}
	,{0x80,0xC7}
	,{0x81,0xFC}
	,{0x82,0xE9}
	,{0x83,0xE2}
	,{0x84,0xE4}
	,{0x85,0xE0}
	,{0x86,0xE5}
	,{0x87,0xE7}
	,{0x88,0xEA}
	,{0x89,0xEB}
	,{0x8A,0xE8}
	,{0x8B,0xEF}
	,{0x8C,0xEE}
	,{0x8D,0xEC}
	,{0x8E,0xC4}
	,{0x8F,0xC5}
	,{0x90,0xC9}
	,{0x91,0xE6}
	,{0x92,0xC6}
	,{0x93,0xF4}
	,{0x94,0xF6}
	,{0x95,0xF2}
	,{0x96,0xFB}
	,{0x97,0xF9}
	,{0x98,0xFF}
	,{0x99,0xD6}
	,{0x9A,0xDC}
	,{0x9B,0xA2}
	,{0x9C,0xA3}
	,{0x9D,0xA5}
	,{0x9E,0x20A7}
	,{0x9F,0x192}
	,{0xA0,0xE1}
	,{0xA1,0xED}
	,{0xA2,0xF3}
	,{0xA3,0xFA}
	,{0xA4,0xF1}
	,{0xA5,0xD1}
	,{0xA6,0xAA}
	,{0xA7,0xBA}
	,{0xA8,0xBF}
	,{0xA9,0x2310}
	,{0xAA,0xAC}
	,{0xAB,0xBD}
	,{0xAC,0xBC}
	,{0xAD,0xA1}
	,{0xAE,0xAB}
	,{0xAF,0xBB}
	,{0xB0,0x2591}
	,{0xB1,0x2592}
	,{0xB2,0x2593}
	,{0xB3,0x2502}
	,{0xB4,0x2524}
	,{0xB5,0x2561}
	,{0xB6,0x2562}
	,{0xB7,0x2556}
	,{0xB8,0x2555}
	,{0xB9,0x2563}
	,{0xBA,0x2551}
	,{0xBB,0x2557}
	,{0xBC,0x255D}
	,{0xBD,0x255C}
	,{0xBE,0x255B}
	,{0xBF,0x2510}
	,{0xC0,0x2514}
	,{0xC1,0x2534}
	,{0xC2,0x252C}
	,{0xC3,0x251C}
	,{0xC4,0x2500}
	,{0xC5,0x253C}
	,{0xC6,0x255E}
	,{0xC7,0x255F}
	,{0xC8,0x255A}
	,{0xC9,0x2554}
	,{0xCA,0x2569}
	,{0xCB,0x2566}
	,{0xCC,0x2560}
	,{0xCD,0x2550}
	,{0xCE,0x256C}
	,{0xCF,0x2567}
	,{0xD0,0x2568}
	,{0xD1,0x2564}
	,{0xD2,0x2565}
	,{0xD3,0x2559}
	,{0xD4,0x2558}
	,{0xD5,0x2552}
	,{0xD6,0x2553}
	,{0xD7,0x256B}
	,{0xD8,0x256A}
	,{0xD9,0x2518}
	,{0xDA,0x250C}
	,{0xDB,0x2588}
	,{0xDC,0x2584}
	,{0xDD,0x258C}
	,{0xDE,0x2590}
	,{0xDF,0x2580}
	,{0xE0,0x3B1}
	,{0xE1,0xDF}
	,{0xE2,0x393}
	,{0xE3,0x3C0}
	,{0xE4,0x3A3}
	,{0xE5,0x3C3}
	,{0xE6,0xB5}
	,{0xE7,0x3C4}
	,{0xE8,0x3A6}
	,{0xE9,0x398}
	,{0xEA,0x3A9}
	,{0xEB,0x3B4}
	,{0xEC,0x221E}
	,{0xED,0x3C6}
	,{0xEE,0x3B5}
	,{0xEF,0x2229}
	,{0xF0,0x2261}
	,{0xF1,0xB1}
	,{0xF2,0x2265}
	,{0xF3,0x2264}
	,{0xF4,0x2320}
	,{0xF5,0x2321}
	,{0xF6,0xF7}
	,{0xF7,0x2248}
	,{0xF8,0xB0}
	,{0xF9,0x2219}
	,{0xFA,0xB7}
	,{0xFB,0x221A}
	,{0xFC,0x207F}
	,{0xFD,0xB2}
	,{0xFE,0x25A0}
	,{0xFF,0xA0}
	};
} // namespace SIE