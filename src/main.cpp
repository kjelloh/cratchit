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
#include <set>
 
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

std::string filtered(std::string const& s,auto filter) {
	std::string result{};;
	std::copy_if(s.begin(),s.end(),std::back_inserter(result),filter);
	return result;
}

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

bool strings_share_tokens(std::string const& s1,std::string const& s2) {
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

using Amount= float;
using optional_amount = std::optional<Amount>;

using Date = std::chrono::year_month_day;
using OptionalDate = std::optional<Date>;

std::ostream& operator<<(std::ostream& os, Date const& yyyymmdd) {
	// TODO: Remove when support for ostream << chrono::year_month_day (g++11 stdlib seems to lack support?)
	os << std::setfill('0') << std::setw(4) << static_cast<int>(yyyymmdd.year());
	os << std::setfill('0') << std::setw(2) << static_cast<unsigned>(yyyymmdd.month());
	os << std::setfill('0') << std::setw(2) << static_cast<unsigned>(yyyymmdd.day());
	return os;
}
std::string to_string(Date const& yyyymmdd) {
		std::ostringstream os{};
		os << yyyymmdd;
		return os.str();
}

Date to_date(int year,unsigned month,unsigned day) {
	return Date {
		std::chrono::year{year}
		,std::chrono::month{month}
		,std::chrono::day{day}
	};
}

OptionalDate to_date(std::string const& sYYYYMMDD) {
	// std::cout << "\nto_date(" << sYYYYMMDD << ")";
	OptionalDate result{};
	if (sYYYYMMDD.size()==8) {
		result = to_date(
			 std::stoi(sYYYYMMDD.substr(0,4))
			,static_cast<unsigned>(std::stoul(sYYYYMMDD.substr(4,2)))
			,static_cast<unsigned>(std::stoul(sYYYYMMDD.substr(6,2))));
	}
	else {
		// Handle "YYYY-MM-DD" "YYYY MM DD" etc.
		std::string sDate = filtered(sYYYYMMDD,::isdigit);
		if (sDate.size()==8) result = to_date(sDate);
	}
	// if (result) std::cout << " = " << *result;
	// else std::cout << " = null";
	return result;
}

Date to_today() {
	// TODO: Upgrade to correct std::chrono way when C++20 compiler support is there
	// std::cout << "\nto_today";
	std::ostringstream os{};
	auto now_timet = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm* now_local = localtime(&now_timet);
	return to_date(1900 + now_local->tm_year,1 + now_local->tm_mon,now_local->tm_mday);	
}

class DateRange {
public:
	DateRange(Date const& begin,Date const& end) : m_begin{begin},m_end{end} {}
	DateRange(std::string const& yyyymmdd_begin,std::string const& yyyymmdd_end) {
		OptionalDate begin{to_date(yyyymmdd_begin)};
		OptionalDate end{to_date(yyyymmdd_end)};
		if (begin and end) {
			m_valid = true;
			m_begin = *begin;
			m_end = *end;
		}
	}
	Date begin() const {return m_begin;}
	Date end() const {return m_end;}
	bool contains(Date const& date) const { return begin() <= date and date <= end();}
	operator bool() const {return m_valid;}
private:
	bool m_valid{};
	Date m_begin{};
	Date m_end{};
};
using OptionalDateRange = std::optional<DateRange>;

struct Quarter {
	unsigned ix;
};

Quarter to_quarter(Date const& a_period_date) {
	return {((static_cast<unsigned>(a_period_date.month())-1) / 3u)+ 1u}; // ((0..3) + 1
}

std::chrono::month to_quarter_begin(Quarter const& quarter) {
	unsigned begin_month_no = (quarter.ix-1u) * 3u + 1u; // [0..3]*3 = [0,3,6,9] + 1 = [1,4,7,10]
	return std::chrono::month{begin_month_no};
}

std::chrono::month to_quarter_end(Quarter const& quarter) {
	return (to_quarter_begin(quarter) + std::chrono::months{2});
}

DateRange to_quarter_range(Date const& a_period_date) {
	std::cout << "\nto_quarter_range: a_period_date:" << a_period_date;
	auto quarter = to_quarter(a_period_date);
	auto begin_month = to_quarter_begin(quarter);
	auto end_month = to_quarter_end(quarter);
	auto begin = Date{a_period_date.year()/begin_month/std::chrono::day{1u}};
	auto end = Date{a_period_date.year()/end_month/std::chrono::last};
	return {begin,end};
}

DateRange to_previous_quarter(DateRange const& quarter) {
	auto const quarter_duration = std::chrono::months{3};
	return {quarter.begin() - quarter_duration,quarter.end() - quarter_duration};
}

std::ostream& operator<<(std::ostream& os,DateRange const& dr) {
	os << dr.begin() << "..." << dr.end();
	return os;
}

struct IsPeriod {
	DateRange period;
	bool operator()(Date const& date) const {
		return period.contains(date);
	}
};

IsPeriod to_is_period(DateRange const& period) {
	return {period};
}

std::optional<IsPeriod> to_is_period(std::string const& yyyymmdd_begin,std::string const& yyyymmdd_end) {
	std::optional<IsPeriod> result{};
	if (DateRange date_range{yyyymmdd_begin,yyyymmdd_end}) result = to_is_period(date_range);
	else {
		std::cerr << "\nERROR, to_is_period failed. Invalid period " << std::quoted(yyyymmdd_begin) << " ... " << std::quoted(yyyymmdd_begin);
	}
	return result;
}

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

using BASAccountNo = unsigned int;
unsigned first_digit(BASAccountNo account_no) {
	return account_no / 1000;
}
using OptionalBASAccountNo = std::optional<BASAccountNo>;

template <typename Meta,typename Defacto>
class MetaDefacto {
public:
	Meta meta;
	Defacto defacto;
private:
};

namespace BAS {

	enum class AccountType {
		// Account type is specified as T, S, K or I (asset, debt, cost or income)
		// Swedish: tillgång, skuld, kostnad eller intäkt
		Unknown
		,Asset 	// + or Debit for MORE assets
		,Debt		// - or credit for MORE debt 
		,Cost		// + or Debit for MORE cost
		,Income // - or Credit for MORE income
		,Undefined
	};

	using OptionalAccountType = std::optional<AccountType>;

	struct AccountMeta {
		std::string name{};
		OptionalAccountType account_type{};
		SRU::OptionalAccountNo sru_code{};
	};
	using AccountMetas = std::map<BASAccountNo,BAS::AccountMeta>;

	namespace detail {
		// "Hidden" Global mapping between BAS account no and its registered meta data like name, SRU code etc (from SIE file(s) imported)
		// See accessor global_account_metas().
		AccountMetas global_account_metas{};
	}

	AccountMetas const& global_account_metas() {return detail::global_account_metas;}

	namespace anonymous {

		struct AccountTransaction {
			BASAccountNo account_no;
			std::optional<std::string> transtext{};
			Amount amount;
			bool operator<(AccountTransaction const& other) const {
				bool result{false};
				if (account_no == other.account_no) {
					if (amount == other.amount) {
						result = (transtext < other.transtext);
					}
					else result = (amount < other.amount);
				}
				else result = (account_no < other.account_no);
				return result;
			}
		};
		using OptionalAccountTransaction = std::optional<AccountTransaction>;
		using AccountTransactions = std::vector<AccountTransaction>;

		struct JournalEntry {
			std::string caption{};
			Date date{};
			AccountTransactions account_transactions;
		};
		using OptionalJournalEntry = std::optional<JournalEntry>;
		using JournalEntries = std::vector<JournalEntry>;
	} // namespace anonymous

	using VerNo = unsigned int;
	using OptionalVerNo = std::optional<VerNo>;
	using Series = char;

	struct JournalEntryMeta {
		Series series;
		OptionalVerNo verno;
		std::optional<bool> unposted_flag{};
		bool operator==(JournalEntryMeta const& other) const = default;
	};
	using OptionalJournalEntryMeta = std::optional<JournalEntryMeta>;

	using MetaEntry = MetaDefacto<JournalEntryMeta,BAS::anonymous::JournalEntry>;
	using OptionalMetaEntry = std::optional<MetaEntry>;
	using MetaEntries = std::vector<MetaEntry>;

	using MetaAccountTransaction = MetaDefacto<BAS::MetaEntry,BAS::anonymous::AccountTransaction>;
	using OptionalMetaAccountTransaction = std::optional<MetaAccountTransaction>;
	using MetaAccountTransactions = std::vector<MetaAccountTransaction>;

	Amount mats_sum(BAS::MetaAccountTransactions const& mats) {
		return std::accumulate(mats.begin(),mats.end(),Amount{},[](Amount acc,BAS::MetaAccountTransaction const& mat){
			acc += mat.defacto.amount;
			return acc;
		});
	}

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

	OptionalJournalEntryMeta to_journal_meta(std::string const& s) {
		OptionalJournalEntryMeta result{};
		const std::regex meta_regex("[A-Z]\\d+"); // series followed by verification number
		if (std::regex_match(s,meta_regex)) result = JournalEntryMeta{
			.series = s[0]
			,.verno = static_cast<VerNo>(std::stoi(s.substr(1)))};
		return result;
	}

	namespace filter {
		struct is_series {
			BAS::Series required_series;
			bool operator()(MetaEntry const& me) {
				return (me.meta.series == required_series);
			}
		};

		struct is_flagged_unposted {
			bool operator()(MetaEntry const& me) {
				return (me.meta.unposted_flag and *me.meta.unposted_flag); // Rely on C++ short-circuit (https://en.cppreference.com/w/cpp/language/operator_logical)
			}
		};

		struct contains_account {
			BASAccountNo account_no;
			bool operator()(MetaEntry const& me) {
				return std::any_of(me.defacto.account_transactions.begin(),me.defacto.account_transactions.end(),[this](auto const& at){
					return (this->account_no == at.account_no);
				});
			}
		};

		struct matches_meta {
			JournalEntryMeta entry_meta;
			bool operator()(MetaEntry const& me) {
				return (me.meta == entry_meta);
			}
		};

		struct matches_amount {
			Amount amount;
			bool operator()(MetaEntry const& me) {
				return std::any_of(me.defacto.account_transactions.begin(),me.defacto.account_transactions.end(),[this](auto const& at){
					return (this->amount == at.amount);
				});
			}
		};

		struct matches_heading {
			std::string user_search_string;
			bool operator()(MetaEntry const& me) {
				bool result{false};
				result = strings_share_tokens(user_search_string,me.defacto.caption);
				if (!result) {
					result = std::any_of(me.defacto.account_transactions.begin(),me.defacto.account_transactions.end(),[this](auto const& at){
						if (at.transtext) return strings_share_tokens(user_search_string,*at.transtext);
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

namespace SKV { 
	namespace XML {
		namespace VATReturns {
			using BoxNo = unsigned int;
			using BoxNos = std::vector<BoxNo>;

			BoxNos const EU_VAT_BOX_NOS{30,31,32};
			BoxNos const EU_PURCHASE_BOX_NOS{20,21};
			BoxNos const VAT_BOX_NOS{10,11,12,30,31,32,60,61,62,48,49};

		} // namespace VATReturns 
	} // namespace XML 
} // namespace SKV 

std::set<BASAccountNo> to_vat_returns_form_bas_accounts(SKV::XML::VATReturns::BoxNos const& box_nos); // Forward (future header)
std::set<BASAccountNo> const& to_vat_accounts(); // Forward (future header)

auto is_any_of_accounts(BAS::MetaAccountTransaction const mat,BASAccountNos const& account_nos) {
	return std::any_of(account_nos.begin(),account_nos.end(),[&mat](auto other){
		return other == mat.defacto.account_no;
	});
}

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

BAS::anonymous::OptionalAccountTransaction to_bas_account_transaction(std::vector<std::string> const& ast) {
	BAS::anonymous::OptionalAccountTransaction result{};
	if (ast.size() > 1) {
			if (auto account_no = BAS::to_account_no(ast[0])) {
				switch (ast.size()) {
					case 2: {
						if (auto amount = to_amount(ast[1])) {
							result = BAS::anonymous::AccountTransaction{.account_no=*account_no,.transtext=std::nullopt,.amount=*amount};
						}
					} break;
					case 3: {
						if (auto amount = to_amount(ast[2])) {
							result = BAS::anonymous::AccountTransaction{.account_no=*account_no,.transtext=ast[1],.amount=*amount};
						}
					} break;
					default:;
				}
			}
	}
	return result;
}

std::ostream& operator<<(std::ostream& os,BAS::anonymous::AccountTransaction const& at) {
	if (BAS::global_account_metas().contains(at.account_no)) os << std::quoted(BAS::global_account_metas().at(at.account_no).name) << ":";
	os << at.account_no;
	os << " " << at.transtext;
	os << " " << at.amount;
	return os;
};

std::string to_string(BAS::anonymous::AccountTransaction const& at) {
	std::ostringstream os{};
	os << at;
	return os.str();
};

std::ostream& operator<<(std::ostream& os,BAS::anonymous::JournalEntry const& aje) {
	os << std::quoted(aje.caption) << " " << aje.date;
	for (auto const& at : aje.account_transactions) {
		os << "\n\t" << to_string(at); 
	}
	return os;
};

std::ostream& operator<<(std::ostream& os,BAS::OptionalVerNo const& verno) {
	if (verno and *verno!=0) os << *verno;
	else os << " _ ";
	return os;
}

std::ostream& operator<<(std::ostream& os,std::optional<bool> flag) {
	auto ch = (flag)?((*flag)?'*':' '):' '; // '*' if known and set, else ' '
	os << ch;
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::JournalEntryMeta const& jem) {
	os << jem.unposted_flag << jem.series << jem.verno;
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::MetaAccountTransaction const& mat) {
	os << mat.meta.meta << " " << mat.defacto;
	return os;
};

std::ostream& operator<<(std::ostream& os,BAS::MetaEntry const& me) {
	os << me.meta << " " << me.defacto;
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::MetaEntries const& mes) {
	for (auto const& me : mes) {
		os << "\n" << me;
	}
	return os;
};

std::string to_string(BAS::anonymous::JournalEntry const& aje) {
	std::ostringstream os{};
	os << aje;
	return os.str();
};

std::string to_string(BAS::MetaEntry const& me) {
	std::ostringstream os{};
	os << me;
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
	Date date{};
	BAS::OptionalMetaEntry current_candidate{};
};

std::ostream& operator<<(std::ostream& os,HeadingAmountDateTransEntry const& had) {
	os << had.heading;
	os << " " << had.amount;
	os << " " << to_string(had.date); 
	return os;
}

using OptionalHeadingAmountDateTransEntry = std::optional<HeadingAmountDateTransEntry>;

using HeadingAmountDateTransEntries = std::vector<HeadingAmountDateTransEntry>;

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

namespace Key {
		class Path {
		public:
			auto begin() const {return m_path.begin();}
			auto end() const {return m_path.end();}
			Path() = default;
			Path(Path const& other) = default;
			Path(std::string const& s_path,char delim = '^') : 
			  m_delim{delim}
				,m_path(tokenize::splits(s_path,delim)) {};
			auto size() const {return m_path.size();}
			Path operator+(std::string const& key) const {Path result{*this};result.m_path.push_back(key);return result;}
			operator std::string() const {
				std::ostringstream os{};
				os << *this;
				return os.str();
			}
			Path& operator+=(std::string const& key) {
				m_path.push_back(key);
				// std::cout << "\noperator+= :" << *this  << " size:" << this->size();
				return *this;
			}
			Path& operator--() {
				m_path.pop_back();
				// std::cout << "\noperator-- :" << *this << " size:" << this->size();
				return *this;
			}
			Path parent() {
				Path result{*this};
				--result;
				// std::cout << "\nparent:" << result << " size:" << result.size();
				return result;
			}
			std::string back() const {return m_path.back();}
			std::string operator[](std::size_t pos) const {return m_path[pos];}
			friend std::ostream& operator<<(std::ostream& os,Path const& key_path);
			// bool operator==(Path const& other) const {
			// 	bool result = (this->m_path == other.m_path);
			// 	std::cout << "\n1:" << *this << " size:" << this->size();
			// 	std::cout << "\n2:" << other << " size:" << other.size();
			// 	std::cout << "\n============";
			// 	if (result) std::cout << "IS EQUEAL";
			// 	else std::cout << "differs";
			// 	return result;
			// }
		private:
			std::vector<std::string> m_path{};
			char m_delim{'^'};
		};

		using Paths = std::vector<Path>;

		std::ostream& operator<<(std::ostream& os,Key::Path const& key_path) {
			int key_count{0};
			for (auto const& key : key_path) {
				if (key_count++>0) os << key_path.m_delim;
				os << key;
				// std::cout << "\n\t[" << key_count-1 << "]:" << std::quoted(key);
			}
			return os;
		}
} // namespace Key

using char16_t_string = std::wstring;

namespace CP435 {
	extern std::map<char,char16_t> cp435ToUnicodeMap;

	char16_t cp435ToUnicode(char ch435) {
		return cp435ToUnicodeMap[ch435];
	}

	char16_t_string cp435ToUnicode(std::string s435) {
		char16_t_string result{};
		std::transform(s435.begin(),s435.end(),std::back_inserter(result),[](char ch){
			return CP435::cp435ToUnicode(ch);
		});
		return result;
	}
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

	std::string unicode_to_utf8(char16_t_string const& s) {
		std::ostringstream os{};
		UTF8::Stream utf8_os{os};
		for (auto cp : s) utf8_os << cp;
		return os.str();
	}
} // namespace UTF8

namespace SIE {

	using Stream = std::istream;

	struct Tag {
		std::string const expected;
	};

	// #ORGNR CIN 
	struct OrgNr {
		std::string tag;
		std::string CIN;
		// acq_no 
		// act_no
	};

	// #FNAMN company name
	struct FNamn {
		std::string tag;
		std::string company_name;
	};

	// #ADRESS contact distribution address postal address tel
	struct Adress {
		std::string tag;
		std::string contact;
		std::string distribution_address;
		std::string postal_address;
		std::string tel;
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
		std::optional<Date> transdate{};
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
		Date verdate;
		std::string vertext;
		std::optional<Date> regdate{};
		std::optional<std::string> sign{};
		std::vector<Trans> transactions{};
	};
	struct AnonymousLine {};
	using SIEFileEntry = std::variant<OrgNr,FNamn,Adress,Konto,Sru,Ver,Trans,AnonymousLine>;
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
		std::optional<Date>& date;
	};
	struct YYYYMMdd_parser {
		Date& date;
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
			std::optional<Date> od{};
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

	SIEParseResult parse_ORGNR(Stream& in,std::string const& konto_tag) {
		SIEParseResult result{};
		// #ORGNR CIN 
		// struct OrgNr {
		// 	std::string tag;
		// 	std::string CIN;
		// 	// acq_no 
		// 	// act_no
		// };
		SIE::OrgNr orgnr{};
		auto pos = in.tellg();
		if (in >> Tag{konto_tag} >> orgnr.CIN) {
			result = orgnr;
			pos = in.tellg(); // accept new stream position
		}
		in.seekg(pos); // Reset position in case of failed parse
		in.clear(); // Reset failbit to allow try for other parse

		return result;
	}

	SIEParseResult parse_FNAMN(Stream& in,std::string const& konto_tag) {
		SIEParseResult result{};
		// #FNAMN company name
		// struct FNamn {
		// 	std::string tag;
		// 	std::string company_name;
		// };
		SIE::FNamn fnamn{};
		auto pos = in.tellg();
		if (in >> Tag{konto_tag} >> std::quoted(fnamn.company_name)) {
			result = fnamn;
			pos = in.tellg(); // accept new stream position
		}
		in.seekg(pos); // Reset position in case of failed parse
		in.clear(); // Reset failbit to allow try for other parse
		return result;
	}

	SIEParseResult parse_ADRESS(Stream& in,std::string const& konto_tag) {
		SIEParseResult result{};
		// #ADRESS contact distribution address postal address tel
		// struct Adress {
		// 	std::string tag;
		// 	std::string contact;
		// 	std::string distribution_address;
		// 	std::string postal_address;
		// 	std::string tel;
		// };
		SIE::Adress adress{};
		auto pos = in.tellg();
		if (in >> Tag{konto_tag} >> std::quoted(adress.contact) >> std::quoted(adress.distribution_address) >> std::quoted(adress.postal_address) >> std::quoted(adress.tel)) {
			result = adress;
			pos = in.tellg(); // accept new stream position
		}
		in.seekg(pos); // Reset position in case of failed parse
		in.clear(); // Reset failbit to allow try for other parse
		return result;
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
			auto sUnicode = CP435::cp435ToUnicode(s437);
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
				auto sUnicode = CP435::cp435ToUnicode(s437);
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

	SIEParseResult parse_VER(Stream& in) {
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
			auto sUnicode = CP435::cp435ToUnicode(s437);
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

SIE::Trans to_sie_t(BAS::anonymous::AccountTransaction const& trans) {
	SIE::Trans result{
		.account_no = trans.account_no
		,.amount = trans.amount
		,.transtext = trans.transtext
	};
	return result;
}

SIE::Ver to_sie_t(BAS::MetaEntry const& me) {
		/*
		Series series;
		BAS::VerNo verno;
		Date verdate;
		std::string vertext;
		*/

	SIE::Ver result{
		.series = me.meta.series
		,.verno = (me.meta.verno)?*me.meta.verno:0
		,.verdate = me.defacto.date
		,.vertext = me.defacto.caption};
	for (auto const& trans : me.defacto.account_transactions) {
		result.transactions.push_back(to_sie_t(trans));
	}
	return result;
}

bool is_vat_returns_form_at(std::vector<SKV::XML::VATReturns::BoxNo> const& box_nos,BAS::anonymous::AccountTransaction const& at) {
	auto const& bas_account_nos = to_vat_returns_form_bas_accounts(box_nos);
	return bas_account_nos.contains(at.account_no);
}

bool is_vat_account(BASAccountNo account_no) {
	auto const& vat_accounts = to_vat_accounts();
	return vat_accounts.contains(account_no);
}

auto is_vat_account_at = [](BAS::anonymous::AccountTransaction const& at){
	return is_vat_account(at.account_no);
};

Amount to_positive_gross_transaction_amount(BAS::anonymous::JournalEntry const& aje) {
	Amount result = std::accumulate(aje.account_transactions.begin(),aje.account_transactions.end(),Amount{},[](Amount acc,BAS::anonymous::AccountTransaction const& account_transaction){
		acc += (account_transaction.amount>0)?account_transaction.amount:0;
		return acc;
	});
	return result;
}

BAS::anonymous::OptionalAccountTransaction gross_account_transaction(BAS::anonymous::JournalEntry const& aje) {
	BAS::anonymous::OptionalAccountTransaction result{};
	auto trans_amount = to_positive_gross_transaction_amount(aje);
	auto iter = std::find_if(aje.account_transactions.begin(),aje.account_transactions.end(),[&trans_amount](auto const& at){
		return std::abs(at.amount) == trans_amount;
	});
	if (iter != aje.account_transactions.end()) result = *iter;
	return result;
}

Amount to_account_transactions_sum(BAS::anonymous::AccountTransactions const& ats) {
	Amount result = std::accumulate(ats.begin(),ats.end(),Amount{},[](Amount acc,BAS::anonymous::AccountTransaction const& at){
		acc += at.amount;
		return acc;
	});
	return result;
}

bool have_opposite_signs(Amount a1,Amount a2) {
	return ((a1 > 0) and (a2 < 0)) or ((a1 < 0) and (a2 > 0)); // Note: false also for a1==a2==0
}

BAS::anonymous::AccountTransactions counter_account_transactions(BAS::anonymous::JournalEntry const& aje,BAS::anonymous::AccountTransaction const& gross_at) {
	BAS::anonymous::AccountTransactions result{};
	// Gather all ats with opposite sign and that sums upp to gross_at amount
	std::copy_if(aje.account_transactions.begin(),aje.account_transactions.end(),std::back_inserter(result),[&gross_at](auto const& at){
		return (have_opposite_signs(at.amount,gross_at.amount));
	});
	if (to_account_transactions_sum(result) != -gross_at.amount) result.clear();
	return result;
}

BAS::anonymous::OptionalAccountTransaction net_account_transaction(BAS::anonymous::JournalEntry const& aje) {
	BAS::anonymous::OptionalAccountTransaction result{};
	auto trans_amount = to_positive_gross_transaction_amount(aje);
	auto iter = std::find_if(aje.account_transactions.begin(),aje.account_transactions.end(),[&trans_amount](auto const& at){
		return (std::abs(at.amount) < trans_amount and not is_vat_account_at(at));
		// return std::abs(at.amount) == 0.8*trans_amount;
	});
	if (iter != aje.account_transactions.end()) result = *iter;
	return result;
}

BAS::anonymous::OptionalAccountTransaction vat_account_transaction(BAS::anonymous::JournalEntry const& aje) {
	BAS::anonymous::OptionalAccountTransaction result{};
	auto trans_amount = to_positive_gross_transaction_amount(aje);
	auto iter = std::find_if(aje.account_transactions.begin(),aje.account_transactions.end(),[&trans_amount](auto const& at){
		return is_vat_account_at(at);
		// return std::abs(at.amount) == 0.2*trans_amount;
	});
	if (iter != aje.account_transactions.end()) result = *iter;
	return result;
}

struct AccountTransactionTemplate {
	AccountTransactionTemplate(BASAccountNo account_no,Amount gross_amount,Amount account_amount) 
		:  m_account_no{account_no}
			,m_percent{static_cast<int>(std::round(account_amount*100 / gross_amount))}  {}
	BASAccountNo m_account_no;
	int m_percent;
	BAS::anonymous::AccountTransaction operator()(Amount amount) const {
		// BAS::anonymous::AccountTransaction result{.account_no = m_account_no,.transtext="",.amount=amount*m_factor};
		BAS::anonymous::AccountTransaction result{
			 .account_no = m_account_no
			,.transtext=""
			,.amount=static_cast<Amount>(std::round(amount*m_percent)/100.0)};
		return result;
	}
};
using AccountTransactionTemplates = std::vector<AccountTransactionTemplate>;

class JournalEntryTemplate {
public:

	JournalEntryTemplate(BAS::Series series,BAS::MetaEntry const& me) : m_series{series} {
		auto gross_amount = to_positive_gross_transaction_amount(me.defacto);
		if (gross_amount >= 0.01) {
			std::transform(me.defacto.account_transactions.begin(),me.defacto.account_transactions.end(),std::back_inserter(templates),[gross_amount](BAS::anonymous::AccountTransaction const& account_transaction){
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

	BAS::anonymous::AccountTransactions operator()(Amount amount) const {
		BAS::anonymous::AccountTransactions result{};
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

OptionalJournalEntryTemplate to_template(BAS::MetaEntry const& me) {
	OptionalJournalEntryTemplate result({me.meta.series,me});
	return result;
}

BAS::MetaEntry to_journal_entry(HeadingAmountDateTransEntry const& had,JournalEntryTemplate const& jet) {
	BAS::MetaEntry result{};
	result.meta = {
		.series = jet.series()
	};
	result.defacto.caption = had.heading;
	result.defacto.date = had.date;
	result.defacto.account_transactions = jet(std::abs(had.amount)); // Ignore sign to have template apply its sign
	return result;
}

std::ostream& operator<<(std::ostream& os,JournalEntryTemplate const& entry) {
	os << "template: series " << entry.series();
	std::for_each(entry.templates.begin(),entry.templates.end(),[&os](AccountTransactionTemplate const& t){
		os << "\n\t" << t.m_account_no << " " << t.m_percent;
	});
	return os;
}

bool had_matches_trans(HeadingAmountDateTransEntry const& had,BAS::anonymous::JournalEntry const& aje) {
	return strings_share_tokens(had.heading,aje.caption);
}

BAS::MetaEntries template_candidates(BASJournals const& journals,auto const& matches) {
	// std::cout << "\ntemplate_candidates";
	BAS::MetaEntries result{};
	for (auto const& je : journals) {
		auto const& [series,journal] = je;
		for (auto const& [verno,entry] : journal) {								
			if (matches(entry)) result.push_back({{.series = series,.verno = verno},entry});
		}
	}
	std::sort(result.begin(),result.end(),[](auto const& je1,auto const& je2){
		return (je1.defacto.date < je2.defacto.date);
	});
	return result;
}

// ==================================================================================

bool same_whole_units(Amount const& a1,Amount const& a2) {
	return (std::round(a1) == std::round(a2));
}

BAS::MetaEntry updated_entry(BAS::MetaEntry const& me,BAS::anonymous::AccountTransaction const& at) {
	BAS::MetaEntry result{me};
	std::sort(result.defacto.account_transactions.begin(),result.defacto.account_transactions.end(),[](auto const& e1,auto const& e2){
		return (std::abs(e1.amount) > std::abs(e2.amount)); // greater to lesser
	});
	auto iter = std::find_if(result.defacto.account_transactions.begin(),result.defacto.account_transactions.end(),[&at](auto const& entry){
		return (entry.account_no == at.account_no);
	});
	if (iter == result.defacto.account_transactions.end()) {
		result.defacto.account_transactions.push_back(at);
		return updated_entry(result,at); // recurse with added entry
	}
	else if (me.defacto.account_transactions.size()==4) {
		// std::cout << "\n4 OK";
		// Assume 0: Transaction Amount, 1: Amount no VAT, 3: VAT, 4: rounding amount
		auto& trans_amount = result.defacto.account_transactions[0].amount;
		auto& ex_vat_amount = result.defacto.account_transactions[1].amount;
		auto& vat_amount = result.defacto.account_transactions[2].amount;
		auto& round_amount = result.defacto.account_transactions[3].amount;

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
				switch (std::distance(result.defacto.account_transactions.begin(),iter)) {
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
	SIE::OrgNr organisation_no{};
	SIE::FNamn organisation_name{};
	SIE::Adress organisation_address{};

	BASJournals& journals() {return m_journals;}
	BASJournals const& journals() const {return m_journals;}
	bool is_unposted(BAS::Series series, BAS::VerNo verno) const {
		bool result{true};
		if (verno_of_last_posted_to.contains(series)) result = verno > this->verno_of_last_posted_to.at(series);
		return result;
	}
	void post(BAS::MetaEntry const& me) {
		if (me.meta.verno) {
			m_journals[me.meta.series][*me.meta.verno] = me.defacto;
			verno_of_last_posted_to[me.meta.series] = *me.meta.verno;
		}
		else {
			std::cerr << "\nSIEEnvironment::post failed - can't post an entry with null verno";
		}
	}
	std::optional<BAS::MetaEntry> stage(BAS::MetaEntry const& entry) {
		std::optional<BAS::MetaEntry> result{};
		if (this->already_in_posted(entry) == false) result = this->add(entry);
		return result;
	}

	BAS::MetaEntries stage(SIEEnvironment const& staged_sie_environment) {
		BAS::MetaEntries result{};
		for (auto const& [series,journal] : staged_sie_environment.journals()) {
			for (auto const& [verno,aje] : journal) {
				auto je = this->stage({{.series=series,.verno=verno},aje});
				if (!je) {
					result.push_back({{.series=series,.verno=verno},aje}); // no longer staged
				}
			}
		}
		return result;
	}
	BAS::MetaEntries unposted() const {
		// std::cout << "\nunposted()";
		BAS::MetaEntries result{};
		for (auto const& [series,verno] : this->verno_of_last_posted_to) {
			// std::cout << "\n\tseries:" << series << " verno:" << verno;
			auto last = m_journals.at(series).find(verno);
			for (auto iter = ++last;iter!=m_journals.at(series).end();++iter) {
				// std::cout << "\n\tunposted:" << iter->first;
				BAS::MetaEntry bjer{
					.meta = {
						.series = series
						,.verno = iter->first
					}
					,.defacto = iter->second
				};
				result.push_back(bjer);
			}
		}
		return result;
	}

	BAS::AccountMetas const & account_metas() const {return BAS::detail::global_account_metas;} // const ref global instance

	void set_account_name(BASAccountNo bas_account_no ,std::string const& name) {
		if (BAS::detail::global_account_metas.contains(bas_account_no)) {
			if (BAS::detail::global_account_metas[bas_account_no].name != name) {
				std::cerr << "\nWARNING: BAS Account " << bas_account_no << " name " << std::quoted(BAS::detail::global_account_metas[bas_account_no].name) << " changed to " << std::quoted(name);
			}
		}
		BAS::detail::global_account_metas[bas_account_no].name = name; // Mutate global instance
	}
	void set_account_SRU(BASAccountNo bas_account_no, SRU::AccountNo sru_code) {
		if (BAS::detail::global_account_metas.contains(bas_account_no)) {
			if (BAS::detail::global_account_metas[bas_account_no].sru_code) {
				if (*BAS::detail::global_account_metas[bas_account_no].sru_code != sru_code) {
					std::cerr << "\nWARNING: BAS Account " << bas_account_no << " SRU Code " << *BAS::detail::global_account_metas[bas_account_no].sru_code << " changed to " << sru_code;
				}
			}
		}
		BAS::detail::global_account_metas[bas_account_no].sru_code = sru_code; // Mutate global instance
	}

private:
	BASJournals m_journals{};
	std::map<char,BAS::VerNo> verno_of_last_posted_to{};
	BAS::MetaEntry add(BAS::MetaEntry const& me) {
		BAS::MetaEntry result{me};
		// Assign "actual" sequence number
		auto verno = largest_verno(me.meta.series) + 1;
		result.meta.verno = verno;
		m_journals[me.meta.series][verno] = me.defacto;
		return result;
	}
	BAS::VerNo largest_verno(BAS::Series series) {
		auto const& journal = m_journals[series];
		return std::accumulate(journal.begin(),journal.end(),unsigned{},[](auto acc,auto const& entry){
			return (acc<entry.first)?entry.first:acc;
		});
	}
	bool already_in_posted(BAS::MetaEntry const& me) {
		bool result{false};
		if (me.meta.verno and *me.meta.verno > 0) {
			auto journal_iter = m_journals.find(me.meta.series);
			if (journal_iter != m_journals.end()) {
				if (me.meta.verno) {
					auto entry_iter = journal_iter->second.find(*me.meta.verno);
					result = (entry_iter != journal_iter->second.end());
				}
			}
		}
		return result;
	}
};

using OptionalSIEEnvironment = std::optional<SIEEnvironment>;
using SIEEnvironments = std::map<std::string,SIEEnvironment>;

void for_each_anonymous_journal_entry(SIEEnvironment const& sie_env,auto& f) {
	for (auto const& [journal_id,journal] : sie_env.journals()) {
		for (auto const& [verno,aje] : journal) {
			f(aje);
		}
	}
}

void for_each_anonymous_journal_entry(SIEEnvironments const& sie_envs,auto& f) {
	for (auto const& [year_id,sie_env] : sie_envs) {
		for_each_anonymous_journal_entry(sie_env,f);
	}
}

void for_each_meta_entry(SIEEnvironment const& sie_env,auto& f) {
	for (auto const& [series,journal] : sie_env.journals()) {
		for (auto const& [verno,aje] : journal) {
			f(BAS::MetaEntry{.meta ={.series=series,.verno=verno,.unposted_flag=sie_env.is_unposted(series,verno)},.defacto=aje});
		}
	}
}

void for_each_meta_entry(SIEEnvironments const& sie_envs,auto& f) {
	for (auto const& [year_id,sie_env] : sie_envs) {
		for_each_meta_entry(sie_env,f);
	}
}

void for_each_anonymous_account_transaction(BAS::anonymous::JournalEntry const& aje,auto& f) {
	for (auto const& at : aje.account_transactions) {
		f(at);
	}
}

void for_each_anonymous_account_transaction(SIEEnvironment const& sie_env,auto& f) {
	auto f_caller = [&f](BAS::anonymous::JournalEntry const& aje){for_each_anonymous_account_transaction(aje,f);};
	for_each_anonymous_journal_entry(sie_env,f_caller);
}

void for_each_meta_account_transaction(BAS::MetaEntry const& me,auto& f) {
	for (auto const& at : me.defacto.account_transactions) {
		f(BAS::MetaAccountTransaction{
			.meta = me
			,.defacto = at
		});
	}
}

void for_each_meta_account_transaction(SIEEnvironment const& sie_env,auto& f) {
	auto f_caller = [&f](BAS::MetaEntry const& mee){for_each_meta_account_transaction(mee,f);};
	for_each_meta_entry(sie_env,f_caller);
}

void for_each_meta_account_transaction(SIEEnvironments const& sie_envs,auto& f) {
	auto f_caller = [&f](BAS::MetaEntry const& me){for_each_meta_account_transaction(me,f);};
	for (auto const& [year_id,sie_env] : sie_envs) {
		for_each_meta_entry(sie_env,f_caller);
	}
}

struct GrossAccountTransactions {
	BAS::anonymous::AccountTransactions result;
	void operator()(BAS::anonymous::JournalEntry const& aje) {
		if (auto at = gross_account_transaction(aje)) {
			result.push_back(*at);
		}
	}
};

struct NetAccountTransactions {
	BAS::anonymous::AccountTransactions result;
	void operator()(BAS::anonymous::JournalEntry const& aje) {
		if (auto at = net_account_transaction(aje)) {
			result.push_back(*at);
		}
	}
};

struct VatAccountTransactions {
	BAS::anonymous::AccountTransactions result;
	void operator()(BAS::anonymous::JournalEntry const& aje) {
		if (auto at = vat_account_transaction(aje)) {
			result.push_back(*at);
		}
	}
};

BAS::anonymous::AccountTransactions to_gross_account_transactions(BAS::anonymous::JournalEntry const& aje) {
	GrossAccountTransactions ats{};
	ats(aje);
	return ats.result;
}

BAS::anonymous::AccountTransactions to_gross_account_transactions(SIEEnvironments const& sie_envs) {
	GrossAccountTransactions ats{};
	for_each_anonymous_journal_entry(sie_envs,ats);
	return ats.result;
}

BAS::anonymous::AccountTransactions to_net_account_transactions(SIEEnvironments const& sie_envs) {
	NetAccountTransactions ats{};
	for_each_anonymous_journal_entry(sie_envs,ats);
	return ats.result;
}

BAS::anonymous::AccountTransactions to_vat_account_transactions(SIEEnvironments const& sie_envs) {
	VatAccountTransactions ats{};
	for_each_anonymous_journal_entry(sie_envs,ats);
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
	os << t2e.first.meta;
	os << " <- " << t2e.second.linking_account << " ->";
	os << " " << t2e.second.me.meta;
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
				auto at_iter1 = std::find_if(me.defacto.account_transactions.begin(),me.defacto.account_transactions.end(),[&t2_iter](BAS::anonymous::AccountTransaction const& at1){
					auto  at_iter2 = std::find_if(t2_iter->me.defacto.account_transactions.begin(),t2_iter->me.defacto.account_transactions.end(),[&at1](BAS::anonymous::AccountTransaction const& at2){
						return (at1.account_no == at2.account_no) and (at1.amount == -at2.amount);
					});
					return (at_iter2 != t2_iter->me.defacto.account_transactions.end());
				});
				if (at_iter1 != me.defacto.account_transactions.end()) {
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

// SKV Electronic API (file formats for upload)

namespace SKV {

	int to_tax(Amount amount) {return std::round(amount);}
	int to_fee(Amount amount) {return std::round(amount);}

	OptionalDateRange to_date_range(std::string const& period_id) {
		OptionalDateRange result{};
		try {
			auto today = to_today();
			const std::regex is_year_quarter("^\\s*\\d\\d-Q[1-4]\\s*$"); // YY-Q1..YY-Q4
			const std::regex is_anonymous_quarter("Q[1-4]"); // Q1..Q4
			if (period_id.size()==0) {
				// default to current quarter
				result = to_quarter_range(today);
			}
			else if (std::regex_match(period_id,is_year_quarter)) {
				// Create quarter range of given year YY-Qx
				auto current_century = static_cast<int>(today.year()) / 100;
				std::chrono::year year{current_century*100 + std::stoi(period_id.substr(0,2))};
				std::chrono::month end_month{3u * static_cast<unsigned>(period_id[4] - '0')};
				result = to_quarter_range(year/end_month/std::chrono::last);
			}
			else if (std::regex_match(period_id,is_anonymous_quarter)) {
				// Create quarter range of current year Qx
				std::chrono::month end_month{3u * static_cast<unsigned>(period_id[1]-'0')};
				result = to_quarter_range(today.year()/end_month/std::chrono::last);
			}
			else throw std::runtime_error(std::string{"Can't interpret period_id=\""} + period_id + "\"");
		}
		catch (std::exception const& e) {
			std::cerr << "\nERROR, to_date_range failed. Exception = " << std::quoted(e.what());
		}
		return result;
	}

	struct ContactPersonMeta {
		std::string name{};
		std::string phone{};
		std::string e_mail{};
	};
	using OptionalContactPersonMeta = std::optional<ContactPersonMeta>;

	struct OrganisationMeta {
		std::string org_no{};
		std::vector<ContactPersonMeta> contact_persons{};
	};
	using OptionalOrganisationMeta = std::optional<OrganisationMeta>;

	namespace XML {

		struct DeclarationMeta {
			std::string creation_date_and_time{}; // e.g.,2021-01-30T07:42:25
			std::string declaration_period_id{}; // e.g., 202101
		};
		using OptionalDeclarationMeta = std::optional<DeclarationMeta>;

		struct TaxDeclaration {
			std::string employee_12_digit_birth_no{}; // e.g., 198202252386
			Amount paid_employer_fee{};
			Amount paid_tax{};
		};

		using TaxDeclarations = std::vector<TaxDeclaration>;

		using XMLMap = std::map<std::string,std::string>;
		extern SKV::XML::XMLMap skv_xml_template; // See bottom of this file

		std::string to_12_digit_orgno(std::string generic_org_no) {
			std::string result{};
			// The SKV XML IT-system requires 12 digit organisation numbers with digits only
			// E.g., SIE-file organisation XXXXXX-YYYY has to be transformed into 16XXXXXXYYYY
			// See https://sv.wikipedia.org/wiki/Organisationsnummer
			std::string sdigits = filtered(generic_org_no,::isdigit);
			switch (sdigits.size()) {
				case 10: result = std::string{"16"} + sdigits; break;
				case 12: result = sdigits; break;
				default: throw std::runtime_error(std::string{"ERROR: to_12_digit_orgno failed, invalid input generic_org_no:"} + "\"" + generic_org_no + "\""); break;
			}
			return result;
		}

		struct TagValuePair {
			std::string tag{};
			std::string value;
		};

		std::string to_value(XMLMap const& xml_map,std::string tag) {
			if (xml_map.contains(tag) and xml_map.at(tag).size() > 0) {
				return xml_map.at(tag);
			}
			else throw std::runtime_error(std::string{"to_string failed, no value for tag:"} + tag);
		}

		XMLMap::value_type to_entry(XMLMap const& xml_map,std::string tag) {
			auto iter = xml_map.find(tag);
			if (iter != xml_map.end()) {
				return *iter;
			}
			else throw std::runtime_error(std::string{"to_entry failed, tag:"} + tag + " not defined");
		}
		
		struct EmployerDeclarationOStream {
			std::ostream& os;
		};

		EmployerDeclarationOStream& operator<<(EmployerDeclarationOStream& edos,std::string const& s) {
			auto& os = edos.os;
			os << s;
			return edos;
		}

		EmployerDeclarationOStream& operator<<(EmployerDeclarationOStream& edos,XMLMap::value_type const& entry) {
			Key::Path p{entry.first};
			std::string indent(p.size(),' ');
			edos << indent << "<" << p.back() << ">" << entry.second << "</" << p.back() << ">";
			return edos;
		}

		EmployerDeclarationOStream& operator<<(EmployerDeclarationOStream& edos,XMLMap const& xml_map) {
			try {
				Key::Path p{};
				// IMPORTANT: No empty line (nor any white space) allowed before the "<?xml..." tag! *sigh*
				edos << R"(<?xml version="1.0" encoding="UTF-8" standalone="no"?>)";
				edos << "\n" << R"(<Skatteverket omrade="Arbetsgivardeklaration")";
					p += "Skatteverket";
					edos << "\n" << R"(xmlns="http://xmls.skatteverket.se/se/skatteverket/da/instans/schema/1.1")";
					edos << "\n" << R"(xmlns:agd="http://xmls.skatteverket.se/se/skatteverket/da/komponent/schema/1.1")";
					edos << "\n" << R"(xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://xmls.skatteverket.se/se/skatteverket/da/instans/schema/1.1 http://xmls.skatteverket.se/se/skatteverket/da/arbetsgivardeklaration/arbetsgivardeklaration_1.1.xsd">)";

					edos << "\n" << R"(<agd:Avsandare>)";
						p += "agd:Avsandare";
						edos << "\n" << to_entry(xml_map,p + "agd:Programnamn");
						edos << "\n" << to_entry(xml_map,p + "agd:Organisationsnummer");
						edos << "\n" << R"(<agd:TekniskKontaktperson>)";
							p += "agd:TekniskKontaktperson";
							edos << "\n" << to_entry(xml_map,p + "agd:Namn");
							edos << "\n" << to_entry(xml_map,p + "agd:Telefon");
							edos << "\n" << to_entry(xml_map,p + "agd:Epostadress");
							--p;
						edos << "\n" << R"(</agd:TekniskKontaktperson>)";
						edos << "\n" << to_entry(xml_map,p + "agd:Skapad");
						--p;
					edos << "\n" << R"(</agd:Avsandare>)";

					edos << "\n" << R"(<agd:Blankettgemensamt>)";
						p += "agd:Blankettgemensamt";
						edos << "\n" << R"(<agd:Arbetsgivare>)";
							p += "agd:Arbetsgivare";
							edos << "\n" << to_entry(xml_map,p + "agd:AgRegistreradId");
							edos << "\n" << R"(<agd:Kontaktperson>)";
								p += "agd:Kontaktperson";
								edos << "\n" << to_entry(xml_map,p + "agd:Namn");
								edos << "\n" << to_entry(xml_map,p + "agd:Telefon");
								edos << "\n" << to_entry(xml_map,p + "agd:Epostadress");
								--p;
							edos << "\n" << R"(</agd:Kontaktperson>)";
							--p;
						edos << "\n" << R"(</agd:Arbetsgivare>)";
						--p;
					edos << "\n" << R"(</agd:Blankettgemensamt>)";

					edos << "\n" << R"(<!-- Uppgift 1 HU -->)";
					edos << "\n" << R"(<agd:Blankett>)";
						p += "agd:Blankett";
						edos << "\n" << R"(<agd:Arendeinformation>)";
							p += "agd:Arendeinformation";
							edos << "\n" << to_entry(xml_map,p + "agd:Arendeagare");
							edos << "\n" << to_entry(xml_map,p + "agd:Period");
							--p;
						edos << "\n" << R"(</agd:Arendeinformation>)";
						edos << "\n" << R"(<agd:Blankettinnehall>)";
							p += "agd:Blankettinnehall";
							edos << "\n" << R"(<agd:HU>)";
								p += "agd:HU";
								edos << "\n" << R"(<agd:ArbetsgivareHUGROUP>)";
									p += "agd:ArbetsgivareHUGROUP";
									edos << "\n" << R"(<agd:AgRegistreradId faltkod="201">)" << to_value(xml_map,p + R"(agd:AgRegistreradId faltkod="201")") << "</agd:AgRegistreradId>";
									--p;
								edos << "\n" << R"(</agd:ArbetsgivareHUGROUP>)";
								edos << "\n" << R"(<agd:RedovisningsPeriod faltkod="006">)" << to_value(xml_map,p + R"(agd:RedovisningsPeriod faltkod="006")") << "</agd:RedovisningsPeriod>";
								edos << "\n" << R"(<agd:SummaArbAvgSlf faltkod="487">)" <<  to_value(xml_map,p + R"(agd:SummaArbAvgSlf faltkod="487")") << "</agd:SummaArbAvgSlf>";
								edos << "\n" << R"(<agd:SummaSkatteavdr faltkod="497">)" <<  to_value(xml_map,p + R"(agd:SummaSkatteavdr faltkod="497")") << "</agd:SummaSkatteavdr>";
								--p;
							edos << "\n" << R"(</agd:HU>)";
							--p;
						edos << "\n" << R"(</agd:Blankettinnehall>)";
						--p;
					edos << "\n" << R"(</agd:Blankett>)";

					edos << "\n" << R"(<!-- Uppgift 1 IU -->)";
					edos << "\n" << R"(<agd:Blankett>)";
						p += "agd:Blankett";
						edos << "\n" << R"(<agd:Arendeinformation>)";
							p += "agd:Arendeinformation";
							edos << "\n" << to_entry(xml_map,p + "agd:Arendeagare");
							edos << "\n" << to_entry(xml_map,p + "agd:Period");
							--p;
						edos << "\n" << R"(</agd:Arendeinformation>)";
						edos << "\n" << R"(<agd:Blankettinnehall>)";
							p += "agd:Blankettinnehall";
							edos << "\n" << R"(<agd:IU>)";
								p += "agd:IU";
								edos << "\n" << R"(<agd:ArbetsgivareIUGROUP>)";
									p += "agd:ArbetsgivareIUGROUP";
									edos << "\n" << R"(<agd:AgRegistreradId faltkod="201">)" << to_value(xml_map,p + R"(agd:AgRegistreradId faltkod="201")") << "</agd:AgRegistreradId>";
									--p;
								edos << "\n" << R"(</agd:ArbetsgivareIUGROUP>)";
								edos << "\n" << R"(<agd:BetalningsmottagareIUGROUP>)";
									p += "agd:BetalningsmottagareIUGROUP";
									edos << "\n" << R"(<agd:BetalningsmottagareIDChoice>)";
										p += "agd:BetalningsmottagareIDChoice";
										edos << "\n" << R"(<agd:BetalningsmottagarId faltkod="215">)" <<  to_value(xml_map,p + R"(agd:BetalningsmottagarId faltkod="215")") <<  "</agd:BetalningsmottagarId>";
										--p;
									edos << "\n" << R"(</agd:BetalningsmottagareIDChoice>)";
									--p;
								edos << "\n" << R"(</agd:BetalningsmottagareIUGROUP>)";
								edos << "\n" << R"(<agd:RedovisningsPeriod faltkod="006">)" <<  to_value(xml_map,p + R"(agd:RedovisningsPeriod faltkod="006")") << "</agd:RedovisningsPeriod>";
								edos << "\n" << R"(<agd:Specifikationsnummer faltkod="570">)" <<  to_value(xml_map,p + R"(agd:Specifikationsnummer faltkod="570")") << "</agd:Specifikationsnummer>";
								edos << "\n" << R"(<agd:AvdrPrelSkatt faltkod="001">)" <<  to_value(xml_map,p + R"(agd:AvdrPrelSkatt faltkod="001")") << "</agd:AvdrPrelSkatt>";
								--p;
							edos << "\n" << R"(</agd:IU>)";
							--p;
						edos << "\n" << R"(</agd:Blankettinnehall>)";
						--p;
					edos << "\n" << R"(</agd:Blankett>)";
					--p;
				edos << "\n" << R"(</Skatteverket>)";
			}
			catch (std::exception const& e) {
				std::cerr << "\nERROR: Failed to generate skv-file, excpetion=" << e.what();
				edos.os.setstate(std::ostream::badbit);
			}
			return edos;
		}

		bool to_employee_contributions_and_PAYE_tax_return_file(std::ostream& os,XMLMap const& xml_map) {
			try {
				EmployerDeclarationOStream edos{os};
				edos << xml_map;
			}
			catch (std::exception const& e) {
				std::cerr << "\nERROR: Failed to generate skv-file, excpetion=" << e.what();
			}
			return static_cast<bool>(os);
		}

		namespace VATReturns {

			extern char const* ACCOUNT_VAT_CSV; // See bottom of this source file

			// An example provided by Swedish Tax Agency at https://www.skatteverket.se/download/18.3f4496fd14864cc5ac99cb2/1415022111801/momsexempel_v6.txt
			auto eskd_template_0 = R"(<?xml version="1.0" encoding="ISO-8859-1"?>
<!DOCTYPE eSKDUpload PUBLIC "-//Skatteverket, Sweden//DTD Skatteverket eSKDUpload-DTD Version 6.0//SV" "https://www1.skatteverket.se/demoeskd/eSKDUpload_6p0.dtd">
<eSKDUpload Version="6.0">
  <OrgNr>599900-0465</OrgNr>
  <Moms>
    <Period>201507</Period>
    <ForsMomsEjAnnan>100000</ForsMomsEjAnnan>
    <UttagMoms>200000</UttagMoms>
    <UlagMargbesk>300000</UlagMargbesk>
    <HyrinkomstFriv>400000</HyrinkomstFriv>
    <InkopVaruAnnatEg>5000</InkopVaruAnnatEg>
    <InkopTjanstAnnatEg>6000</InkopTjanstAnnatEg>
    <InkopTjanstUtomEg>7000</InkopTjanstUtomEg>
    <InkopVaruSverige>8000</InkopVaruSverige>
    <InkopTjanstSverige>9000</InkopTjanstSverige>
    <MomsUlagImport>10000</MomsUlagImport>
    <ForsVaruAnnatEg>11000</ForsVaruAnnatEg>
    <ForsVaruUtomEg>12000</ForsVaruUtomEg>
    <InkopVaruMellan3p>13000</InkopVaruMellan3p>
    <ForsVaruMellan3p>14000</ForsVaruMellan3p>
    <ForsTjSkskAnnatEg>15000</ForsTjSkskAnnatEg>
    <ForsTjOvrUtomEg>16000</ForsTjOvrUtomEg>
    <ForsKopareSkskSverige>17000</ForsKopareSkskSverige>
    <ForsOvrigt>18000</ForsOvrigt>
    <MomsUtgHog>200000</MomsUtgHog>
    <MomsUtgMedel>15000</MomsUtgMedel>
    <MomsUtgLag>5000</MomsUtgLag>
    <MomsInkopUtgHog>2500</MomsInkopUtgHog>
    <MomsInkopUtgMedel>1000</MomsInkopUtgMedel>
    <MomsInkopUtgLag>500</MomsInkopUtgLag>
    <MomsImportUtgHog>2000</MomsImportUtgHog>
    <MomsImportUtgMedel>350</MomsImportUtgMedel>
    <MomsImportUtgLag>150</MomsImportUtgLag>
    <MomsIngAvdr>1000</MomsIngAvdr>
    <MomsBetala>225500</MomsBetala>
    <TextUpplysningMoms>Bla bla bla bla</TextUpplysningMoms>
  </Moms>
</eSKDUpload>)";

			// An example eskd-file generated by the online program Fortnox
			auto eskd_template_1 = R"(<?xml version="1.0" encoding="ISO-8859-1"?>
<!DOCTYPE eSKDUpload PUBLIC "-//Skatteverket, Sweden//DTD Skatteverket eSKDUpload-DTD Version 6.0//SV" "https://www1.skatteverket.se/demoeskd/eSKDUpload_6p0.dtd">
<eSKDUpload Version="6.0">
  <OrgNr>556782-8172</OrgNr>
  <Moms>
    <Period>202109</Period>
    <ForsMomsEjAnnan>333200</ForsMomsEjAnnan>
    <InkopVaruAnnatEg>6616</InkopVaruAnnatEg>
    <MomsUtgHog>83300</MomsUtgHog>
    <MomsInkopUtgHog>1654</MomsInkopUtgHog>
    <MomsIngAvdr>2539</MomsIngAvdr>
    <MomsBetala>82415</MomsBetala>
  </Moms>
</eSKDUpload>)";

		// An example eskd-file generated by Swedish Tax Agency web service at https://www1.skatteverket.se/es/skapaeskdfil/tillStart.do
		auto eskd_template_2 = R"(<?xml version="1.0" encoding="ISO-8859-1"?>
<!DOCTYPE eSKDUpload PUBLIC "-//Skatteverket, Sweden//DTD Skatteverket eSKDUpload-DTD Version 6.0//SV" "https://www1.skatteverket.se/demoeskd/eSKDUpload_6p0.dtd">
<eSKDUpload Version="6.0">
  <OrgNr>556782-8172</OrgNr>
  <Moms>
    <Period>202203</Period>
    <ForsMomsEjAnnan>333200</ForsMomsEjAnnan>
    <InkopVaruAnnatEg>6616</InkopVaruAnnatEg>
    <MomsUlagImport>597</MomsUlagImport>
    <ForsTjSkskAnnatEg>957</ForsTjSkskAnnatEg>
    <MomsUtgHog>83300</MomsUtgHog>
    <MomsInkopUtgHog>1654</MomsInkopUtgHog>
    <MomsImportUtgHog>149</MomsImportUtgHog>
    <MomsIngAvdr>2688</MomsIngAvdr>
    <MomsBetala>82415</MomsBetala>
  </Moms>
</eSKDUpload>)";

			// Deduced mapping between some VAT Return form box numbers and XML tag names to hold corresponding amount
			// Amount		VAT Return Box			XML Tag
			// 333200		05									"ForsMomsEjAnnan"
			// 83300		10									"MomsUtgHog"
			// 6616			20									"InkopVaruAnnatEg"
			// 1654			30									"MomsInkopUtgHog"
			// 957			39									"ForsTjSkskAnnatEg"
			// 2688			48									"MomsIngAvdr"
			// 82415		49									"MomsBetala"
			// 597			50									"MomsUlagImport"
			// 149			60									"MomsImportUtgHog"

			using FormBoxMap = std::map<BoxNo,BAS::MetaAccountTransactions>;

			// Meta-data required to frame a VAT Returns form to SKV
			struct VatReturnsMeta {
				DateRange period;
				std::string period_to_declare; // VAT Returns period e.g., 202203 for Mars 2022 (also implying Q1 jan-mars)
			};
			using OptionalVatReturnsMeta = std::optional<VatReturnsMeta>;

			OptionalVatReturnsMeta to_vat_returns_meta(DateRange const& period) {
				OptionalVatReturnsMeta result{};
				try {
					std::string period_to_declare = to_string(period.end()); // YYYYMMDD
					result = {
						.period = period
						,.period_to_declare = period_to_declare.substr(0,6) // only YYYYMM
					};
				}
				catch (std::exception const& e) {
					std::cerr << "ERROR, to_vat_returns_meta failed. Excpetion=" << std::quoted(e.what());
				}
				return result;
			}

			Amount to_form_sign(BoxNo box_no, Amount amount) {
				// All VAT Returns form amounts are without sign except box 49 where + means VAT to pay and - means VAT to "get back"
				switch (box_no) {
					case 49: return -1 * amount; break; // The VAT Returns form must encode VAT to be paied as positive (opposite of how it is booked in BAS, negative meaning a debt to SKV)
					default: return std::abs(amount);
				}
			}

			std::ostream& operator<<(std::ostream& os,SKV::XML::VATReturns::FormBoxMap const& fbm) {
				for (auto const& [boxno,mats] : fbm) {
					auto mat_sum = BAS::mats_sum(mats);
					os << "\nVAT returns Form[" << boxno << "] = " << to_tax(to_form_sign(boxno,mat_sum)) << " (from sum " <<  mat_sum << ")";
					Amount rolling_amount{};
					for (auto const& mat : mats) {
						rolling_amount += mat.defacto.amount;
						os << "\n\t" << to_string(mat.meta.defacto.date) << " " << mat << " " << rolling_amount;
					}
				}
				return os;
			}

			struct OStream {
				std::ostream& os;
				operator bool() {return static_cast<bool>(os);}
			};

			OStream& operator<<(OStream& os,std::string const& s) {
				os.os << s;
				return os;
			}

			OStream& operator<<(OStream& os,XMLMap::value_type const& entry) {
				Key::Path p{entry.first};
				std::string indent(p.size(),' ');
				os << indent << "<" << p.back() << ">" << entry.second << "</" << p.back() << ">";
				return os;
			}

			OStream& operator<<(OStream& os,SKV::XML::XMLMap const& xml_map) {
				Key::Path p{};
				os << R"(<!DOCTYPE eSKDUpload PUBLIC "-//Skatteverket, Sweden//DTD Skatteverket eSKDUpload-DTD Version 6.0//SV" "https://www1.skatteverket.se/demoeskd/eSKDUpload_6p0.dtd">)";
				os << "\n" << R"(<eSKDUpload Version="6.0">)";
				p += R"(eSKDUpload Version="6.0")";
				//   <OrgNr>556782-8172</OrgNr>
				os << "\n" << to_entry(xml_map,p + "OrgNr");
				os << "\n  " << R"(<Moms>)";
				p += R"(Moms)";
				// NOTE: It seems SKV requires the XML tags to come in a required sequence
				// See DTD file 
				// <!ELEMENT Moms (Period, ForsMomsEjAnnan?, UttagMoms?, UlagMargbesk?, HyrinkomstFriv?, InkopVaruAnnatEg?, InkopTjanstAnnatEg?, InkopTjanstUtomEg?, InkopVaruSverige?, InkopTjanstSverige?, MomsUlagImport?, ForsVaruAnnatEg?, ForsVaruUtomEg?, InkopVaruMellan3p?, ForsVaruMellan3p?, ForsTjSkskAnnatEg?, ForsTjOvrUtomEg?, ForsKopareSkskSverige?, ForsOvrigt?, MomsUtgHog?, MomsUtgMedel?, MomsUtgLag?, MomsInkopUtgHog?, MomsInkopUtgMedel?, MomsInkopUtgLag?, MomsImportUtgHog?, MomsImportUtgMedel?, MomsImportUtgLag?, MomsIngAvdr?, MomsBetala?, TextUpplysningMoms?)>
				//     <!ELEMENT Period (#PCDATA)>
				//     <!ELEMENT ForsMomsEjAnnan (#PCDATA)>
				//     <!ELEMENT UttagMoms (#PCDATA)>
				//     <!ELEMENT UlagMargbesk (#PCDATA)>
				//     <!ELEMENT HyrinkomstFriv (#PCDATA)>
				//     <!ELEMENT InkopVaruAnnatEg (#PCDATA)>
				//     <!ELEMENT InkopTjanstAnnatEg (#PCDATA)>
				//     <!ELEMENT InkopTjanstUtomEg (#PCDATA)>
				//     <!ELEMENT InkopVaruSverige (#PCDATA)>
				//     <!ELEMENT InkopTjanstSverige (#PCDATA)>
				//     <!ELEMENT MomsUlagImport (#PCDATA)>
				//     <!ELEMENT ForsVaruAnnatEg (#PCDATA)>
				//     <!ELEMENT ForsVaruUtomEg (#PCDATA)>
				//     <!ELEMENT InkopVaruMellan3p (#PCDATA)>
				//     <!ELEMENT ForsVaruMellan3p (#PCDATA)>
				//     <!ELEMENT ForsTjSkskAnnatEg (#PCDATA)>
				//     <!ELEMENT ForsTjOvrUtomEg (#PCDATA)>
				//     <!ELEMENT ForsKopareSkskSverige (#PCDATA)>
				//     <!ELEMENT ForsOvrigt (#PCDATA)>
				//     <!ELEMENT MomsUtgHog (#PCDATA)>
				//     <!ELEMENT MomsUtgMedel (#PCDATA)>
				//     <!ELEMENT MomsUtgLag (#PCDATA)>
				//     <!ELEMENT MomsInkopUtgHog (#PCDATA)>
				//     <!ELEMENT MomsInkopUtgMedel (#PCDATA)>
				//     <!ELEMENT MomsInkopUtgLag (#PCDATA)>
				//     <!ELEMENT MomsImportUtgHog (#PCDATA)>
				//     <!ELEMENT MomsImportUtgMedel (#PCDATA)>
				//     <!ELEMENT MomsImportUtgLag (#PCDATA)>
				//     <!ELEMENT MomsIngAvdr (#PCDATA)>
				//     <!ELEMENT MomsBetala (#PCDATA)>
				//     <!ELEMENT TextUpplysningMoms (#PCDATA)>

				//     <Period>202203</Period>
				os << "\n" << to_entry(xml_map,p + "Period");
				//     <ForsMomsEjAnnan>333200</ForsMomsEjAnnan>
				os << "\n" << to_entry(xml_map,p + "ForsMomsEjAnnan");
				//     <InkopVaruAnnatEg>6616</InkopVaruAnnatEg>
				os << "\n" << to_entry(xml_map,p + "InkopVaruAnnatEg");
				//     <MomsUlagImport>597</MomsUlagImport>
				os << "\n" << to_entry(xml_map,p + "MomsUlagImport");
				//     <ForsTjSkskAnnatEg>957</ForsTjSkskAnnatEg>
				os << "\n" << to_entry(xml_map,p + "ForsTjSkskAnnatEg");
				//     <MomsUtgHog>83300</MomsUtgHog>
				os << "\n" << to_entry(xml_map,p + "MomsUtgHog");
				//     <MomsInkopUtgHog>1654</MomsInkopUtgHog>
				os << "\n" << to_entry(xml_map,p + "MomsInkopUtgHog");
				//     <MomsImportUtgHog>149</MomsImportUtgHog>
				os << "\n" << to_entry(xml_map,p + "MomsImportUtgHog");
				//     <MomsIngAvdr>2688</MomsIngAvdr>
				os << "\n" << to_entry(xml_map,p + "MomsIngAvdr");
				//     <MomsBetala>82415</MomsBetala>
				os << "\n" << to_entry(xml_map,p + "MomsBetala");
				--p;
				os << "\n  " << R"(</Moms>)";
				--p;
				os << "\n" << R"(</eSKDUpload>)";
				return os;
			}

			std::map<BoxNo,std::string> BOXNO_XML_TAG_MAP {
				// Amount		VAT Return Box			XML Tag
				// 333200		05									"ForsMomsEjAnnan"
				{5,R"(ForsMomsEjAnnan)"}
				// 83300		10									"MomsUtgHog"
				,{10,R"(MomsUtgHog)"}
				// 6616			20									"InkopVaruAnnatEg"
				,{20,R"(InkopVaruAnnatEg)"}
				// 1654			30									"MomsInkopUtgHog"
				,{30,R"(MomsInkopUtgHog)"}
				// 957			39									"ForsTjSkskAnnatEg"
				,{39,R"(ForsTjSkskAnnatEg)"}
				// 2688			48									"MomsIngAvdr"
				,{48,R"(MomsIngAvdr)"}
				// 82415		49									"MomsBetala"
				,{49,R"(MomsBetala)"}
				// 597			50									"MomsUlagImport"
				,{50,R"(MomsUlagImport)"}
				// 149			60									"MomsImportUtgHog"
				,{60,R"(MomsImportUtgHog)"}
			};

			std::string to_xml_tag(BoxNo const& box_no) {
				std::string result{"??"};
				if (BOXNO_XML_TAG_MAP.contains(box_no)) {
					auto const& tag = BOXNO_XML_TAG_MAP.at(box_no);
					if (tag.size() > 0) result = tag;
					else throw std::runtime_error{std::string{"ERROR: to_xml_tag failed. tag for box_no:"} + std::to_string(box_no) + " of zero length"};  
				}
				else throw std::runtime_error{std::string{"ERROR: to_xml_tag failed. box_no:"} + std::to_string(box_no) + " not defined"};
				return result;
			}

			std::string to_11_digit_orgno(std::string generic_org_no) {
				std::string result{generic_org_no};
				switch (result.size()) {
					case 10: result = result.substr(0,7) + "-" + result.substr(7,4);
						break;
					case 11: if (result[6] != '-') throw std::runtime_error(std::string{"ERROR: to_11_digit_orgno failed, can't process org_no="} + generic_org_no);
						break;
					case 12: result = to_11_digit_orgno(result.substr(2)); // recurce with assumed prefix "16" removed
						break;							
					default: throw std::runtime_error(std::string{"ERROR: to_11_digit_orgno failed, wrong length org_no="} + generic_org_no);
						break;
				}
				return result;
			}

			Key::Paths account_vat_form_mapping() {
				Key::Paths result{};
				std::istringstream is{ACCOUNT_VAT_CSV};
				std::string row{};
				while (std::getline(is,row)) {
					result.push_back(Key::Path{row,';'});
				}
				return result;
			}

			BASAccountNos to_accounts(BoxNo box_no) {
				static auto const ps = account_vat_form_mapping();
				BASAccountNos result{};
				return std::accumulate(ps.begin(),ps.end(),BASAccountNos{},[&box_no](auto acc,Key::Path const& p){
					std::ostringstream os{};
					os << std::setfill('0') << std::setw(2) << box_no;
					if (p[2].find(os.str()) != std::string::npos) acc.push_back(std::stoi(p[0]));
					return acc;
				});
				return result;
			}

			std::set<BASAccountNo> to_accounts(BoxNos const& box_nos) {
				std::set<BASAccountNo> result{};
				for (auto const& box_no : box_nos) {
					auto vat_account_nos = to_accounts(box_no);
					std::copy(vat_account_nos.begin(),vat_account_nos.end(),std::inserter(result,result.end()));
				}
				return result;
			}

			std::set<BASAccountNo> to_vat_accounts() {
				return to_accounts({10,11,12,30,31,32,60,61,62,48,49});
			}			

			auto is_not_vat_returns_form_transaction(BAS::MetaAccountTransaction const& mat) {
				return (mat.meta.meta.series != 'M');
			}

			BAS::MetaAccountTransactions to_mats(SIEEnvironment const& sie_env,auto const& matches_mat) {
				BAS::MetaAccountTransactions result{};
				auto x = [&matches_mat,&result](BAS::MetaAccountTransaction const& mat){
					if (matches_mat(mat)) result.push_back(mat);
				};
				for_each_meta_account_transaction(sie_env,x);
				return result;
			}

			BAS::MetaAccountTransactions to_mats(SIEEnvironments const& sie_envs,auto const& matches_mat) {
				BAS::MetaAccountTransactions result{};
				auto x = [&matches_mat,&result](BAS::MetaAccountTransaction const& mat){
					if (matches_mat(mat)) result.push_back(mat);
				};
				for_each_meta_account_transaction(sie_envs,x);
				return result;
			}

			std::optional<SKV::XML::XMLMap> to_xml_map(FormBoxMap const& vat_returns_form_box_map,SKV::OrganisationMeta const& org_meta,SKV::XML::DeclarationMeta const& form_meta) {
				std::optional<SKV::XML::XMLMap> result{};
				try {
					XMLMap xml_map{};
						// Amount		VAT Return Box			XML Tag
						// 333200		05									"ForsMomsEjAnnan"
						// 83300		10									"MomsUtgHog"
						// 6616			20									"InkopVaruAnnatEg"
						// 1654			30									"MomsInkopUtgHog"
						// 957			39									"ForsTjSkskAnnatEg"
						// 2688			48									"MomsIngAvdr"
						// 82415		49									"MomsBetala"
						// 597			50									"MomsUlagImport"
						// 149			60									"MomsImportUtgHog"
						Key::Path p{};
						// <!DOCTYPE eSKDUpload PUBLIC "-//Skatteverket, Sweden//DTD Skatteverket eSKDUpload-DTD Version 6.0//SV" "https://www1.skatteverket.se/demoeskd/eSKDUpload_6p0.dtd">
						// <eSKDUpload Version="6.0">
						p += R"(eSKDUpload Version="6.0")";
						//   <OrgNr>556782-8172</OrgNr>
						xml_map[p+"OrgNr"] = to_11_digit_orgno(org_meta.org_no);
						//   <Moms>
						p += R"(Moms)";
						//     <Period>202203</Period>
						xml_map[p+"Period"] = form_meta.declaration_period_id;
						for (auto const& [box_no,mats] : vat_returns_form_box_map)  {
							xml_map[p+to_xml_tag(box_no)] = std::to_string(to_tax(to_form_sign(box_no,BAS::mats_sum(mats))));
						}
						//     <ForsMomsEjAnnan>333200</ForsMomsEjAnnan>
						//     <InkopVaruAnnatEg>6616</InkopVaruAnnatEg>
						//     <MomsUlagImport>597</MomsUlagImport>
						//     <ForsTjSkskAnnatEg>957</ForsTjSkskAnnatEg>
						//     <MomsUtgHog>83300</MomsUtgHog>
						//     <MomsInkopUtgHog>1654</MomsInkopUtgHog>
						//     <MomsImportUtgHog>149</MomsImportUtgHog>
						//     <MomsIngAvdr>2688</MomsIngAvdr>
						//     <MomsBetala>82415</MomsBetala>
						--p;
						//   </Moms>
						--p;
						// </eSKDUpload>;
						result = xml_map;
				}
				catch (std::exception const& e) {
					std::cerr << "\nERROR, to_xml_map failed. Expection=" << std::quoted(e.what());
				}
				return result;
			}

			BAS::MetaAccountTransaction dummy_mat(Amount amount) {
				return {
					.meta = {
						.meta = {
							.series = 'X'
						}
						,.defacto = {
							.caption = "Dummy..."
						}
					}
					,.defacto = {
						.amount = amount
					}
				};
			}

			BAS::MetaAccountTransactions to_vat_returns_mats(BoxNo box_no,SIEEnvironments const& sie_envs,auto mat_predicate) {
				auto account_nos = to_accounts(box_no);
				return to_mats(sie_envs,[&mat_predicate,&account_nos](BAS::MetaAccountTransaction const& mat) {
					return (mat_predicate(mat) and is_any_of_accounts(mat,account_nos) and is_not_vat_returns_form_transaction(mat));
				});
			}

			std::optional<FormBoxMap> to_form_box_map(SIEEnvironments const& sie_envs,auto mat_predicate) {
				std::optional<FormBoxMap> result{};
				try {
					FormBoxMap box_map{};
					// Amount		VAT Return Box			XML Tag
					// 333200		05									"ForsMomsEjAnnan"
					box_map[5] = to_vat_returns_mats(5,sie_envs,mat_predicate);
					// box_map[5].push_back(dummy_mat(333200));
					// 83300		10									"MomsUtgHog"
					box_map[10] = to_vat_returns_mats(10,sie_envs,mat_predicate);
					// box_map[10].push_back(dummy_mat(83300));
					// 6616			20									"InkopVaruAnnatEg"
					box_map[20] = to_vat_returns_mats(20,sie_envs,mat_predicate);
					// box_map[20].push_back(dummy_mat(6616));
					// 1654			30									"MomsInkopUtgHog"
					box_map[30] = to_vat_returns_mats(30,sie_envs,mat_predicate);
					// box_map[30].push_back(dummy_mat(1654));
					// 957			39									"ForsTjSkskAnnatEg"
					box_map[39] = to_vat_returns_mats(39,sie_envs,mat_predicate);
					// box_map[39].push_back(dummy_mat(957));
					// 2688			48									"MomsIngAvdr"
					box_map[48] = to_vat_returns_mats(48,sie_envs,mat_predicate);
					// box_map[48].push_back(dummy_mat(2688));
					// 597			50									"MomsUlagImport"
					box_map[50] = to_vat_returns_mats(50,sie_envs,mat_predicate);
					// box_map[50].push_back(dummy_mat(597));
					// 149			60									"MomsImportUtgHog"
					box_map[60] = to_vat_returns_mats(60,sie_envs,mat_predicate);
					// box_map[60].push_back(dummy_mat(149));

					// NOTE: Box 49, vat designation id R1, R2 is a  t a r g e t  account, NOT a source.
					BoxNos vat_box_nos{10,11,12,30,31,32,50,61,62,48};
					auto box_49_amount = std::accumulate(vat_box_nos.begin(),vat_box_nos.end(),Amount{},[&box_map](Amount acc,BoxNo box_no){
						if (box_map.contains(box_no)) acc += BAS::mats_sum(box_map.at(box_no));
						return acc;
					});
					box_map[49].push_back(dummy_mat(box_49_amount));

					result = box_map;
				}
				catch (std::exception const& e) {
					std::cerr << "\nERROR, to_form_box_map failed. Expection=" << std::quoted(e.what());
				}
				return result;
			}

			OptionalHeadingAmountDateTransEntry to_vat_returns_had(SIEEnvironments const& sie_envs) {
				OptionalHeadingAmountDateTransEntry result{};
				try {
					// Create a had for last quarter if there is no journal entry in the M series for it
					// Otherwise create a had for current quarter
					auto today = to_today();
					{
						// had for previous quarter VAT Returns
						auto quarter = to_previous_quarter(to_quarter_range(today));
						auto vat_returns_meta = to_vat_returns_meta(quarter);
						auto is_quarter = [&vat_returns_meta](BAS::MetaAccountTransaction const& mat){
							return vat_returns_meta->period.contains(mat.meta.defacto.date);
						};
						if (auto box_map = to_form_box_map(sie_envs,is_quarter)) {
							std::cerr << "\nTODO: In to_vat_returns_had turn created box_map for previous quarter to a had";
						}
					}
					{
						auto quarter = to_quarter_range(today);
						auto vat_returns_meta = to_vat_returns_meta(quarter);
						auto is_quarter = [&vat_returns_meta](BAS::MetaAccountTransaction const& mat){
							return vat_returns_meta->period.contains(mat.meta.defacto.date);
						};
						if (auto box_map = to_form_box_map(sie_envs,is_quarter)) {
							std::cerr << "\nTODO: In to_vat_returns_had turn created box_map for current quarter to a had";
						}
					}

					// if (auto vat_returns_meta = SKV::XML::VATReturns::to_vat_returns_meta()) {
					// 	SKV::OrganisationMeta org_meta {
					// 		.org_no = model->sie["current"].organisation_no.CIN
					// 	};
					// 	SKV::XML::DeclarationMeta form_meta {
					// 		.declaration_period_id = vat_returns_meta->period_to_declare
					// 	};
					// 	auto is_quarter = to_is_period(vat_returns_meta->period);
					// 	auto box_map = SKV::XML::VATReturns::to_form_box_map(model->sie,is_quarter);
					// }
				}
				catch (std::exception const& e) {
					std::cerr << "\nERROR: to_vat_returns_had failed. Excpetion = " << std::quoted(e.what());
				}
				return result;
			}

		} // namespace VATReturns
	} // namespace XML

	namespace CSV {
		// See https://www.skatteverket.se/foretag/moms/deklareramoms/periodisksammanstallning/lamnaperiodisksammanstallningmedfiloverforing.4.7eada0316ed67d72822104.html
		// Note 1: I have failed to find an actual technical specification document that specifies the format of the file to send for the form "Periodisk Sammanställning"
		// Note 2: This CSV format is one of three file formats (CSV, SRU and XML) I know of so far that Swedish tax agency uses for file uploads?

		// Example: "SKV574008;""
		namespace EUSalesList {

			using FormAmount = unsigned int; // All EU Sales List amounts are positive (as opposed to BAS account sales that are credit = negative)

			// Correct sign and rounding
			Amount to_eu_sales_list_amount(Amount amount) {
				return std::abs(std::round(amount)); // All amounts in the sales list form are defined to be positve (although sales in BAS are negative credits)
			}

			// Correct amount type for the form
			FormAmount to_form_amount(Amount amount) {
				return to_eu_sales_list_amount(amount);
			}

			std::string to_10_digit_orgno(std::string generic_org_no) {
				std::string result{};
				// The SKV CSV IT-system requires 10 digit organisation numbers with digits only
				// E.g., SIE-file organisation XXXXXX-YYYY has to be transformed into XXXXXXYYYY
				// See https://sv.wikipedia.org/wiki/Organisationsnummer
				std::string sdigits = filtered(generic_org_no,::isdigit);
				switch (sdigits.size()) {
					case 10: result = sdigits; break;
					case 12: result = sdigits.substr(2); // Assume the two first are the prefix "16"
					default: throw std::runtime_error(std::string{"ERROR: to_10_digit_orgno failed, invalid input generic_org_no:"} + "\"" + generic_org_no + "\""); break;
				}
				return result;
			}


			struct SwedishVATRegistrationID {std::string twenty_digits{};};
			struct EUVATRegistrationID {
				std::string with_country_code{};
				bool operator<(EUVATRegistrationID const& other) const {return with_country_code < other.with_country_code;}
			};

			struct Month {
				// Månadskoden för den månad eller det kalenderkvartal uppgifterna gäller, till exempel 2012 för december 2020, 2101 för januari 2021, 
				std::string yymm;
			};

			struct Quarter {
				// Kvartalskoden för det kvartar som uppgifterna gäller, till exempel 20-4 för fjärde kvartalet 2020 eller 21-1 för första kvartalet 2021.
				std::string yy_hyphen_quarter_seq_no{};
				bool operator<(Quarter const& other) const {return yy_hyphen_quarter_seq_no < other.yy_hyphen_quarter_seq_no;};
			};
			using PeriodID = std::variant<Quarter,Month>;

			struct Contact {std::string name_max_35_characters;};
			struct Phone {
				// Swedish telephone numbers are between eight and ten digits long. 
				// They start with a two to four digit area code. 
				// A three digit code starting with 07 indicates that the number is for a mobile phone. 
				// All national numbers start with one leading 0, and international calls are specified by 00 or +. 
				// The numbers are written with the area code followed by a hyphen, 
				// and then two to three groups of digits separated by spaces.
				std::string swedish_format_no_space_max_17_chars{}; // E.g., +CCAA-XXXXXXX where AA is area code without leading zero (070 => 70)
			}; // Consider https://en.wikipedia.org/wiki/National_conventions_for_writing_telephone_numbers#Sweden

			struct FirstRow {
				char const* entry = "SKV574008";
			};

		// Swedish Specification
		/*
		• Det 12-siffriga momsregistreringsnumret för den som är skyldig att lämna uppgifterna. Med eller utan landskoden SE.
		• Månads- eller kvartalskoden för den månad eller det kalenderkvartal uppgifterna gäller, till exempel 2012 för december 2020, 2101 för januari 2021, 20-4 för fjärde kvartalet 2020 eller 21-1 för första kvartalet 2021.
		• Namnet på personen som är ansvarig för de lämnade uppgifterna (högst 35 tecken).
		• Telefonnummer till den som är ansvarig för uppgifterna (endast siffror, med bindestreck efter riktnumret eller utlandsnummer, som inleds med plustecken (högst 17 tecken)).
		• Frivillig uppgift om e-postadress till den som är ansvarig för uppgifterna.		
		*/
		// Example: "556000016701;2001;Per Persson;0123-45690; post@filmkopia.se"
			struct SecondRow {
				SwedishVATRegistrationID vat_registration_id{};
				PeriodID period_id{};
				Contact name{};
				Phone phone_number{};
				std::optional<std::string> e_mail{};
			};

		// Swedish Specification
			/*
			• Köparens momsregistreringsnummer (VAT-nummer) med inledande landskod.
			• Värde av varuleveranser (högst 12 siffror inklusive eventuellt minustecken).
			• Värde av trepartshandel (högst 12 siffror inklusive eventuellt minustecken).
			• Värde av tjänster enligt huvudregeln (högst 12 siffror inklusive eventuellt minustecken)."
			*/
			struct RowN {
				EUVATRegistrationID vat_registration_id{};
				std::optional<FormAmount> goods_amount{};
				std::optional<FormAmount> three_part_business_amount{};
				std::optional<FormAmount> services_amount{};
			};

			struct Form {
				static constexpr FirstRow first_row{};
				SecondRow second_row{};
				std::vector<RowN> third_to_n_row{};
			};

			class OStream {
			public:
				std::ostream& os;
				operator bool() const {return static_cast<bool>(os);}
			};

			OStream& operator<<(OStream& os,char ch) {
				os.os << ch;
				return os;
			}

			OStream& operator<<(OStream& os,std::string const& s) {
				os.os << s;
				return os;
			}

			struct PeriodIDStreamer {
				OStream& os;
				void operator()(Month const& month) {
					os << month.yymm;
				}	
				void operator()(Quarter const& quarter) {
					os << quarter.yy_hyphen_quarter_seq_no;
				}	
			};

			OStream& operator<<(OStream& os,PeriodID const& period_id) {
				PeriodIDStreamer streamer{os};
				std::visit(streamer,period_id);
				return os;
			}
			
			OStream& operator<<(OStream& os,FirstRow const& row) {
				os.os << row.entry << ';';
				return os;
			}

			// Example: "556000016701;2001;Per Persson;0123-45690; post@filmkopia.se"
			OStream& operator<<(OStream& os,SecondRow const& row) {
				os << row.vat_registration_id.twenty_digits;
				os << ';' << row.period_id;
				os << ';' << row.name.name_max_35_characters;
				os << ';' << row.phone_number.swedish_format_no_space_max_17_chars;
				if (row.e_mail) os << ';' << *row.e_mail;
				return os;
			}

			OStream& operator<<(OStream& os,std::optional<FormAmount> const& ot) {
				os << ';';
				if (ot) os << std::to_string(*ot); // to_string to solve ambugiouty (TODO: Try to get rid of ambugiouty? and use os << *ot ?)
				return os;
			}

			// Example: FI01409351;16400;;;
			OStream& operator<<(OStream& os,RowN const& row) {
				os << row.vat_registration_id.with_country_code;
				os << row.goods_amount;
				os << row.three_part_business_amount;
				os << row.services_amount;
				return os;
			}

			OStream& operator<<(OStream& os,Form const& form) {
				os << form.first_row;
				os << '\n' << form.second_row;
				std::for_each(form.third_to_n_row.begin(),form.third_to_n_row.end(),[&os](auto row) {
				 	os << '\n' << row;
				});
				return os;
			}

			Quarter to_eu_list_quarter(Date const& date) {
				auto quarter = to_quarter(date);
				std::ostringstream os{};
				os << (static_cast<int>(date.year()) % 100u) << "-" << quarter.ix;
				return {os.str()};
			}

			EUVATRegistrationID to_eu_vat_id(SKV::XML::VATReturns::BoxNo const& box_no,BAS::MetaAccountTransaction const& mat) {
				std::ostringstream os{};
				if (!mat.defacto.transtext) {
						os << "* transtext " << std::quoted("") << " for " << mat << " does not define the EU VAT ID for this transaction *";						
				}
				else {
					// See https://en.wikipedia.org/wiki/VAT_identification_number#European_Union_VAT_identification_numbers
					const std::regex eu_vat_id("^[A-Z]{2}\\w*"); // Must begin with two uppercase charachters for the country code
					if (std::regex_match(*mat.defacto.transtext,eu_vat_id)) {
						os << *mat.defacto.transtext;
					}
					else {
						os << "* transtext " << std::quoted(*mat.defacto.transtext) << " for " << mat << " does not define the EU VAT ID for this transaction *";						
					}
				}
				return {os.str()};
			}

			std::vector<RowN> sie_to_eu_sales_list_rows(SKV::XML::VATReturns::FormBoxMap const& box_map) {
				// Default example data
				// FI01409351;16400;;;
				std::vector<RowN> result{};
				// Generate a row for each EU VAT Id
				std::map<EUVATRegistrationID,RowN> vat_id_map{};
				for (auto const& [box_no,mats] : box_map) {
					switch (box_no) {
						case 35: {
						} break;
						case 38: {

						} break;
						case 39: {
							auto x = [box_no=box_no,&vat_id_map](BAS::MetaAccountTransaction const& mat){
								auto eu_vat_id = to_eu_vat_id(box_no,mat);
								if (!vat_id_map.contains(eu_vat_id)) vat_id_map[eu_vat_id].vat_registration_id = eu_vat_id;
								if (!vat_id_map[eu_vat_id].services_amount) vat_id_map[eu_vat_id].services_amount = 0;
								*vat_id_map[eu_vat_id].services_amount += to_form_amount(mat.defacto.amount);
							};
							std::for_each(mats.begin(),mats.end(),x);
						} break;
					}
				}
				std::transform(vat_id_map.begin(),vat_id_map.end(),std::back_inserter(result),[](auto const& entry){
					return entry.second;
				});
				return result;
			}
			
			SwedishVATRegistrationID to_swedish_vat_registration_id(SKV::OrganisationMeta const& org_meta) {
				std::ostringstream os{};
				os << "SE" << to_10_digit_orgno(org_meta.org_no) << "01";
				return {os.str()};
			}

			std::optional<Form> vat_returns_to_eu_sales_list_form(SKV::XML::VATReturns::FormBoxMap const& box_map,SKV::OrganisationMeta const& org_meta,DateRange const& period) {
				std::optional<Form> result{};
				try {
					if (org_meta.contact_persons.size()==0) throw std::runtime_error(std::string{"vat_returns_to_eu_sales_list_form failed - zero org_meta.contact_persons"});

					Form form{};
					// struct SecondRow {
					// 	SwedishVATRegistrationID vat_registration_id{};
					// 	PeriodID period_id{};
					// 	Contact name{};
					// 	Phone phone_number{};
					// 	std::optional<std::string> e_mail{};
					// };
					// Default example data
					// 556000016701;2001;Per Persson;0123-45690; post@filmkopia.se					
					SecondRow second_row{
						.vat_registration_id = to_swedish_vat_registration_id(org_meta)
						,.period_id = to_eu_list_quarter(period.end()) // Support for quarter Sales List form so far
						,.name = {org_meta.contact_persons[0].name}
						,.phone_number = {org_meta.contact_persons[0].phone}
						,.e_mail = {org_meta.contact_persons[0].e_mail}
					};
					form.second_row = second_row;
					auto rows = sie_to_eu_sales_list_rows(box_map);
					std::copy(rows.begin(),rows.end(),std::back_inserter(form.third_to_n_row));
					result = form;
				}
				catch (std::exception& e) {
					std::cerr << "\nvat_returns_to_eu_sales_list_form failed. Exception = " << std::quoted(e.what());
				}
				return result;
			}
		} // namespace EUSalesList
	} // namespace CSV {
} // namespace SKV

std::set<BASAccountNo> to_vat_returns_form_bas_accounts(SKV::XML::VATReturns::BoxNos const& box_nos) {
	return SKV::XML::VATReturns::to_accounts(box_nos);
}

std::set<BASAccountNo> const& to_vat_accounts() {
	static auto const vat_accounts = SKV::XML::VATReturns::to_vat_accounts(); // cache
	// Define in terms of how SKV VAT returns form defines linking to BAS Accounts for correct content
	return vat_accounts;
}

// expose operator<< for type alias FormBoxMap, which being an std::map template is causing compiler to consider all std::operator<< in std and not in the one in SKV::XML::VATReturns
// See https://stackoverflow.com/questions/13192947/argument-dependent-name-lookup-and-typedef
using SKV::XML::VATReturns::operator<<;

std::optional<SKV::XML::XMLMap> to_skv_xml_map(SKV::OrganisationMeta sender_meta,SKV::XML::DeclarationMeta declaration_meta,SKV::OrganisationMeta employer_meta,SKV::XML::TaxDeclarations tax_declarations) {
	// std::cout << "\nto_skv_map" << std::flush;
	std::optional<SKV::XML::XMLMap> result{};
	SKV::XML::XMLMap xml_map{SKV::XML::skv_xml_template};
	// sender_meta -> Skatteverket.agd:Avsandare.*
	Key::Path p{};
	try {
		if (sender_meta.contact_persons.size()==0) throw std::runtime_error(std::string{"to_skv_xml_map failed - zero sender_meta.contact_persons"});
		if (employer_meta.contact_persons.size()==0) throw std::runtime_error(std::string{"to_skv_xml_map failed - zero employer_meta.contact_persons"});
		if (tax_declarations.size()==0) throw std::runtime_error(std::string{"to_skv_xml_map failed - zero tax_declarations"});
		// <?xml version="1.0" encoding="UTF-8" standalone="no"?>
		// <Skatteverket omrade="Arbetsgivardeklaration"
		
		p += "Skatteverket";
		//   xmlns="http://xmls.skatteverket.se/se/skatteverket/da/instans/schema/1.1"
		//   xmlns:agd="http://xmls.skatteverket.se/se/skatteverket/da/komponent/schema/1.1"
		//   xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://xmls.skatteverket.se/se/skatteverket/da/instans/schema/1.1 http://xmls.skatteverket.se/se/skatteverket/da/arbetsgivardeklaration/arbetsgivardeklaration_1.1.xsd">
		//   <agd:Avsandare>
		p += "agd:Avsandare";
		//     <agd:Programnamn>Programmakarna AB</agd:Programnamn>
		xml_map[p + "agd:Programnamn"] = "https://github.com/kjelloh/cratchit";
		//     <agd:Organisationsnummer>190002039006</agd:Organisationsnummer>
		xml_map[p + "agd:Organisationsnummer"] = SKV::XML::to_12_digit_orgno(sender_meta.org_no);
		//     <agd:TekniskKontaktperson>
		p += "agd:TekniskKontaktperson";
		//       <agd:Namn>Valle Vadman</agd:Namn>
		xml_map[p + "agd:Namn"] = sender_meta.contact_persons[0].name;
		//       <agd:Telefon>23-2-4-244454</agd:Telefon>
		xml_map[p + "agd:Telefon"] = sender_meta.contact_persons[0].phone;
		//       <agd:Epostadress>valle.vadman@programmakarna.se</agd:Epostadress>
		xml_map[p + "agd:Epostadress"] = sender_meta.contact_persons[0].e_mail;
		--p;
		//     </agd:TekniskKontaktperson>
		//     <agd:Skapad>2021-01-30T07:42:25</agd:Skapad>
		xml_map[p + "agd:Skapad"] = declaration_meta.creation_date_and_time;
		--p;
		//   </agd:Avsandare>
		//   <agd:Blankettgemensamt>
		p += "agd:Blankettgemensamt";

		//     <agd:Arbetsgivare>
		p += "agd:Arbetsgivare";
		// employer_meta -> Skatteverket.agd:Blankettgemensamt.agd:Arbetsgivare.*
		//       <agd:AgRegistreradId>165560269986</agd:AgRegistreradId>
		xml_map[p + "agd:AgRegistreradId"] = SKV::XML::to_12_digit_orgno(employer_meta.org_no);
		//       <agd:Kontaktperson>
		p += "agd:Kontaktperson";
		//         <agd:Namn>Ville Vessla</agd:Namn>
		xml_map[p + "agd:Namn"] = employer_meta.contact_persons[0].name;
		//         <agd:Telefon>555-244454</agd:Telefon>
		xml_map[p + "agd:Telefon"] = employer_meta.contact_persons[0].phone;
		//         <agd:Epostadress>ville.vessla@foretaget.se</agd:Epostadress>
		xml_map[p + "agd:Epostadress"] = employer_meta.contact_persons[0].e_mail;
		--p;
		//       </agd:Kontaktperson>
		--p;
		//     </agd:Arbetsgivare>
		--p;
		//   </agd:Blankettgemensamt>
		//   <!-- Uppgift 1 HU -->
		//   <agd:Blankett>
		p += "agd:Blankett";
		//     <agd:Arendeinformation>
		p += "agd:Arendeinformation";
		//       <agd:Arendeagare>165560269986</agd:Arendeagare>
		xml_map[p + "agd:Arendeagare"] = SKV::XML::to_12_digit_orgno(employer_meta.org_no);
		//       <agd:Period>202101</agd:Period>
		xml_map[p + "agd:Period"] = declaration_meta.declaration_period_id;
		--p;
		//     </agd:Arendeinformation>
		//     <agd:Blankettinnehall>
		p += "agd:Blankettinnehall";
		//       <agd:HU>
		p += "agd:HU";
		// employer_meta + sum(tax_declarations) -> Skatteverket.agd:Kontaktperson.agd:Blankettinnehall.agd:HU.*
		//         <agd:ArbetsgivareHUGROUP>
		p += "agd:ArbetsgivareHUGROUP";
		//           <agd:AgRegistreradId faltkod="201">165560269986</agd:AgRegistreradId>
		std::cout << "\nxml_map[" << p + R"(agd:AgRegistreradId faltkod="201")" << "] = " << SKV::XML::to_12_digit_orgno(employer_meta.org_no);
		xml_map[p + R"(agd:AgRegistreradId faltkod="201")"] = SKV::XML::to_12_digit_orgno(employer_meta.org_no);
		--p;
		//         </agd:ArbetsgivareHUGROUP>
		//         <agd:RedovisningsPeriod faltkod="006">202101</agd:RedovisningsPeriod>
		xml_map[p + R"(agd:RedovisningsPeriod faltkod="006")"] = declaration_meta.declaration_period_id;
		//         <agd:SummaArbAvgSlf faltkod="487">0</agd:SummaArbAvgSlf>
		auto fee_sum = std::accumulate(tax_declarations.begin(),tax_declarations.end(),Amount{0},[](auto acc,auto const& tax_declaration){
			// Amount paid_employer_fee;
			// Amount paid_tax;
			return acc + tax_declaration.paid_employer_fee;
		});
		xml_map[p + R"(agd:SummaArbAvgSlf faltkod="487")"] = std::to_string(SKV::to_fee(fee_sum));
		//         <agd:SummaSkatteavdr faltkod="497">0</agd:SummaSkatteavdr>
		auto tax_sum = std::accumulate(tax_declarations.begin(),tax_declarations.end(),Amount{0},[](auto acc,auto const& tax_declaration){
			// Amount paid_employer_fee;
			// Amount paid_tax;
			return acc + tax_declaration.paid_tax;
		});
		xml_map[p + R"(agd:SummaSkatteavdr faltkod="497")"] = std::to_string(SKV::to_tax(tax_sum));
		--p;
		//       </agd:HU>
		--p;
		//     </agd:Blankettinnehall>
		--p;
		//   </agd:Blankett>
		//   <!-- Uppgift 1 IU -->
		//   <agd:Blankett>
		p += "agd:Blankett";
		//     <agd:Arendeinformation>
		p += "agd:Arendeinformation";
		//       <agd:Arendeagare>165560269986</agd:Arendeagare>
		xml_map[p + R"(agd:Arendeagare)"] = SKV::XML::to_12_digit_orgno(employer_meta.org_no);
		//       <agd:Period>202101</agd:Period>
		xml_map[p + R"(agd:Period)"] = declaration_meta.declaration_period_id;
		--p;
		//     </agd:Arendeinformation>
		//     <agd:Blankettinnehall>
		p += "agd:Blankettinnehall";
		//       <agd:IU>
		p += "agd:IU";
		// employer_meta + tax_declaration[i] -> Skatteverket.agd:Blankett.agd:Blankettinnehall.agd:IU.*
		//         <agd:ArbetsgivareIUGROUP>
		p += "agd:ArbetsgivareIUGROUP";
		//           <agd:AgRegistreradId faltkod="201">165560269986</agd:AgRegistreradId>
		xml_map[p + R"(agd:AgRegistreradId faltkod="201")"] = SKV::XML::to_12_digit_orgno(employer_meta.org_no);
		--p;
		//         </agd:ArbetsgivareIUGROUP>
		//         <agd:BetalningsmottagareIUGROUP>
		p += "agd:BetalningsmottagareIUGROUP";
		//           <agd:BetalningsmottagareIDChoice>
		p += "agd:BetalningsmottagareIDChoice";
		//             <agd:BetalningsmottagarId faltkod="215">198202252386</agd:BetalningsmottagarId>
		xml_map[p + R"(agd:BetalningsmottagarId faltkod="215")"] = tax_declarations[0].employee_12_digit_birth_no;
		--p;
		//           </agd:BetalningsmottagareIDChoice>
		--p;
		//         </agd:BetalningsmottagareIUGROUP>
		//         <agd:RedovisningsPeriod faltkod="006">202101</agd:RedovisningsPeriod>
		xml_map[p + R"(agd:RedovisningsPeriod faltkod="006")"] = declaration_meta.declaration_period_id;;
		//         <agd:Specifikationsnummer faltkod="570">001</agd:Specifikationsnummer>
		xml_map[p + R"(agd:Specifikationsnummer faltkod="570")"] = "001";
		//         <agd:AvdrPrelSkatt faltkod="001">0</agd:AvdrPrelSkatt>
		xml_map[p + R"(agd:AvdrPrelSkatt faltkod="001")"] = std::to_string(SKV::to_tax(tax_declarations[0].paid_tax));
		--p;
		//       </agd:IU>
		--p;
		//     </agd:Blankettinnehall>
		--p;
		//   </agd:Blankett>
		--p;
		// </Skatteverket>
		result = xml_map;
	}
	catch (std::exception const& e) {
		std::cerr << "\nERROR: to_skv_xml_map failed, exception=" << e.what();
	}
	if (result) {
		for (auto const& [tag,value] : *result) {
			std::cout << "\nto_skv_xml_map: " << tag << " = " << std::quoted(value);
		}
	}
	return result;
}

/*
to_skv_xml_map: Skatteverket.agd:Avsandare.agd:Organisationsnummer = "165567828172"
to_skv_xml_map: Skatteverket.agd:Avsandare.agd:Programnamn = "Programmakarna AB"
to_skv_xml_map: Skatteverket.agd:Avsandare.agd:Skapad = "2022-04-12T19:07:27"
to_skv_xml_map: Skatteverket.agd:Avsandare.agd:TekniskKontaktperson.agd:Epostadress = "ville.vessla@foretaget.se"
to_skv_xml_map: Skatteverket.agd:Avsandare.agd:TekniskKontaktperson.agd:Namn = "Ville Vessla"
to_skv_xml_map: Skatteverket.agd:Avsandare.agd:TekniskKontaktperson.agd:Telefon = "555-244454"
to_skv_xml_map: Skatteverket.agd:Blankett.agd:Arendeinformation.agd:Arendeagare = "165560269986"
to_skv_xml_map: Skatteverket.agd:Blankett.agd:Arendeinformation.agd:Period = "202101"
to_skv_xml_map: Skatteverket.agd:Blankett.agd:Blankettinnehall.agd:IU.agd:ArbetsgivareIUGROUP.agd:AgRegistreradId faltkod="201") = "165560269986"
to_skv_xml_map: Skatteverket.agd:Blankett.agd:Blankettinnehall.agd:IU.agd:AvdrPrelSkatt faltkod="001" = "0"
to_skv_xml_map: Skatteverket.agd:Blankett.agd:Blankettinnehall.agd:IU.agd:BetalningsmottagareIUGROUP.agd:BetalningsmottagareIDChoice.agd:BetalningsmottagarId faltkod="215" = "198202252386"
to_skv_xml_map: Skatteverket.agd:Blankett.agd:Blankettinnehall.agd:IU.agd:RedovisningsPeriod faltkod"},{"006" = "202101"
to_skv_xml_map: Skatteverket.agd:Blankett.agd:Blankettinnehall.agd:IU.agd:Specifikationsnummer faltkod="570" = "001"
to_skv_xml_map: Skatteverket.agd:Blankettgemensamt.agd:Arbetsgivare.agd:AgRegistreradId = "165567828172"
to_skv_xml_map: Skatteverket.agd:Blankettgemensamt.agd:Arbetsgivare.agd:Kontaktperson.agd:Epostadress = "ville.vessla@foretaget.se"
to_skv_xml_map: Skatteverket.agd:Blankettgemensamt.agd:Arbetsgivare.agd:Kontaktperson.agd:Namn = "Ville Vessla"
to_skv_xml_map: Skatteverket.agd:Blankettgemensamt.agd:Arbetsgivare.agd:Kontaktperson.agd:Telefon = "555-244454"
to_skv_xml_map: Skatteverket.agd:Blankettgemensamt.agd:Blankett.agd:Arendeinformation.agd:Arendeagare = "165567828172"
to_skv_xml_map: Skatteverket.agd:Blankettgemensamt.agd:Blankett.agd:Arendeinformation.agd:Period = "202101"
to_skv_xml_map: Skatteverket.agd:Blankettgemensamt.agd:Blankett.agd:Blankettinnehall.agd:HU.agd:ArbetsgivareHUGROUP.agd:AgRegistreradId faltkod="201" = "165567828172"
to_skv_xml_map: Skatteverket.agd:Blankettgemensamt.agd:Blankett.agd:Blankettinnehall.agd:HU.agd:RedovisningsPeriod faltkod="006" = "202101"
to_skv_xml_map: Skatteverket.agd:Blankettgemensamt.agd:Blankett.agd:Blankettinnehall.agd:HU.agd:SummaArbAvgSlf faltkod="487" = "0.000000"
to_skv_xml_map: Skatteverket.agd:Blankettgemensamt.agd:Blankett.agd:Blankettinnehall.agd:HU.agd:SummaSkatteavdr faltkod="497" = "0.000000"
to_skv_xml_map: Skatteverket.agd:Blankettgemensamt.agd:Blankett.agd:Blankettinnehall.agd:IU.agd:ArbetsgivareIUGROUP.agd:AgRegistreradId faltkod="201" = "165567828172"
to_skv_xml_map: Skatteverket.agd:Blankettgemensamt.agd:Blankett.agd:Blankettinnehall.agd:IU.agd:AvdrPrelSkatt faltkod="001" = "0.000000"
to_skv_xml_map: Skatteverket.agd:Blankettgemensamt.agd:Blankett.agd:Blankettinnehall.agd:IU.agd:BetalningsmottagareIUGROUP.agd:BetalningsmottagareIDChoice.agd:BetalningsmottagarId faltkod="215" = "198202252386"
to_skv_xml_map: Skatteverket.agd:Blankettgemensamt.agd:Blankett.agd:Blankettinnehall.agd:IU.agd:RedovisningsPeriod faltkod="006" = "202101"
to_skv_xml_map: Skatteverket.agd:Blankettgemensamt.agd:Blankett.agd:Blankettinnehall.agd:IU.agd:Specifikationsnummer faltkod="570" = "001"
to_skv_xml_map: Skatteverket.agd:Kontaktperson.agd:Arendeinformation.agd:Arendeagare = "165560269986"
to_skv_xml_map: Skatteverket.agd:Kontaktperson.agd:Arendeinformation.agd:Period = "202101"
to_skv_xml_map: Skatteverket.agd:Kontaktperson.agd:Blankettinnehall.agd:HU.agd:ArbetsgivareHUGROUP.agd:AgRegistreradId faltkod="201" = "16556026998"
to_skv_xml_map: Skatteverket.agd:Kontaktperson.agd:Blankettinnehall.agd:HU.agd:RedovisningsPeriod faltkod="006" = "202101"
to_skv_xml_map: Skatteverket.agd:Kontaktperson.agd:Blankettinnehall.agd:HU.agd:SummaArbAvgSlf faltkod="487" = "0"
to_skv_xml_map: Skatteverket.agd:Kontaktperson.agd:Blankettinnehall.agd:HU.agd:SummaSkatteavdr faltkod="497" = "0"
*/

// "2021-01-30T07:42:25"
std::string to_skv_date_and_time(std::chrono::time_point<std::chrono::system_clock> const current) {
	std::ostringstream os{};
	auto now_timet = std::chrono::system_clock::to_time_t(current);
	auto now_local = localtime(&now_timet);
	os << std::put_time(now_local,"%Y-%m-%dT%H:%M:%S");
	return os.str();
}

std::optional<SKV::XML::XMLMap> cratchit_to_skv(SIEEnvironment const& sie_env,	std::vector<SKV::ContactPersonMeta> const& organisation_contacts, std::vector<std::string> const& employee_birth_ids) {
	std::cout << "\ncratchit_to_skv" << std::flush;
	std::cout << "\ncratchit_to_skv organisation_no.CIN=" << sie_env.organisation_no.CIN;
	std::optional<SKV::XML::XMLMap> result{};
	try {
		SKV::OrganisationMeta sender_meta{};sender_meta.contact_persons.push_back({});
		SKV::XML::DeclarationMeta declaration_meta{};
		SKV::OrganisationMeta employer_meta{};employer_meta.contact_persons.push_back({});
		SKV::XML::TaxDeclarations tax_declarations{};tax_declarations.push_back({});
		// declaration_meta.creation_date_and_time = "2021-01-30T07:42:25";
		declaration_meta.creation_date_and_time = to_skv_date_and_time(std::chrono::system_clock::now());
		declaration_meta.declaration_period_id = "202101";
		// employer_meta.org_no = "165560269986";
		employer_meta.org_no = SKV::XML::to_12_digit_orgno(sie_env.organisation_no.CIN);
		// sender_meta.org_no = "190002039006";
		sender_meta.org_no = employer_meta.org_no;
		if (organisation_contacts.size()>0) {
			// Use orgnaisation contact person as the file sender technical contact
			sender_meta.contact_persons[0] = organisation_contacts[0];

			// Use organisation contact person as employer contact
			employer_meta.contact_persons[0] = organisation_contacts[0];
		}
		else {
			throw std::runtime_error("cratchit_to_skv failed. No organisation_contacts provided");
		}
		if (employee_birth_ids.size() > 0) {
			tax_declarations[0].employee_12_digit_birth_no = employee_birth_ids[0];
			tax_declarations[0].paid_tax = 0;
		}
		else {
			throw std::runtime_error("cratchit_to_skv failed. No employee_birth_ids provided");
		}
		result = to_skv_xml_map(sender_meta,declaration_meta,employer_meta,tax_declarations);
	}
	catch (std::exception const& e) {
		std::cerr << "\nERROR: Failed to create SKV data from SIE Environment, expection=" << e.what();
	}
	return result;
}

using EnvironmentValue = std::map<std::string,std::string>;
using Environment = std::multimap<std::string,EnvironmentValue>;

OptionalJournalEntryTemplate template_of(OptionalHeadingAmountDateTransEntry const& had,SIEEnvironment const& sie_environ) {
	OptionalJournalEntryTemplate result{};
	if (had) {
		BAS::MetaEntries candidates{};
		for (auto const& je : sie_environ.journals()) {
			auto const& [series,journal] = je;
			for (auto const& [verno,aje] : journal) {								
				if (aje.caption.find(had->heading) != std::string::npos) {
					candidates.push_back({
						.meta = {
							.series = series
							,.verno = verno}
						,.defacto = aje});
				}
			}
		}
		// select the entry with the latest date
		std::nth_element(candidates.begin(),candidates.begin(),candidates.end(),[](auto const& je1, auto const& je2){
			return (je1.defacto.date > je2.defacto.date);
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

EnvironmentValue to_environment_value(SKV::ContactPersonMeta const& cpm) {
	EnvironmentValue ev{};
	ev["name"] = cpm.name;
	ev["phone"] = cpm.phone;
	ev["e-mail"] = cpm.e_mail;
	return ev;
}

SKV::OptionalContactPersonMeta to_contact(EnvironmentValue const& ev) {
	SKV::OptionalContactPersonMeta result;
	SKV::ContactPersonMeta cpm{};
	while (true) {
		if (auto iter = ev.find("name");iter != ev.end()) cpm.name = iter->second;
		else break;
		if (auto iter = ev.find("phone");iter != ev.end()) cpm.phone = iter->second;
		else break;
		if (auto iter = ev.find("e-mail");iter != ev.end()) cpm.e_mail = iter->second;
		else break;
		result = cpm;
		break;
	}
	return result;
}

std::optional<std::string> to_employee(EnvironmentValue const& ev) {
	std::optional<std::string> result{};
	std::string birth_id{};
	while (true) {
		if (auto iter = ev.find("birth-id");iter != ev.end()) birth_id = iter->second;
		else break;
		result = birth_id;
		break;
	}
	return result;
}

enum class PromptState {
	Undefined
	,Root
	,HADIndex
	,JEIndex
	,AccountIndex
	,Amount
	,CounterAccountsEntry
	,SKVEntryIndex
	,SKVTaxReturnEntryIndex
	,EnterContact
	,EnterEmployeeID
	,Unknown
};

class ConcreteModel {
public:
	std::vector<SKV::ContactPersonMeta> organisation_contacts{};
	std::vector<std::string> employee_birth_ids{};
	std::string user_input{};
	PromptState prompt_state{PromptState::Root};
	size_t had_index{};
	BAS::MetaEntries template_candidates{};
	BAS::anonymous::AccountTransactions at_candidates{};
	BAS::anonymous::AccountTransaction at{};
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

	BAS::anonymous::OptionalAccountTransaction selected_had_at(int at_index) {
		BAS::anonymous::OptionalAccountTransaction result{};
		if (auto had = this->selected_had()) {
			if (had->current_candidate) {
				auto iter = had->current_candidate->defacto.account_transactions.begin();
				auto end = had->current_candidate->defacto.account_transactions.end();
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

struct KeyPress {char value;};
using Command = std::string;
struct Quit {};
struct Nop {};

using Msg = std::variant<Nop,KeyPress,Quit,Command>;
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

class FilteredSIEEnvironment {
public:
	FilteredSIEEnvironment(SIEEnvironment const& sie_environment,BAS::MatchesMetaEntry matches_meta_entry)
		:  m_sie_environment{sie_environment}
			,m_matches_meta_entry{matches_meta_entry} {}

	void for_each(auto const& f) const {
		auto f_if_match = [this,&f](BAS::MetaEntry const& me){
			if (this->m_matches_meta_entry(me)) f(me);
		};
		for_each_meta_entry(m_sie_environment,f_if_match);
	}
private:
	SIEEnvironment const& m_sie_environment;
	BAS::MatchesMetaEntry m_matches_meta_entry{};
};

std::ostream& operator<<(std::ostream& os,FilteredSIEEnvironment const& filtered_sie_environment) {
	struct stream_entry_to {
		std::ostream& os;
		void operator()(BAS::MetaEntry const& me) const {
			os << '\n' << me;
		}
	};
	os << "\n*Filter BEGIN*";
	filtered_sie_environment.for_each(stream_entry_to{os});
	os << "\n*Filter end*";
	return os;
}

std::ostream& operator<<(std::ostream& os,SIEEnvironment const& sie_environment) {
	os << "\nCIN:" << sie_environment.organisation_no.CIN;
	for (auto const& je : sie_environment.journals()) {
		auto& [series,journal] = je;
		for (auto const& [verno,entry] : journal) {
			BAS::MetaEntry me {
				.meta = {
					 .series = series
					,.verno = verno
					,.unposted_flag = sie_environment.is_unposted(series,verno)
				}
				,.defacto = entry
			};
			os << '\n' << me;
		}
	}
	return os;
}

BAS::MetaEntry to_entry(SIE::Ver const& ver) {
	BAS::MetaEntry result{
		.meta = {
			.series = ver.series
			,.verno = ver.verno
		}
	};
	result.defacto.caption = ver.vertext;
	result.defacto.date = ver.verdate;
	for (auto const& trans : ver.transactions) {								
		result.defacto.account_transactions.push_back(BAS::anonymous::AccountTransaction{
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
			if (auto opt_entry = SIE::parse_ORGNR(in,"#ORGNR")) {
				SIE::OrgNr orgnr = std::get<SIE::OrgNr>(*opt_entry);
				sie_environment.organisation_no = orgnr;
			}
			else if (auto opt_entry = SIE::parse_FNAMN(in,"#FNAMN")) {
				SIE::FNamn fnamn = std::get<SIE::FNamn>(*opt_entry);
				sie_environment.organisation_name = fnamn;
			}
			else if (auto opt_entry = SIE::parse_ADRESS(in,"#ADRESS")) {
				SIE::Adress adress = std::get<SIE::Adress>(*opt_entry);
				sie_environment.organisation_address = adress;
			}
			else if (auto opt_entry = SIE::parse_KONTO(in,"#KONTO")) {
				SIE::Konto konto = std::get<SIE::Konto>(*opt_entry);
				sie_environment.set_account_name(konto.account_no,konto.name);
			}
			else if (auto opt_entry = SIE::parse_SRU(in,"#SRU")) {
				SIE::Sru sru = std::get<SIE::Sru>(*opt_entry);
				sie_environment.set_account_SRU(sru.bas_account_no,sru.sru_account_no);
			}
			else if (auto opt_entry = SIE::parse_VER(in)) {
				SIE::Ver ver = std::get<SIE::Ver>(*opt_entry);
				// std::cout << "\n\tVER!";
				auto me = to_entry(ver);
				sie_environment.post(me);
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
		case PromptState::SKVEntryIndex: {
			prompt << ":skv";
		} break;
		case PromptState::SKVTaxReturnEntryIndex: {
			prompt << ":skv:tax_return";
		}
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

auto falling_date = [](auto const& had1,auto const& had2){
	return (had1.date > had2.date);
};

std::optional<int> to_signed_ix(std::string const& s) {
	std::optional<int> result{};
	try {
		const std::regex signed_ix_regex("^\\s*[+-]?\\d+$"); // +/-ddd...
		if (std::regex_match(s,signed_ix_regex)) result = std::stoi(s);
	}
	catch (...) {}
	return result;
}

class Updater {
public:
	Model model;
	Cmd operator()(KeyPress const& key) {
		// std::cout << "\noperator(Key)";
		if (model->user_input.size()==0) model->prompt = prompt_line(model->prompt_state);
		Cmd cmd{};
		if (key.value != '\n') {
			// std::cout << "\nKeyPress:" << key.value;
			model->user_input += key.value;
		}
		else {
			cmd = to_cmd(model->user_input);
			model->user_input.clear();
		}
		return cmd;
	}
	Cmd operator()(Command const& command) {
		std::cout << "\noperator(Command)";
		std::ostringstream prompt{};
		auto ast = quoted_tokens(command);
		if (ast.size() > 0) {
			int signed_ix{};
			std::istringstream is{ast[0]};
			if (auto signed_ix = to_signed_ix(ast[0]); signed_ix and model->prompt_state != PromptState::Amount) {
				std::cout << "\nAct on ix = " << *signed_ix;
				size_t ix = std::abs(*signed_ix);
				bool do_remove = (*signed_ix<0);
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
									BAS::MetaEntry me{
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
									me.defacto.account_transactions.push_back(gross_at);
									me.defacto.account_transactions.push_back(net_at);
									me.defacto.account_transactions.push_back(vat_at);
									model->template_candidates.push_back(me);
								}
								// List options
								unsigned ix = 0;
								for (int i=0; i < model->template_candidates.size(); ++i) {
									prompt << "\n    " << ix++ << " " << model->template_candidates[i];
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
								BAS::MetaEntry me{
									.defacto = {
										 .caption = had.heading
									  ,.date = had.date
									}
								};
								me.defacto.account_transactions.emplace_back(BAS::anonymous::AccountTransaction{.account_no=*account_no,.amount=had.amount});
								had.current_candidate = me;
								// List the options to the user
								unsigned int i{};
								prompt << "\n" << had.current_candidate->defacto.caption << " " << had.current_candidate->defacto.date;
								for (auto const& at : had.current_candidate->defacto.account_transactions) {
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
										auto me = to_journal_entry(had,*tp);
										// auto je = to_journal_entry(model->selected_had,*tp);
										if (std::any_of(me.defacto.account_transactions.begin(),me.defacto.account_transactions.end(),[](BAS::anonymous::AccountTransaction const& at){
											return std::abs(at.amount) < 1.0;
										})) {
											// Assume we need to specify rounding
											unsigned int i{};
											std::for_each(me.defacto.account_transactions.begin(),me.defacto.account_transactions.end(),[&i,&prompt](auto const& at){
												prompt << "\n  " << i++ << " " << at;
											});
											had.current_candidate = me;
											model->prompt_state = PromptState::AccountIndex;
										}
										else {
											// Assume we can apply template as-is and stage the generated journal entry
											auto staged_je = model->sie["current"].stage(me);
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

					case PromptState::CounterAccountsEntry: {
						prompt << "\nThe Counter Account Entry state is not yet active in this version of cratchit";
					} break;

					case PromptState::SKVEntryIndex: {
						// prompt << "\n1: Arbetsgivardeklaration (Employer’s contributions and PAYE tax return form)";
						// prompt << "\n2: Periodisk Sammanställning (EU sales list)"
						switch (ix) {
							case 1: {
								// Assume Employer’s contributions and PAYE tax return form

								// List Tax Return Form skv options (user data edit)
								auto const& [delta_prompt,prompt_state] = this->transition_prompt_state(model->prompt_state,PromptState::SKVTaxReturnEntryIndex);
								prompt << delta_prompt;
								model->prompt_state = prompt_state;
							} break;
							case 3: {
								// VAT Returns
								std::string period_id = (ast.size()>1)?ast[1]:"";
								if (auto period_range = SKV::to_date_range(period_id)) {
									std::cout << "\nperiod_range " << *period_range;
									prompt << "\nVAT Returns for " << *period_range;
									if (auto vat_returns_meta = SKV::XML::VATReturns::to_vat_returns_meta(*period_range)) {
										std::cout << "\nvat_returns_meta ";
										SKV::OrganisationMeta org_meta {
											.org_no = model->sie["current"].organisation_no.CIN
											,.contact_persons = model->organisation_contacts
										};
										SKV::XML::DeclarationMeta form_meta {
											.declaration_period_id = vat_returns_meta->period_to_declare
										};
										auto is_quarter = [&vat_returns_meta](BAS::MetaAccountTransaction const& mat){
											return vat_returns_meta->period.contains(mat.meta.defacto.date);
										};
										auto box_map = SKV::XML::VATReturns::to_form_box_map(model->sie,is_quarter);
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
												std::filesystem::path skv_file_name{std::string{"periodisk_sammanstallning_"} + eu_list_quarter.yy_hyphen_quarter_seq_no + ".csv"};						
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
										else prompt << "\nSorry, failed to gather form data required for the VAR Returns form";
									}
									else {
										prompt << "\nSorry, Failed to failed to gather requiored data for period " << period_range;
									}
								}
								else {
									prompt << "\nSorry, failed to understand how to interpret period " << std::quoted(period_id);
								}
							} break;
							default: {prompt << "\nPlease enter a valid index";} break;
						}
					} break;

					case PromptState::SKVTaxReturnEntryIndex: {
						switch (ix) {
							case 1: {model->prompt_state = PromptState::EnterContact;} break;
							case 2: {model->prompt_state = PromptState::EnterEmployeeID;} break;
							default: {prompt << "\nPlease enter a valid index";} break;
						}
					} break;

					case PromptState::EnterContact:
					case PromptState::EnterEmployeeID:
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
					if (ast[1]=="*") {
						// List unposted sie entries
						FilteredSIEEnvironment filtered_sie{model->sie["current"],BAS::filter::is_flagged_unposted{}};
						prompt << filtered_sie;
					}
					else if (model->sie.contains(ast[1])) {
						prompt << model->sie[ast[1]];
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
					else if (ast[1] == "-types") {
						// #TYPES
						using TypedAccountTransactions = std::map<BAS::anonymous::AccountTransaction,std::set<std::string>>;
						auto f = [&prompt](BAS::MetaEntry const& me) {
							TypedAccountTransactions typed_ats{};
							auto gross_amount = to_positive_gross_transaction_amount(me.defacto);

							// Direct type detection based on gross_amount and account meta data
							for (auto const& at : me.defacto.account_transactions) {
								if (std::round(std::abs(at.amount)) == std::round(gross_amount)) typed_ats[at].insert("gross");
								if (is_vat_account_at(at)) typed_ats[at].insert("vat");
								if (std::abs(at.amount) < 1) typed_ats[at].insert("cents");
								if (std::round(std::abs(at.amount)) == std::round(gross_amount / 2)) typed_ats[at].insert("transfer");

								// TODO: Add recognition of this entry?
								// typed: A27Direktinköp EU20210914
								// 	 ? : "PlusGiro":1920 "" -6616.93
								// 	 vat : "Utgående moms omvänd skattskyldighet, 25 %":2614 "Momsrapport (30)" -1654.23
								// 	 vat : "Ingående moms":2640 "" 1654.23
								// 	 ? : "Varuvärde Inlöp annat EG-land (Momsrapport ruta 20)":9021 "Momsrapport (20)" 6616.93
								// 	 ? : "Motkonto Varuvärde Inköp EU/Import":9099 "Motkonto Varuvärde Inköp EU/Import" -6616.93
								// 	 ? : "Elektroniklabb - Verktyg och maskiner":1226 "Favero Assioma DUO-Shi" 6616.93

							}

							// Ex vat amount Detection
							Amount ex_vat_amount{},vat_amount{};
							for (auto const& at : me.defacto.account_transactions) {
								if (!typed_ats.contains(at)) {
									// Not gross, Not VAT = candidate for ex VAT
									ex_vat_amount += at.amount;
								}
								else if (typed_ats.at(at).contains("vat")) {
									vat_amount += at.amount;
								}
							}
							if (std::abs(std::round(std::abs(ex_vat_amount)) + std::round(std::abs(vat_amount)) - gross_amount) <= 1) {
								// ex_vat + vat within cents of gross
								// tag non typed ats as ex-vat
								for (auto const& at : me.defacto.account_transactions) {
									if (!typed_ats.contains(at)) {
										typed_ats[at].insert("net");
									}
								}
							}

							// Identify an EU Purchase journal entry
							// NOTE: I am sure there is some secret algorithm to make thus much easier? (But her goes brute force programming...)
							// Example:
							// typed: A27 Direktinköp EU 20210914
							// 	 gross : "PlusGiro":1920 "" -6616.93
							// 	 eu_vat vat : "Utgående moms omvänd skattskyldighet, 25 %":2614 "Momsrapport (30)" -1654.23
							// 	 eu_vat vat : "Ingående moms":2640 "" 1654.23
							// 	 eu_purchase : "Varuvärde Inlöp annat EG-land (Momsrapport ruta 20)":9021 "Momsrapport (20)" 6616.93
							// 	 eu_purchase : "Motkonto Varuvärde Inköp EU/Import":9099 "Motkonto Varuvärde Inköp EU/Import" -6616.93
							// 	 gross : "Elektroniklabb - Verktyg och maskiner":1226 "Favero Assioma DUO-Shi" 6616.93
							Amount eu_vat_amount{},eu_purchase_amount{};
							for (auto const& at : me.defacto.account_transactions) {
								// Identify transactions to EU VAT and EU Purchase tagged accounts
								if (is_vat_returns_form_at(SKV::XML::VATReturns::EU_VAT_BOX_NOS,at)) {
									typed_ats[at].insert("eu_vat");
									eu_vat_amount = at.amount;
								}
								if (is_vat_returns_form_at(SKV::XML::VATReturns::EU_PURCHASE_BOX_NOS,at)) {
									typed_ats[at].insert("eu_purchase");
									eu_purchase_amount = at.amount;
								}
							}
							for (auto const& at : me.defacto.account_transactions) {
								// Identify counter transactions to EU VAT and EU Purchase tagged accounts
								if (at.amount == -eu_vat_amount) typed_ats[at].insert("eu_vat"); // The counter trans for EU VAT
								if ((first_digit(at.account_no) == 4 or first_digit(at.account_no) == 9) and (at.amount == -eu_purchase_amount)) typed_ats[at].insert("eu_purchase"); // The counter trans for EU Purchase
							}
							// Mark gross accounts for EU VAT transaction journal entry
							for (auto const& at : me.defacto.account_transactions) {
								// We expect two accounts left unmarked and they are the gross accounts
								if (!typed_ats.contains(at) and (std::abs(at.amount) == std::abs(eu_purchase_amount))) {
									typed_ats[at].insert("gross");
								}
							}
							
							// LOG
							prompt << "\ntyped:" << me.meta << " " << me.defacto.caption << " " << me.defacto.date;
							for (auto const& at : me.defacto.account_transactions) {
								prompt << "\n\t";
								if (typed_ats.contains(at)) {
									for (auto const& prop : typed_ats.at(at)) {
										prompt << " " << prop;
									}
								}
								else {
									prompt << " " << "?";
								}
								prompt << " : " << at;
							}
						};
						for_each_meta_entry(model->sie,f);
					}
					else {
						// assume user search criteria on transaction heading and comments
						FilteredSIEEnvironment filtered_sie{model->sie["current"],BAS::filter::matches_user_search_criteria{ast[1]}};
						prompt << "\nNot '*', existing year id or existing file: " << std::quoted(ast[1]);
						prompt << "\nFilter current sie for " << std::quoted(ast[1]); 
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
				if (auto vat_returns_had = SKV::XML::VATReturns::to_vat_returns_had(model->sie)) {
						prompt << "\n*NEW* " << *vat_returns_had;
						model->heading_amount_date_entries.push_back(*vat_returns_had);
				}
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
				auto ats = to_gross_account_transactions(model->sie);
				for (auto const& at : ats) {
					prompt << "\n" << at;
				}				
			}
			else if (ast[0] == "-net") {
				auto ats = to_net_account_transactions(model->sie);
				for (auto const& at : ats) {
					prompt << "\n" << at;
				}				
			}
			else if (ast[0] == "-vat") {
				auto vats = to_vat_account_transactions(model->sie);
				for (auto const& vat : vats) {
					prompt << "\n" << vat;
				}				
			}
			else if (ast[0] == "-t2") {
				auto t2s = t2_entries(model->sie);
				prompt << t2s;
			}
			else if (ast[0] == "-skv") {
				if (ast.size() == 1) {
					// List skv options
					prompt << "\n1: Arbetsgivardeklaration (TAX Returns)";
					prompt << "\n3: Momsrapport (VAT Returns)";
					model->prompt_state = PromptState::SKVEntryIndex;
				}
				else if (ast.size() == 2) {
					// Assume -skv <period>
					if (auto xml_map = cratchit_to_skv(model->sie["current"],model->organisation_contacts,model->employee_birth_ids)) {
						auto period_to_declare = ast[1];
						// Brute force the period into the map (TODO: Inject this value in a better way into the production code above?)
						(*xml_map)[R"(Skatteverket.agd:Blankett.agd:Arendeinformation.agd:Period)"] = period_to_declare;
						(*xml_map)[R"(Skatteverket.agd:Blankett.agd:Blankettinnehall.agd:HU.agd:RedovisningsPeriod faltkod="006")"] = period_to_declare;
						(*xml_map)[R"(Skatteverket.agd:Blankett.agd:Blankettinnehall.agd:IU.agd:RedovisningsPeriod faltkod="006")"] = period_to_declare;
						(*xml_map)[R"(Skatteverket.agd:Kontaktperson.agd:Blankettinnehall.agd:HU.agd:RedovisningsPeriod faltkod="006")"] = period_to_declare;
						std::filesystem::path skv_files_folder{"to_skv"};
						std::filesystem::path skv_file_name{std::string{"arbetsgivaredeklaration_"} + period_to_declare + ".xml"};						
						std::filesystem::path skv_file_path = skv_files_folder / skv_file_name;
						std::filesystem::create_directories(skv_file_path.parent_path());
						std::ofstream skv_file{skv_file_path};
						if (SKV::XML::to_employee_contributions_and_PAYE_tax_return_file(skv_file,*xml_map)) {
							prompt << "\nCreated " << skv_file_path;
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
				std::cout << "\nAct on words";
				// Assume word based input
				if (model->prompt_state == PromptState::JEIndex) {
					// Assume the user has entered a new search criteria for template candidates
					model->template_candidates = this->all_years_template_candidates([&command](BAS::anonymous::JournalEntry const& aje){
						return strings_share_tokens(command,aje.caption);
					});
					int i{0};
					for (; i < model->template_candidates.size(); ++i) {
						prompt << "\n    " << i << " " << model->template_candidates[i];
					}
					// Consider the user may have entered the name of a gross account to journal the transaction amount
					auto gats = to_gross_account_transactions(model->sie);
					model->at_candidates.clear();
					std::copy_if(gats.begin(),gats.end(),std::back_inserter(model->at_candidates),[&command,this](BAS::anonymous::AccountTransaction const& at){
						bool result{false};
						if (at.transtext) result |= strings_share_tokens(command,*at.transtext);
						if (model->sie["current"].account_metas().contains(at.account_no)) {
							auto const& meta = model->sie["current"].account_metas().at(at.account_no);
							result |= strings_share_tokens(command,meta.name);
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
							prompt << "\n" << had->current_candidate->defacto.caption << " " << had->current_candidate->defacto.date;
							for (auto const& at : had->current_candidate->defacto.account_transactions) {
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
								auto me = model->sie["current"].stage(*had->current_candidate);
								if (me) {
									prompt << "\n" << *me;
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
		// std::cout << "\n*prompt=" << prompt.str();
		prompt << prompt_line(model->prompt_state);
		// std::cout << "\nprompt=" << prompt.str();
		model->prompt = prompt.str();
		// std::cout << "\nmodel->prompt=" << model->prompt;
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
	BAS::MetaEntries all_years_template_candidates(auto const& matches) {
		BAS::MetaEntries result{};
		for (auto const& [year_key,sie] : model->sie) {
			auto mes = template_candidates(sie.journals(),matches);
			std::copy(mes.begin(),mes.end(),std::back_inserter(result));
		}
		std::sort(result.begin(),result.end(),[](auto const& je1,auto const& je2){
			return (je1.defacto.date < je2.defacto.date);
		});
		return result;
	}

	std::pair<std::string,PromptState> transition_prompt_state(PromptState const& from_state,PromptState const& to_state) {
		std::ostringstream prompt{};
		switch (to_state) {
			case PromptState::SKVTaxReturnEntryIndex: {
				prompt << "\n1 Organisation Contact:" << std::quoted(model->organisation_contacts[0].name) << " " << std::quoted(model->organisation_contacts[0].phone) << " " << model->organisation_contacts[0].e_mail;
				prompt << "\n2 Employee birth no:" << model->employee_birth_ids[0];
			} break;
			default: {
				prompt << "\nPlease mail developer that transition_prompt_state detected a potential design insufficiency";
			} break;
		}
		return {prompt.str(),to_state};
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
		// std::cout << "\nCratchit::update entry" << std::flush;
		Cmd cmd{};
		std::ostringstream prompt{};
		{
			// Scope to ensure correct move of model in and out of updater
			Updater updater{std::move(model)};
			// std::cout << "\nCratchit::update updater created" << std::flush;
			cmd = std::visit(updater,msg);
			model = std::move(updater.model);
		}
		// std::cout << "\nCratchit::update updater visit ok" << std::flush;
		if (model->quit) {
			// std::cout << "\nCratchit::update if (updater.model->quit) {" << std::flush;
			auto model_environment = environment_from_model(model);
			auto cratchit_environment = add_cratchit_environment(model_environment);
			this->environment_to_file(cratchit_environment,cratchit_file_path);
			unposted_to_sie_file(model->sie["current"],model->staged_sie_file_path);
			// Update/create the skv xml-file (employer monthly tax declaration)
			std::cout << R"(\nmodel->sie["current"].organisation_no.CIN=)" << model->sie["current"].organisation_no.CIN;
		}
		// std::cout << "\nCratchit::update before prompt update" << std::flush;
		// std::cout << "\nCratchit::update before return" << std::flush;
		model->prompt += prompt.str();
		return {std::move(model),cmd};
	}
	Ux view(Model const& model) {
		Ux ux{};
		ux.push_back(model->prompt);
		return ux;
	}
private:
	std::filesystem::path cratchit_file_path{};
	std::vector<SKV::ContactPersonMeta> contacts_from_environment(Environment const& environment) {
		std::vector<SKV::ContactPersonMeta> result{};
		auto [begin,end] = environment.equal_range("contact");
		if (begin == end) {
			// Instantiate default values if required
			result.push_back({
					.name = "Ville Vessla"
					,.phone = "555-244454"
					,.e_mail = "ville.vessla@foretaget.se"
				});
		}
		else {
			std::transform(begin,end,std::back_inserter(result),[](auto const& entry){
				return *to_contact(entry.second); // Assume success
			});
		}
		return result;
	}
	std::vector<std::string> employee_birth_ids_from_environment(Environment const& environment) {
		std::vector<std::string> result{};
		auto [begin,end] = environment.equal_range("employee");
		if (begin == end) {
			// Instantiate default values if required
			result.push_back({"198202252386"});
		}
		else {
			std::transform(begin,end,std::back_inserter(result),[](auto const& entry){
				return *to_employee(entry.second); // Assume success
			});
		}
		return result;
	}
	
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
		model->organisation_contacts = this->contacts_from_environment(environment);
		model->employee_birth_ids = this->employee_birth_ids_from_environment(environment);
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
		// sev += std::string{";"} + "path" + "=" + model->sie_file_path["current"].string();
		result.insert({"sie_file",to_environment_value(sev)});
		for (auto const& entry : model->organisation_contacts) {
			result.insert({"contact",to_environment_value(entry)});
		}
		for (auto const& entry : model->employee_birth_ids) {
			result.insert({"employee",to_environment_value(std::string{"birth-id="} + entry)});
		}
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
			try {
				// std::cout << "\nREPL::run before view" << std::flush;
				auto ux = cratchit.view(model);
				// std::cout << "\nREPL::run before render" << std::flush;
				render_ux(ux);
				if (model->quit) break; // Done
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
				}
				else {
					// Get more buffered user input
					std::string user_input{};
					std::getline(std::cin,user_input);
					// for (auto ch : user_input) std::cout << " " << ch << ":" << static_cast<int>(ch); 
					std::for_each(user_input.begin(),user_input.end(),[this](char ch){
						this->in.push(KeyPress{ch});
					});
					this->in.push(KeyPress{'\n'});
				}
			}
			catch (std::exception& e) {
				std::cerr << "\nERROR: run() loop failed, exception=" << e.what() << std::flush;
			}
		}
		// std::cout << "\nREPL::run exit" << std::flush;
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
	auto avfm = SKV::XML::VATReturns::account_vat_form_mapping();
	for (auto const& p : avfm) std::cout << "\nvat_returns_map:" << p[0] << " " << p[2];
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

namespace SKV { 
	namespace XML {
		namespace VATReturns {
// Mapping between BAS Account numbers and VAT Returns form designation codes (SRU Codes as "bonus")
char const* ACCOUNT_VAT_CSV = R"(KONTO;BENÄMNING;MOMSKOD (MOMSRUTA);SRU
3305;Försäljning tjänster till land utanför EU;ÖTEU (40);7410
3521;Fakturerade frakter, EU-land;VTEU (35);7410
3108;Försäljning varor till annat EU-land, momsfri;VTEU (35);7410
2634;Utgående moms omvänd skattskyldighet, 6 %;UTFU3 (32);7369
2624;Utgående moms omvänd skattskyldighet, 12 %;UTFU2 (31);7369
2614;Utgående moms omvänd skattskyldighet, 25 %;UOS1 (30);7369
2635;Utgående moms import av varor, 6 %;UI6 (62);7369
2615;Utgående moms import av varor, 25 %;UI25 (60);7369
2625;Utgående moms import av varor, 12 %;UI12 (61);7369
2636;Utgående moms VMB 6 %;U3 (12);7369
2631;Utgående moms på försäljning inom Sverige, 6 %;U3 (12);7369
2630;Utgående moms, 6 %;U3 (12);7369
2633;Utgående moms för uthyrning, 6 %;U3 (12);7369
2632;Utgående moms på egna uttag, 6 %;U3 (12);7369
2626;Utgående moms VMB 12 %;U2 (11);7369
2622;Utgående moms på egna uttag, 12 %;U2 (11);7369
2621;Utgående moms på försäljning inom Sverige, 12 %;U2 (11);7369
2620;Utgående moms, 12 %;U2 (11);7369
2623;Utgående moms för uthyrning, 12 %;U2 (11);7369
2612;Utgående moms på egna uttag, 25 %;U1 (10);7369
2610;Utgående moms, 25 %;U1 (10);7369
2611;Utgående moms på försäljning inom Sverige, 25 %;U1 (10);7369
2616;Utgående moms VMB 25 %;U1 (10);7369
2613;Utgående moms för uthyrning, 25 %;U1 (10);7369
2650;Redovisningskonto för moms;R2 (49);7369
1650;Momsfordran;R1 (49);7261
3231;Försäljning inom byggsektorn, omvänd skattskyldighet moms;OTTU (41);7410
3003;Försäljning inom Sverige, 6 % moms;MP3 (05);7410
3403;Egna uttag momspliktiga, 6 %;MP3 (05);7410
3402;Egna uttag momspliktiga, 12 %;MP2 (05);7410
3002;Försäljning inom Sverige, 12 % moms;MP2 (05);7410
3401;Egna uttag momspliktiga, 25 %;MP1 (05);7410
3510;Fakturerat emballage;MP1 (05);7410
3600;Rörelsens sidointäkter (gruppkonto);MP1 (05);7410
3530;Fakturerade tull- och speditionskostnader m.m.;MP1 (05);7410
3520;Fakturerade frakter;MP1 (05);7410
3001;Försäljning inom Sverige, 25 % moms;MP1 (05);7410
3540;Faktureringsavgifter;MP1 (05);7410
3106;Försäljning varor till annat EU-land, momspliktig;MP1 (05);7410
3990;Övriga ersättningar och intäkter;MF (42);7413
3404;Egna uttag, momsfria;MF (42);7410
3004;Försäljning inom Sverige, momsfri;MF (42);7410
3980;Erhållna offentliga stöd m.m.;MF (42);7413
4516;Inköp av varor från annat EU-land, 12 %;IVEU (20);7512
4515;Inköp av varor från annat EU-land, 25 %;IVEU (20);7512
9021;Varuvärde Inlöp annat EG-land (Momsrapport ruta 20);IVEU (20);
4517;Inköp av varor från annat EU-land, 6 %;IVEU (20);7512
4415;Inköpta varor i Sverige, omvänd skattskyldighet, 25 % moms;IV (23);7512
4531;Inköp av tjänster från ett land utanför EU, 25 % moms;ITGLOB (22);7512
4532;Inköp av tjänster från ett land utanför EU, 12 % moms;ITGLOB (22);7512
4533;Inköp av tjänster från ett land utanför EU, 6 % moms;ITGLOB (22);7512
4537;Inköp av tjänster från annat EU-land, 6 %;ITEU (21);7512
4536;Inköp av tjänster från annat EU-land, 12 %;ITEU (21);7512
4535;Inköp av tjänster från annat EU-land, 25 %;ITEU (21);7512
4427;Inköpta tjänster i Sverige, omvänd skattskyldighet, 6 %;IT (24);7512
4426;Inköpta tjänster i Sverige, omvänd skattskyldighet, 12 %;IT (24);7512
2642;Debiterad ingående moms i anslutning till frivillig skattskyldighet;I (48);7369
2640;Ingående moms;I (48);7369
2647;Ingående moms omvänd skattskyldighet varor och tjänster i Sverige;I (48);7369
2641;Debiterad ingående moms;I (48);7369
2649;Ingående moms, blandad verksamhet;I (48);7369
2646;Ingående moms på uthyrning;I (48);7369
2645;Beräknad ingående moms på förvärv från utlandet;I (48);7369
3913;Frivilligt momspliktiga hyresintäkter;HFS (08);7413
3541;Faktureringsavgifter, EU-land;FTEU (39);7410
3308;Försäljning tjänster till annat EU-land;FTEU (39);7410
3542;Faktureringsavgifter, export;E (36);7410
3522;Fakturerade frakter, export;E (36);7410
3105;Försäljning varor till land utanför EU;E (36);7410
3211;Försäljning positiv VMB 25 %;BVMB (07);7410
3212;Försäljning negativ VMB 25 %;BVMB (07);7410
9022;Beskattningsunderlag vid import (Momsrapport Ruta 50);BI (50);
4545;Import av varor, 25 % moms;BI (50);7512
4546;Import av varor, 12 % moms;BI (50);7512
4547;Import av varor, 6 % moms;BI (50);7512
3740;Öres- och kronutjämning;A;7410)";
		} // namespace VATReturns
	} // namespace XML
} // namespace SKV 

namespace CP435 {

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

namespace SKV {
	namespace XML {
		SKV::XML::XMLMap skv_xml_template{
		{R"(Skatteverket.agd:Avsandare.agd:Programnamn)",R"(Programmakarna AB)"}
		,{R"(Skatteverket.agd:Avsandare.agd:Organisationsnummer)",R"(190002039006)"}
		,{R"(Skatteverket.agd:Avsandare.agd:TekniskKontaktperson.agd:Namn)",R"(Valle Vadman)"}
		,{R"(Skatteverket.agd:Avsandare.agd:TekniskKontaktperson.agd:Telefon)",R"(23-2-4-244454)"}
		,{R"(Skatteverket.agd:Avsandare.agd:TekniskKontaktperson.agd:Epostadress)",R"(valle.vadman@programmakarna.se)"}
		,{R"(Skatteverket.agd:Avsandare.agd:Skapad)",R"(2021-01-30T07:42:25)"}

		,{R"(Skatteverket.agd:Blankettgemensamt.agd:Arbetsgivare.agd:AgRegistreradId)",R"(165560269986)"}
		,{R"(Skatteverket.agd:Blankettgemensamt.agd:Arbetsgivare.agd:Kontaktperson.agd:Namn)",R"(Ville Vessla)"}
		,{R"(Skatteverket.agd:Blankettgemensamt.agd:Arbetsgivare.agd:Kontaktperson.agd:Telefon)",R"(555-244454)"}
		,{R"(Skatteverket.agd:Blankettgemensamt.agd:Arbetsgivare.agd:Kontaktperson.agd:Epostadress)",R"(ville.vessla@foretaget.se)"}

		,{R"(Skatteverket.agd:Kontaktperson.agd:Arendeinformation.agd:Arendeagare)",R"(165560269986)"}
		,{R"(Skatteverket.agd:Kontaktperson.agd:Arendeinformation.agd:Period)",R"(202101)"}

		,{R"(Skatteverket.agd:Kontaktperson.agd:Blankettinnehall.agd:HU.agd:ArbetsgivareHUGROUP.agd:AgRegistreradId faltkod="201")",R"(16556026998)"}
		,{R"(Skatteverket.agd:Kontaktperson.agd:Blankettinnehall.agd:HU.agd:RedovisningsPeriod faltkod="006")",R"(202101)"}
		,{R"(Skatteverket.agd:Kontaktperson.agd:Blankettinnehall.agd:HU.agd:SummaArbAvgSlf faltkod="487")",R"(0)"}
		,{R"(Skatteverket.agd:Kontaktperson.agd:Blankettinnehall.agd:HU.agd:SummaSkatteavdr faltkod="497")",R"(0)"}

		,{R"(Skatteverket.agd:Blankett.agd:Arendeinformation.agd:Arendeagare)",R"(165560269986)"}
		,{R"(Skatteverket.agd:Blankett.agd:Arendeinformation.agd:Period)",R"(202101)"}

		,{R"(Skatteverket.agd:Blankett.agd:Blankettinnehall.agd:IU.agd:ArbetsgivareIUGROUP.agd:AgRegistreradId faltkod="201"))",R"(165560269986)"}
		,{R"(Skatteverket.agd:Blankett.agd:Blankettinnehall.agd:IU.agd:BetalningsmottagareIUGROUP.agd:BetalningsmottagareIDChoice.agd:BetalningsmottagarId faltkod="215")",R"(198202252386)"}
		,{R"(Skatteverket.agd:Blankett.agd:Blankettinnehall.agd:IU.agd:RedovisningsPeriod faltkod="006")",R"(202101)"}
		,{R"(Skatteverket.agd:Blankett.agd:Blankettinnehall.agd:IU.agd:Specifikationsnummer faltkod="570")",R"(001)"}
		,{R"(Skatteverket.agd:Blankett.agd:Blankettinnehall.agd:IU.agd:AvdrPrelSkatt faltkod="001")",R"(0)"}
		};
	}
}
