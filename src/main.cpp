#include <iostream>
#include <locale>
#include <string>
#include <sstream>
#include <queue>
#include <variant>
#include <vector>
#include <optional>
#include <string_view>

#if __has_include(<filesystem>)
#	include <filesystem>
#elif __has_include(<experimental/filesystem>)
#  include <experimental/filesystem>
#  error DESIGN INSUFFICIENCY <experimental/filesystem>
#endif

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

template <typename I>
std::vector<std::pair<I,I>> to_ranges(std::vector<I> line_nos) {
	std::vector<std::pair<I,I>> result{};
	if (line_nos.size()>0) {
		I begin{line_nos[0]}; 
		I previous{begin};
		for (auto line_ix : line_nos) {
			if (line_ix > previous+1) {
				// Broken sequence - push previous one
				result.push_back({begin,previous});
				begin = line_ix;
			}
			previous = line_ix;
		}
		if (previous > begin) result.push_back({begin,previous});
	}
	return result;
}

template <typename I>
std::ostream& operator<<(std::ostream& os,std::vector<std::pair<I,I>> const& rr) {
	for (auto const& r : rr) {
		if (r.first == r.second) os << " " << r.first;
		else os << " [" << r.first << ".." << r.second << "]";
	}
	return os;
}

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
		,YES
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
		}; // class Path

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

auto utf_ignore_to_upper_f = [](char ch) {
	if (ch <= 0x7F) return static_cast<char>(std::toupper(ch));
	return ch;
};

std::string utf_ignore_to_upper(std::string const& s) {
	std::string result{};
	std::transform(s.begin(),s.end(),std::back_inserter(result),utf_ignore_to_upper_f);
	return result;
}

std::vector<std::string> utf_ignore_to_upper(std::vector<std::string> const& tokens) {
	std::vector<std::string> result{};
	auto f = [](std::string s) {return utf_ignore_to_upper(s);};
	std::transform(tokens.begin(),tokens.end(),std::back_inserter(result),f);
	return result;
}

bool strings_share_tokens(std::string const& s1,std::string const& s2) {
	bool result{false};
	auto s1_words = utf_ignore_to_upper(tokenize::splits(s1));
	auto s2_words = utf_ignore_to_upper(tokenize::splits(s2));
	for (int i=0; (i < s1_words.size()) and !result;++i) {
		for (int j=0; (j < s2_words.size()) and !result;++j) {
			result = (s1_words[i] == s2_words[j]);
		}
	}
	return result;
}

bool first_in_second_case_insensitive(std::string const& s1, std::string const& s2) {
	auto upper_s1 = utf_ignore_to_upper(s1);
	auto upper_s2 = utf_ignore_to_upper(s2);
	return (upper_s2.find(upper_s1) != std::string::npos);
}

namespace SRU {
	using AccountNo = unsigned int;
	using OptionalAccountNo = std::optional<AccountNo>;
}

using Amount= float;
using OptionalAmount = std::optional<Amount>;

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
	try {
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
	}
	catch (std::exception const& e) {} // swallow silently
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

OptionalAmount to_amount(std::string const& sAmount) {
	// std::cout << "\nto_amount " << std::quoted(sAmount);
	OptionalAmount result{};
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

	extern char const* bas_2022_account_plan_csv; // See bottom of this file

	namespace detail {
		// "hidden" poor mans singleton instance creation
		Key::Paths bas_2022_account_plan_paths{};
		Key::Paths to_bas_2022_account_plan_paths() {
			Key::Paths result;
			std::istringstream in{bas_2022_account_plan_csv};
			// TODO: Parse in and assemble a path with nodes:
			// One node per digit in account number?
			// Or each node an optional<string> to handle digit position that is not a "grouping" level?
			// first digit grouping: Always
			// Second digit grouping: sometimes
			// Third digit grouping: Always?
			// ...
			return result;
		}
	}
	Key::Paths const& to_bas_2022_account_plan_paths() {
		// Poor mans "singleton" instance
		static auto const& result = detail::to_bas_2022_account_plan_paths();
		return result;
	}

	void parse_bas_account_plan_csv(std::istream& in,std::ostream& prompt) {
		std::string entry{};
		int line_ix{0};
		std::map<int,int> count_distribution{};
		using field_ix = int;
		using line_no = int;
		std::map<field_ix,std::map<std::string,std::vector<line_no>>> fields_map{};
		while (std::getline(in,entry)) {
			prompt << "\n" << line_ix << entry;
			auto fields = tokenize::splits(entry,';',tokenize::eAllowEmptyTokens::YES);
			int field_ix{0};
			prompt << "\n\tcount:" << fields.size();
			++count_distribution[fields.size()];
			for (auto const& field : fields) {
				prompt << "\n\t  " << field_ix << ":" << std::quoted(field);
				fields_map[field_ix][field].push_back(line_ix);
				++field_ix;
			}
			++line_ix;
		}
		prompt << "\nField Count distribution";
		for (auto const& entry : count_distribution) prompt << "\nfield count:" << entry.first << " entry count:" << entry.second;
		prompt << "\nField Distribution";
		for (auto const& [field_ix,field_map] : fields_map) {
			prompt << "\n\tindex:" << field_ix;
			for (auto const& [field,line_nos] : field_map) {
				prompt << "\n\t  field:" << std::quoted(field) << " line:";
				auto rr = to_ranges(line_nos);
				prompt << rr;
				if (line_nos.size()>0) {
					line_no begin{line_nos[0]}; 
					line_no previous{begin};
					for (auto line_ix : line_nos) {
						if (line_ix > previous+1) {
							// Broken sequence - log previous one
							if (begin == previous) prompt << " " << begin;
							else prompt << " [" << begin << ".." << previous << "]";
							begin = line_ix;
						}
						// prompt << "<" << line_ix << ">";
						previous = line_ix;
					}
					// Log last range
					if (begin == previous) prompt << " " << previous;
					else prompt << " [" << begin << ".." << previous << "]";
				}
			}
		}
	}

	Amount cents_amount(Amount amount) {
		return std::round(amount*100.0)/100.0;
	}

	enum class AccountKind {
		// Account type is specified as T, S, K or I (asset, debt, cost or income)
		// Swedish: tillgång, skuld, kostnad eller intäkt
		Unknown
		,Asset 	// + or Debit for MORE assets
		,Debt		// - or credit for MORE debt 
		,Income // - or Credit for MORE income
		,Cost		// + or Debit for MORE cost
		,Result // - or Credit for MORE result (counter to Assets)
		,Undefined
	};
	using OptionalAccountKind = std::optional<AccountKind>;

	OptionalAccountKind to_account_kind(BASAccountNo const& bas_account_no) {
		OptionalAccountKind result{};
		auto s_account_no = std::to_string(bas_account_no);
		if (s_account_no.size() == 4) {
			switch (s_account_no[0]) {
				case '1': {result = AccountKind::Asset;} break;
				case '2': {result = AccountKind::Debt;} break;
				case '3': {result = AccountKind::Income;} break;
				case '4':
				case '5':
				case '6':
				case '7': {result = AccountKind::Cost;} break;
				case '8': {result = AccountKind::Result;} break;
				case '9': {/* NOT a BAS account (free for use for use as seen fit) */} break;
			}
		}
		return result;
	}

	using BASAccountNumberPath = Key::Path;
	BASAccountNumberPath to_bas__account_number_path(BASAccountNo const& bas_account_no) {
		BASAccountNumberPath result{};
		// TODO: Search 
		return result;
	}

	struct AccountMeta {
		std::string name{};
		OptionalAccountKind account_kind{};
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

		template <typename T>
		struct JournalEntry_t {
			std::string caption{};
			Date date{};
			T account_transactions;
		};
		using JournalEntry = JournalEntry_t<AccountTransactions>;
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
} // namespace BAS

// Forward (TODO: Reorganise if/when splitting into proper header/cpp file structure)
Amount to_positive_gross_transaction_amount(BAS::anonymous::JournalEntry const& aje);
Amount to_negative_gross_transaction_amount(BAS::anonymous::JournalEntry const& aje);
void for_each_anonymous_account_transaction(BAS::anonymous::JournalEntry const& aje,auto& f);

namespace BAS {

	Amount mats_sum(BAS::MetaAccountTransactions const& mats) {
		return std::accumulate(mats.begin(),mats.end(),Amount{},[](Amount acc,BAS::MetaAccountTransaction const& mat){
			acc += mat.defacto.amount;
			return acc;
		});
	}

	using MatchesMetaEntry = std::function<bool(BAS::MetaEntry const& me)>;

	OptionalBASAccountNo to_account_no(std::string const& s) {
		OptionalBASAccountNo result{};
		try {
			if (s.size()==4) {
				if (std::all_of(s.begin(),s.end(),::isdigit)) {
					auto account_no = std::stoi(s);
					if (account_no >= 1000) result = account_no;
				}
			}
		}
		catch (std::exception const& e) { std::cerr << "\nDESIGN INSUFFICIENCY: to_account_no(" << s << ") failed. Exception=" << std::quoted(e.what());}
		return result;
	}

	OptionalJournalEntryMeta to_journal_meta(std::string const& s) {
		OptionalJournalEntryMeta result{};
		try {
			const std::regex meta_regex("[A-Z]\\d+"); // series followed by verification number
			if (std::regex_match(s,meta_regex)) result = JournalEntryMeta{
				.series = s[0]
				,.verno = static_cast<VerNo>(std::stoi(s.substr(1)))};
		}
		catch (std::exception const& e) { std::cerr << "\nDESIGN INSUFFICIENCY: to_journal_meta(" << s << ") failed. Exception=" << std::quoted(e.what());}
		return result;
	}

	auto has_greater_amount = [](BAS::anonymous::AccountTransaction const& at1,BAS::anonymous::AccountTransaction const& at2) {
		return (at1.amount > at2.amount);
	};

	BAS::MetaEntry& sort(BAS::MetaEntry& me,auto& comp) {
		std::sort(me.defacto.account_transactions.begin(),me.defacto.account_transactions.end(),comp);
		return me;
	}

	namespace filter {
		struct is_series {
			BAS::Series required_series;
			bool operator()(MetaEntry const& me) {
				return (me.meta.series == required_series);
			}
		};

		class HasGrossAmount {
		public:
			HasGrossAmount(Amount gross_amount) : m_gross_amount(gross_amount) {}
			bool operator()(BAS::MetaEntry const& me) {
				if (m_gross_amount<0) {
					return (to_negative_gross_transaction_amount(me.defacto) == m_gross_amount);
				}
				else {
					return (to_positive_gross_transaction_amount(me.defacto) == m_gross_amount);
				}
			}
		private:
			Amount m_gross_amount;
		};

		class HasTransactionToAccount {
		public:
			HasTransactionToAccount(BASAccountNo bas_account_no) : m_bas_account_no(bas_account_no) {}
			bool operator()(BAS::MetaEntry const& me) {
				return std::any_of(me.defacto.account_transactions.begin(),me.defacto.account_transactions.end(),[this](BAS::anonymous::AccountTransaction const& at){
					return (at.account_no == this->m_bas_account_no);
				});
			}
		private:
			BASAccountNo m_bas_account_no;
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
	} // namespace filter

	// TYPED Journal Entries (to identify patterns of interest in how the individual account transactions of an entry is dispositioned in amount and on semantics of the account)
	namespace anonymous {
		using AccountTransactionType = std::set<std::string>;
		using TypedAccountTransactions = std::map<BAS::anonymous::AccountTransaction,AccountTransactionType>;
		using TypedAccountTransaction = TypedAccountTransactions::value_type;
		using TypedJournalEntry = BAS::anonymous::JournalEntry_t<TypedAccountTransactions>;
	}
	using TypedMetaEntry = MetaDefacto<JournalEntryMeta,anonymous::TypedJournalEntry>;
	using TypedMetaEntries = std::vector<TypedMetaEntry>;

	void for_each_typed_account_transaction(BAS::TypedMetaEntry const& tme,auto& f) {
		for (auto const& tat : tme.defacto.account_transactions) {
			f(tat);
		}
	}

	namespace kind {

		using BASAccountTopology = std::set<BASAccountNo>;
		using AccountTransactionTypeTopology = std::set<std::string>;

		enum class ATType {
			undefined
			,transfer
			,eu_purchase
			,gross
			,net
			,eu_vat
			,vat
			,cents
			,unknown
		};

		ATType to_at_type(std::string const& prop) {
			ATType result{ATType::undefined};
			static const std::map<std::string,ATType> AT_TYPE_TO_ID_MAP{
				{"",ATType::undefined}
				,{"transfer",ATType::transfer}
				,{"eu_purchase",ATType::eu_purchase}
				,{"gross",ATType::gross}
				,{"net",ATType::net}
				,{"eu_vat",ATType::eu_vat}
				,{"vat",ATType::vat}
				,{"cents",ATType::cents}
			};
			if (AT_TYPE_TO_ID_MAP.contains(prop)) {
				result = AT_TYPE_TO_ID_MAP.at(prop);
			}
			else {
				result = ATType::unknown;
			}
			return result;
		}

		std::size_t to_at_types_order(BAS::kind::AccountTransactionTypeTopology const& topology) {
			std::size_t result{};
			std::vector<ATType> at_types{};
			for (auto const& prop : topology) at_types.push_back(to_at_type(prop));
			std::sort(at_types.begin(),at_types.end(),[](ATType t1,ATType t2){
				return (t1>t2);
			});
			// Assemble a "number" of "digits" each having value 0..ATType::unknown (Assumes C++ enumeration can be trusted to start at 0...)
			for (auto at_type : at_types) result = result*static_cast<std::size_t>(ATType::unknown) + static_cast<std::size_t>(at_type);
			return result;
		}

		std::vector<std::string> sorted(AccountTransactionTypeTopology const& topology) {
			std::vector<std::string> result{topology.begin(),topology.end()};
			std::sort(result.begin(),result.end(),[](auto const& s1,auto const& s2){
				return (to_at_type(s1) < to_at_type(s2));
			});
			return result;
		}

		namespace detail {
			template <typename T>
			struct hash {};

			template <>
			struct hash<BASAccountTopology> {
				std::size_t operator()(BASAccountTopology const& bat) {
					std::size_t result{};
					for (auto const& account_no : bat) {
						auto h = std::hash<BASAccountNo>{}(account_no);
						result = result ^ (h << 1);
					}
					return result;
				}	
			};

			template <>
			struct hash<AccountTransactionTypeTopology> {
				std::size_t operator()(AccountTransactionTypeTopology const& props) {
					std::size_t result{};
					for (auto const& prop : props) {
						auto h = std::hash<std::string>{}(prop);
						result = result ^ (h << 1);
					}
					return result;
				}	
			};
		} // namespace detail

		BASAccountTopology to_accounts_topology(MetaEntry const& me) {
			BASAccountTopology result{};
			auto f = [&result](BAS::anonymous::AccountTransaction const& at) {
				result.insert(at.account_no);
			};
			for_each_anonymous_account_transaction(me.defacto,f);
			return result;
		}

		BASAccountTopology to_accounts_topology(TypedMetaEntry const& tme) {
			BASAccountTopology result{};
			auto f = [&result](BAS::anonymous::TypedAccountTransaction const& tat) {
				auto const& [at,props] = tat;
				result.insert(at.account_no);
			};
			for_each_typed_account_transaction(tme,f);
			return result;
		}

		AccountTransactionTypeTopology to_types_topology(TypedMetaEntry const& tme) {
			AccountTransactionTypeTopology result{};
			auto f = [&result](BAS::anonymous::TypedAccountTransaction const& tat) {
				auto const& [at,props] = tat;
				for (auto const& prop : props) result.insert(prop);
			};
			for_each_typed_account_transaction(tme,f);
			return result;
		}

		std::size_t to_signature(BASAccountTopology const& bat) {
			return detail::hash<BASAccountTopology>{}(bat);
		}

		std::size_t to_signature(AccountTransactionTypeTopology const& met) {
			return detail::hash<AccountTransactionTypeTopology>{}(met);
		}

	} // namespace kind

	namespace group {

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

std::ostream& operator<<(std::ostream& os,BAS::anonymous::AccountTransactions const& ats) {
	for (auto const& at : ats) {
		os << "\n\t" << at; 
	}
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::anonymous::JournalEntry const& aje) {
	os << std::quoted(aje.caption) << " " << aje.date;
	os << aje.account_transactions;
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

// TYPED ENTRY

std::ostream& operator<<(std::ostream& os,BAS::kind::BASAccountTopology const& accounts) {
	if (accounts.size()==0) os << " ?";
	else for (auto const& account : accounts) {
		os << " " << account;
	}
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::kind::AccountTransactionTypeTopology const& props) {
	auto sorted_props = BAS::kind::sorted(props); // props is a std::set, sorted_props is a vector
	if (sorted_props.size()==0) os << " ?";
	else for (auto const& prop : sorted_props) {
		os << " " << prop;
	}
	os << " sort_code:" << BAS::kind::to_at_types_order(props);
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::anonymous::TypedAccountTransaction const& tat) {
	auto const& [at,props] = tat;
	os << props << " : " << at;
	return os;
}

template <typename T>
struct IndentedOnNewLine{
	IndentedOnNewLine(T const& val,int count) : val{val},count{count} {}
	T const& val;
	int count;
};

std::ostream& operator<<(std::ostream& os,IndentedOnNewLine<BAS::anonymous::TypedAccountTransactions> const& indented) {
	for (auto const& at : indented.val) {
		os << "\n";
		for (int x = 0; x < indented.count; ++x) os << ' ';
		os << at;
	}
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::anonymous::TypedJournalEntry const& tje) {
	os << std::quoted(tje.caption) << " " << tje.date;
	for (auto const& tat : tje.account_transactions) {
		os << "\n\t" << tat;
	}
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::TypedMetaEntry const& tme) {
	os << tme.meta << " " << tme.defacto;
	return os;
}

// JOURNAL

using BASJournal = std::map<BAS::VerNo,BAS::anonymous::JournalEntry>;
using BASJournals = std::map<char,BASJournal>; // Swedish BAS Journals named "Series" and labeled A,B,C,...

class ToNetVatAccountTransactions {
public:

	ToNetVatAccountTransactions(BAS::anonymous::AccountTransaction const& net_at, BAS::anonymous::AccountTransaction const& vat_at)
		:  m_net_at{net_at}
		  ,m_vat_at{vat_at}
			,m_gross_vat_rate{static_cast<Amount>((net_at.amount != 0)?vat_at.amount/(net_at.amount + vat_at.amount):1.0)}
			,m_sign{(net_at.amount<0)?-1.0f:1.0f} /* 0 gets sign + */ {}

	BAS::anonymous::AccountTransactions operator()(Amount remaining_counter_amount,std::string const& transtext,OptionalAmount const& inc_vat_amount = std::nullopt) {
		BAS::anonymous::AccountTransactions result{};
		Amount gross_amount = (inc_vat_amount)?*inc_vat_amount:remaining_counter_amount;
		BAS::anonymous::AccountTransaction net_at{
			.account_no = m_net_at.account_no
			,.transtext = transtext
			,.amount = BAS::cents_amount(static_cast<Amount>(m_sign * gross_amount * (1.0-m_gross_vat_rate)))
		};
		BAS::anonymous::AccountTransaction vat_at{
			.account_no = m_vat_at.account_no
			,.transtext = transtext
			,.amount = BAS::cents_amount(static_cast<Amount>(m_sign * gross_amount * m_gross_vat_rate))
		};
		result.push_back(net_at);
		result.push_back(vat_at);
		return result;
	}
private:
	BAS::anonymous::AccountTransaction m_net_at;
	BAS::anonymous::AccountTransaction m_vat_at;
	float m_gross_vat_rate;
	float m_sign;
};

struct HeadingAmountDateTransEntry {
	std::string heading{};
	Amount amount;
	Date date{};
	BAS::OptionalMetaEntry current_candidate{};
	std::optional<ToNetVatAccountTransactions> counter_ats_producer{};
};

std::ostream& operator<<(std::ostream& os,HeadingAmountDateTransEntry const& had) {
	if (auto me = had.current_candidate) {
		os << '*';
	}
	else {
		os << ' ';
	}
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
				auto tokens = tokenize::splits(sEntry,';',tokenize::eAllowEmptyTokens::YES);
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

	HeadingAmountDateTransEntries from_stream(auto& in,OptionalBASAccountNo gross_bas_account_no = std::nullopt) {
		HeadingAmountDateTransEntries result{};
		parse_TRANS(in); // skip first line with field names
		while (auto had = parse_TRANS(in)) {
			if (gross_bas_account_no) {
				std::cout << "\nfrom_stream to gross_bas_account_no:" << *gross_bas_account_no;
				// Add a template with the gross amount transacted to provided bas account no
				BAS::MetaEntry me{
					.meta = {
						.series = 'A'
					}
					,.defacto = {
						.caption = had->heading
						,.date = had->date
					}
				};
				BAS::anonymous::AccountTransaction gross_at{
					.account_no = *gross_bas_account_no
					,.amount = had->amount
				};
				me.defacto.account_transactions.push_back(gross_at);
				had->current_candidate = me;
			}
			result.push_back(*had);
		}
		return result;
	}
} // namespace CSV

using char16_t_string = std::wstring;

namespace charset {

	namespace CP435 {
		extern std::map<char,char16_t> cp435ToUnicodeMap;

		char16_t cp435ToUnicode(char ch435) {
			return cp435ToUnicodeMap[ch435];
		}
	
		uint8_t UnicodeToCP435(char16_t unicode) {
			uint8_t result{'?'};
			auto iter = std::find_if(cp435ToUnicodeMap.begin(),cp435ToUnicodeMap.end(),[&unicode](auto const& entry){
				return entry.second == unicode;
			});
			if (iter != cp435ToUnicodeMap.end()) result = iter->first;
			return result;
		} 

		char16_t_string cp435ToUnicode(std::string s435) {
			char16_t_string result{};
			std::transform(s435.begin(),s435.end(),std::back_inserter(result),[](char ch){
				return charset::CP435::cp435ToUnicode(ch);
			});
			return result;
		}
	} // namespace CP435
} // namespace CharSet

namespace encoding {
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

		encoding::UTF8::Stream& operator<<(encoding::UTF8::Stream& os,char32_t cp) {
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
			encoding::UTF8::Stream utf8_os{os};
			for (auto cp : s) utf8_os << cp;
			return os.str();
		}

		// UTF-8 to Unicode
		class ToUnicodeBuffer {
		public:
			using OptionalUnicode = std::optional<uint16_t>;
			OptionalUnicode push(uint8_t b) {
				OptionalUnicode result{};
				// See https://en.wikipedia.org/wiki/UTF-8#Encoding
				// Code point <-> UTF-8 conversion
				// First code point	Last code point	Byte 1		Byte 2    Byte 3    Byte 4
				// U+0000	U+007F										0xxxxxxx	
				// U+0080	U+07FF										110xxxxx	10xxxxxx	
				// U+0800	U+FFFF										1110xxxx	10xxxxxx	10xxxxxx	
				// U+10000	[nb 2]U+10FFFF					11110xxx	10xxxxxx	10xxxxxx	10xxxxxx
				this->m_utf_8_buffer.push_back(b);
				return this->to_unicode();
			}
		private:
			std::deque<uint8_t> m_utf_8_buffer{};
			OptionalUnicode to_unicode() {
				OptionalUnicode result{};
				switch (m_utf_8_buffer.size()) {
					case 1: {
						if (m_utf_8_buffer[0] < 0x7F) {
							result = m_utf_8_buffer[0];
							m_utf_8_buffer.clear();
						}
					} break;
					case 2: {
						if (m_utf_8_buffer[0]>>5 == 0b110) {
							uint16_t wch = (m_utf_8_buffer[0] & 0b00011111) << 6;
							wch += (m_utf_8_buffer[1] & 0b00111111);
							result = wch;
							m_utf_8_buffer.clear();
						}
					} break;
					case 3: {
						if (m_utf_8_buffer[0]>>4 == 0b1110) {
							uint16_t wch = (m_utf_8_buffer[0] & 0b00001111) << 6;
							wch += (m_utf_8_buffer[1] & 0b00111111);
							wch = (wch << 6) + (m_utf_8_buffer[2] & 0b00111111);
							result = wch;
							m_utf_8_buffer.clear();
						}
					} break;
					default: {
						// We don't support Unicodes over the range U+0800	U+FFFF
						// Clean out any encoding bytes
						while (m_utf_8_buffer.size()>0 and m_utf_8_buffer.front() > 0x7F) m_utf_8_buffer.pop_front();
						result = to_unicode(); // Recurse with the new buffer
					}
				}
				return result;
			}
		};

	} // namespace UTF8
} // namespace encoding

namespace SIE {

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

	SIEParseResult parse_ORGNR(std::istream& in,std::string const& konto_tag) {
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

	SIEParseResult parse_FNAMN(std::istream& in,std::string const& konto_tag) {
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

	SIEParseResult parse_ADRESS(std::istream& in,std::string const& konto_tag) {
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

	SIEParseResult parse_KONTO(std::istream& in,std::string const& konto_tag) {
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
			auto sUnicode = charset::CP435::cp435ToUnicode(s437);
			konto.name = encoding::UTF8::unicode_to_utf8(sUnicode);

			result = konto;
			pos = in.tellg(); // accept new stream position
		}
		in.seekg(pos); // Reset position in case of failed parse
		in.clear(); // Reset failbit to allow try for other parse
		return result;		
	}

	SIEParseResult parse_SRU(std::istream& in,std::string const& sru_tag) {
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

	SIEParseResult parse_TRANS(std::istream& in,std::string const& trans_tag) {
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
				auto sUnicode = charset::CP435::cp435ToUnicode(s437);
				trans.transtext = encoding::UTF8::unicode_to_utf8(sUnicode);
			}

			result = trans;
			pos = in.tellg(); // accept new stream position
		}
		in.seekg(pos); // Reset position in case of failed parse
		in.clear(); // Reset failbit to allow try for other parse
		return result;		
	}

	SIEParseResult parse_Tag(std::istream& in,std::string const& tag) {
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

	SIEParseResult parse_VER(std::istream& in) {
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
			auto sUnicode = charset::CP435::cp435ToUnicode(s437);
			ver.vertext = encoding::UTF8::unicode_to_utf8(sUnicode);

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

	SIEParseResult parse_any_line(std::istream& in) {
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

	/**
	 * NOTE ABOUT UTF-8 TO Code Page 435 used as the character set of an SIE file
	 * 
	 * The convertion is made in overloaded operator<<(SIE::OStream& sieos,char ch) called by operator<<(SIE::OStream& sieos,std::string s).
	 * 
	 * But I have not yet overloaded operator<<(SIE::OStream& for things like Amount, account_no etc.
	 * SO - basically it is mainly transtext and vertext that is fed to the operator<<(SIE::OStream& sieos,std::string s).
	 * TAKE CARE to not mess this up or you will get UTF-8 encoded text into the SIE file that will mess things up quite a lot...
	 */


	struct OStream {
		std::ostream& os;
		encoding::UTF8::ToUnicodeBuffer to_unicode_buffer{};
	};

	SIE::OStream& operator<<(SIE::OStream& sieos,char ch) {
		// Assume s is in UTF-8 and convert it to CP435 charachter set in file
		if (auto unicode = sieos.to_unicode_buffer.push(ch)) {
			auto cp435_ch = charset::CP435::UnicodeToCP435(*unicode);
			sieos.os.put(cp435_ch);
		}
		return sieos;
	}

	SIE::OStream& operator<<(SIE::OStream& sieos,std::string s) {
		for (char ch : s) {
			sieos << ch; // Stream through operator<<(SIE::OStream& sieos,char ch) that will tarnsform utf-8 encoded Unicode, to char encoded CP435
		}
		return sieos;
	}

	SIE::OStream& operator<<(SIE::OStream& sieos,SIE::Trans const& trans) {
		// #TRANS account no {object list} amount transdate transtext quantity sign
		//                                           o          o        o      o
		// #TRANS 1920 {} -890 "" "" 0
		sieos.os << "\n#TRANS"
		<< " " << trans.account_no
		<< " " << "{}"
		<< " " << trans.amount
		<< " " << std::quoted("");
		if (trans.transtext) sieos << " \"" << *trans.transtext << "\"";
		else sieos.os << " " << std::quoted("");
		return sieos;
	}
	
	SIE::OStream& operator<<(SIE::OStream& sieos,SIE::Ver const& ver) {
		// #VER A 1 20210505 "M�nadsavgift PG" 20210817	
		sieos.os << "\n#VER" 
		<< " " << ver.series 
		<< " " << ver.verno
		<< " " << to_string(ver.verdate); // TODO: make compiler find operator<< above (why can to_string use it but we can't?)
		sieos << " \"" << ver.vertext << "\"";
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

Amount to_negative_gross_transaction_amount(BAS::anonymous::JournalEntry const& aje) {
	Amount result = std::accumulate(aje.account_transactions.begin(),aje.account_transactions.end(),Amount{},[](Amount acc,BAS::anonymous::AccountTransaction const& account_transaction){
		acc += (account_transaction.amount<0)?account_transaction.amount:0;
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

class AccountTransactionTemplate {
public:
	AccountTransactionTemplate(Amount gross_amount,BAS::anonymous::AccountTransaction const& at) 
		:  m_at{at}
			,m_percent{static_cast<int>(std::round(at.amount*100 / gross_amount))}  {}
	BAS::anonymous::AccountTransaction operator()(Amount amount) const {
		// BAS::anonymous::AccountTransaction result{.account_no = m_account_no,.transtext="",.amount=amount*m_factor};
		BAS::anonymous::AccountTransaction result{
			 .account_no = m_at.account_no
			,.transtext = m_at.transtext
			,.amount=static_cast<Amount>(std::round(amount*m_percent)/100.0)};
		return result;
	}
	int percent() const {return m_percent;}
private:
	BAS::anonymous::AccountTransaction m_at;
	int m_percent;
	friend std::ostream& operator<<(std::ostream& os,AccountTransactionTemplate const& att);
};
using AccountTransactionTemplates = std::vector<AccountTransactionTemplate>;

class JournalEntryTemplate {
public:

	JournalEntryTemplate(BAS::Series series,BAS::MetaEntry const& me) : m_series{series} {
		auto gross_amount = to_positive_gross_transaction_amount(me.defacto);
		if (gross_amount >= 0.01) {
			std::transform(me.defacto.account_transactions.begin(),me.defacto.account_transactions.end(),std::back_inserter(templates),[gross_amount](BAS::anonymous::AccountTransaction const& at){
				AccountTransactionTemplate result{gross_amount,at};
				return result;
			});
			std::sort(this->templates.begin(),this->templates.end(),[](auto const& e1,auto const& e2){
				return (std::abs(e1.percent()) > std::abs(e2.percent())); // greater to lesser
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

BAS::MetaEntry to_meta_entry(BAS::TypedMetaEntry const& tme) {
	BAS::MetaEntry result {
		.meta = tme.meta
		,.defacto = {
			.caption = tme.defacto.caption
			,.date = tme.defacto.date
		}
	};
	for (auto const& [at,props] : tme.defacto.account_transactions) {
		result.defacto.account_transactions.push_back(at);
	}
	return result;
}

OptionalJournalEntryTemplate to_template(BAS::TypedMetaEntry const& tme) {
	return to_template(to_meta_entry(tme));
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

std::ostream& operator<<(std::ostream& os,AccountTransactionTemplate const& att) {
	os << "\n\t" << att.m_at.account_no << " " << att.m_percent;
	return os;
}

std::ostream& operator<<(std::ostream& os,JournalEntryTemplate const& entry) {
	os << "template: series " << entry.series();
	std::for_each(entry.templates.begin(),entry.templates.end(),[&os](AccountTransactionTemplate const& att){
		os << "\n\t" << att;
	});
	return os;
}

bool had_matches_trans(HeadingAmountDateTransEntry const& had,BAS::anonymous::JournalEntry const& aje) {
	return strings_share_tokens(had.heading,aje.caption);
}

// ==================================================================================

bool same_whole_units(Amount const& a1,Amount const& a2) {
	return (std::round(a1) == std::round(a2));
}

BAS::MetaEntry updated_entry(BAS::MetaEntry const& me,BAS::anonymous::AccountTransaction const& at) {
	std::cout << "\nupdated_entry";
	std::cout << "\nme:" << me;
	std::cout << "\nat:" << at;
	
	BAS::MetaEntry result{me};
	BAS::sort(result,BAS::has_greater_amount);
	auto iter = std::find_if(result.defacto.account_transactions.begin(),result.defacto.account_transactions.end(),[&at](auto const& entry){
		return (entry.account_no == at.account_no);
	});
	if (iter == result.defacto.account_transactions.end()) {
		result.defacto.account_transactions.push_back(at);
		result = updated_entry(result,at); // recurse with added entry
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
	std::cout << "\nresult:" << result;
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
	BAS::MetaEntry add(BAS::MetaEntry me) {
		BAS::MetaEntry result{me};
		// Ensure a valid series
		if (me.meta.series < 'A' or 'M' < me.meta.series) {
			me.meta.series = 'A';
			std::cerr << "\nadd(me) assigned series 'A' to entry with no series assigned";
		}
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

auto to_typed_meta_entry = [](BAS::MetaEntry const& me) -> BAS::TypedMetaEntry {
	BAS::anonymous::TypedAccountTransactions typed_ats{};
	auto gross_amount = to_positive_gross_transaction_amount(me.defacto);

	// Direct type detection based on gross_amount and account meta data
	for (auto const& at : me.defacto.account_transactions) {
		if (std::round(std::abs(at.amount)) == std::round(gross_amount)) typed_ats[at].insert("gross");
		if (is_vat_account_at(at)) typed_ats[at].insert("vat");
		if (std::abs(at.amount) < 1) typed_ats[at].insert("cents");
		if (std::round(std::abs(at.amount)) == std::round(gross_amount / 2)) typed_ats[at].insert("transfer");
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
	// Finally add any still untyped at with empty property set
	for (auto const& at : me.defacto.account_transactions) {
		if (!typed_ats.contains(at)) typed_ats.insert({at,{}});
	}
	BAS::TypedMetaEntry result{
		.meta = me.meta
		,.defacto = {
			.caption = me.defacto.caption
			,.date = me.defacto.date
			,.account_transactions = typed_ats
		}
	};
	return result;
};

int to_vat_type(BAS::TypedMetaEntry const& tme) {
	static bool const log{false};
	int result{-1};
	// Count each type of property (NOTE: Can be less than transaction count as they may overlap, e.g., two or more gross account transactions)
	std::map<std::string,unsigned int> props_counter{};
	for (auto const& [at,props] : tme.defacto.account_transactions) {
		for (auto const& prop : props) props_counter[prop]++;
	}
	// LOG
	if (log) {
		for (auto const& [prop,count] : props_counter) {
			std::cout << "\n" << std::quoted(prop) << " count:" << count; 
		}
	}
	// Calculate total number of properties (NOTE: Can be more that the transactions as e.g., vat and eu_vat overlaps)
	auto props_sum = std::accumulate(props_counter.begin(),props_counter.end(),unsigned{0},[](auto acc,auto const& entry){
		acc += entry.second;
		return acc;
	});
	// Identify what type of VAT the candidate defines
	if ((props_counter.size() == 1) and props_counter.contains("gross")) {
		result = 0; // NO VAT (gross, counter gross)
		if (log) std::cout << "\nTemplate is an NO VAT transaction :)"; // gross,gross
	}
	else if ((props_counter.size() == 3) and props_counter.contains("gross") and props_counter.contains("net") and props_counter.contains("vat") and !props_counter.contains("eu_vat")) {
		if (props_sum == 3) {
			if (log) std::cout << "\nTemplate is a SWEDISH PURCHASE/sale"; // (gross,net,vat);
			result = 1; // Swedish VAT
		}
	}
	else if (
		(     (props_counter.contains("gross"))
			and (props_counter.contains("eu_purchase"))
			and (props_counter.contains("eu_vat")))) {
		result = 2; // EU VAT
		if (log) std::cout << "\nTemplate is an EU PURCHASE :)"; // gross,gross,eu_vat,eu_vat,eu_purchase,eu_purchase
	}
	else {
		if (log) std::cout << "\nFailed to recognise the VAT type";
	}
	return result;
}

void for_each_typed_meta_entry(SIEEnvironments const& sie_envs,auto& f) {
	auto f_caller = [&f](BAS::MetaEntry const& me) {
		auto tme = to_typed_meta_entry(me);
		f(tme);
	};
	for_each_meta_entry(sie_envs,f_caller);
}

using TypedMetaEntryMap = std::map<BAS::kind::AccountTransactionTypeTopology,std::vector<BAS::TypedMetaEntry>>; // AccountTransactionTypeTopology -> TypedMetaEntry
using MetaEntryTopologyMap = std::map<std::size_t,TypedMetaEntryMap>; // hash -> TypeMetaEntry
// TODO: Consider to make MetaEntryTopologyMap an unordered_map (as it is already a map from hash -> TypedMetaEntry)
//       All we should have to do is to define std::hash for this type to make std::unordered_map find it? 

MetaEntryTopologyMap to_meta_entry_topology_map(SIEEnvironments const& sie_envs) {
	MetaEntryTopologyMap result{};
	// Group on Type Topology
	MetaEntryTopologyMap meta_entry_topology_map{};
	auto h = [&result](BAS::TypedMetaEntry const& tme){
		auto types_topology = BAS::kind::to_types_topology(tme);
		auto signature = BAS::kind::to_signature(types_topology);
		result[signature][types_topology].push_back(tme);							
	};
	for_each_typed_meta_entry(sie_envs,h);
	return result;
}

struct TestResult {
	std::ostringstream prompt{"null"};
	bool failed{true};
};

std::ostream& operator<<(std::ostream& os,TestResult const& tr) {
	os << tr.prompt.str();
	return os;
}

// A typed sub-meta-entry is a subset of transactions of provided typed meta entry
// that are all of the same "type" and that all sums to zero (do balance) 
std::vector<BAS::TypedMetaEntry> to_typed_sub_meta_entries(BAS::TypedMetaEntry const& tme) {
	std::vector<BAS::TypedMetaEntry> result{};
	// TODO: When needed, identify sub-entries of typed account transactions that balance (sums to zero)
	result.push_back(tme); // For now, return input as the single sub-entry
	return result;
}

BAS::anonymous::TypedAccountTransactions to_alternative_tats(SIEEnvironments const& sie_envs,BAS::anonymous::TypedAccountTransaction const& tat) {
	BAS::anonymous::TypedAccountTransactions result{};
	result.insert(tat); // For now, return ourself as the only alternative
	return result;
}

bool operator==(BAS::TypedMetaEntry const& tme1,BAS::TypedMetaEntry const& tme2) {
	return BAS::kind::to_types_topology(tme1) == BAS::kind::to_types_topology(tme2);
}

BAS::TypedMetaEntry to_tats_swapped_tme(BAS::TypedMetaEntry const& tme,BAS::anonymous::TypedAccountTransaction const& target_tat,BAS::anonymous::TypedAccountTransaction const& new_tat) {
	BAS::TypedMetaEntry result{tme};
	// TODO: Implement actual swap of tats
	return result;
}

BAS::OptionalMetaEntry to_meta_entry_candidate(BAS::TypedMetaEntry const& tme,Amount const& gross_amount) {
	BAS::OptionalMetaEntry result{};
	// TODO: Implement actual generation of a candidate using the provided typed meta entry and the gross amount
	result = to_meta_entry(tme); // Return with unmodified amounts!
	return result;
}

bool operator==(BAS::MetaEntry const& me1, BAS::MetaEntry const& me2) {
	bool result{false};
	if (     (me1.meta == me2.meta)
		   and (me1.defacto.caption == me2.defacto.caption)
		   and (me1.defacto.date == me2.defacto.date)
	     and (me1.defacto.account_transactions.size() == me2.defacto.account_transactions.size())) {
	
	  for (int i=0;i<me1.defacto.account_transactions.size() and !result;++i) {
			result = (     (me1.defacto.account_transactions[i].transtext == me2.defacto.account_transactions[i].transtext)
								 and (me1.defacto.account_transactions[i].amount == me2.defacto.account_transactions[i].amount));
		}
	}
	return result;
}

TestResult test_typed_meta_entry(SIEEnvironments const& sie_envs,BAS::TypedMetaEntry const& tme) {
	TestResult result{};
	result.prompt << "test_typed_meta_entry=";
	auto sub_tmes = to_typed_sub_meta_entries(tme);
	for (auto const& sub_tme : sub_tmes) {
		for (auto const& tat : sub_tme.defacto.account_transactions) {
			auto alt_tats = to_alternative_tats(sie_envs,tat);
			for (auto const& alt_tat : alt_tats) {
				auto alt_tme = to_tats_swapped_tme(tme,tat,alt_tat);
				result.prompt << "\n\t\t" <<  "Swapped " << tat << " with " << alt_tat;
				// Test that we can do a roundtrip and get the alt_tme back
				auto gross_amount = std::accumulate(alt_tme.defacto.account_transactions.begin(),alt_tme.defacto.account_transactions.end(),Amount{0},[](auto acc, auto const& tat){
					if (tat.first.amount > 0) acc += tat.first.amount;
					return acc;
				});
				auto raw_alt_candidate = to_meta_entry(alt_tme); // Raw conversion
				auto alt_candidate = to_meta_entry_candidate(alt_tme,gross_amount); // Generate from gross amount
				if (alt_candidate and (*alt_candidate == raw_alt_candidate)) {
					result.prompt << "\n\t\t" << "Succes, is equal ok!";
					result.prompt << "\n\t\t      raw: " << raw_alt_candidate;
					result.prompt << "\n\t\tgenerated: " << *alt_candidate;
					result.failed = false;
				}
				else {
					result.prompt << "\n\t\t" << "FAILED, IS NOT EQUAL";
					result.prompt << "\n\t\t      raw: " << raw_alt_candidate;
					result.prompt << "\n\t\tgenerated: " << *alt_candidate;
				}
			}
		}
	}
	result.prompt << "\n\t\t" << " *DONE*";
	return result;
}

using AccountsTopologyMap = std::map<std::size_t,std::map<BAS::kind::BASAccountTopology,BAS::TypedMetaEntries>>;

AccountsTopologyMap to_accounts_topology_map(BAS::TypedMetaEntries const& tmes) {
	AccountsTopologyMap result{};
	auto g = [&result](BAS::TypedMetaEntry const& tme) {
		auto accounts_topology = BAS::kind::to_accounts_topology(tme);
		auto signature = BAS::kind::to_signature(accounts_topology);
		result[signature][accounts_topology].push_back(tme);
	};
	std::for_each(tmes.begin(),tmes.end(),g);
	return result;
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

		namespace TAXReturns {
			extern SKV::XML::XMLMap tax_returns_template; // See bottom of this file
		}

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
					try {
						std::ostringstream os{};
						os << std::setfill('0') << std::setw(2) << box_no;
						if (p[2].find(os.str()) != std::string::npos) acc.push_back(std::stoi(p[0]));

					}
					catch (std::exception const& e) { std::cerr << "\nDESIGN INSUFFICIENCY: to_accounts::lambda failed. Exception=" << std::quoted(e.what());}					
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
							std::cerr << "\nTODO: In to_vat_returns_had, turn created box_map for  p r e v i o u s  quarter to a had";
						}
					}
					{
						auto quarter = to_quarter_range(today);
						auto vat_returns_meta = to_vat_returns_meta(quarter);
						auto is_quarter = [&vat_returns_meta](BAS::MetaAccountTransaction const& mat){
							return vat_returns_meta->period.contains(mat.meta.defacto.date);
						};
						if (auto box_map = to_form_box_map(sie_envs,is_quarter)) {
							std::cerr << "\nTODO: In to_vat_returns_had, turn created box_map for  c u r r e n t  quarter to a had";
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
	SKV::XML::XMLMap xml_map{SKV::XML::TAXReturns::tax_returns_template};
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
	// Manual Build generator states
	,GrossDebitorCreditOption // User selects if Gross account is Debit or Credit
	,CounterTransactionsAggregateOption // User selects what kind of counter trasnaction aggregate to create
	,GrossAccountInput
	,NetVATAccountInput
	,JEAggregateOptionIndex
	,EnterHA
	,AccountIndex
	,Amount
	,CounterAccountsEntry
	,SKVEntryIndex
	,QuarterOptionIndex
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
	// BAS::MetaEntries template_candidates{};
	BAS::TypedMetaEntries template_candidates{};
	BAS::anonymous::AccountTransactions at_candidates{};
	BAS::anonymous::AccountTransaction at{};
  std::string prompt{};
	bool quit{};
	std::map<std::string,std::filesystem::path> sie_file_path{};
	SIEEnvironments sie{};
	HeadingAmountDateTransEntries heading_amount_date_entries{};
	std::filesystem::path staged_sie_file_path{"cratchit.se"};

	std::optional<HeadingAmountDateTransEntries::iterator> to_had_iter(int had_index) {
		std::optional<HeadingAmountDateTransEntries::iterator> result{};
		auto had_iter = this->heading_amount_date_entries.begin();
		auto end = this->heading_amount_date_entries.end();
		// std::cout << "\nto_had_iter had_index:" << had_index << " end-begin:" << std::distance(had_iter,end);
		if (had_index < std::distance(had_iter,end)) {
			std::advance(had_iter,this->had_index);
			result = had_iter;
		}
		return result;
	}

	std::optional<HeadingAmountDateTransEntries::iterator> selected_had() {
		return to_had_iter(this->had_index);
	}

	BAS::anonymous::OptionalAccountTransaction selected_had_at(int at_index) {
		BAS::anonymous::OptionalAccountTransaction result{};
		if (auto had_iter = to_had_iter(this->had_index)) {
			if ((*had_iter)->current_candidate) {
				auto at_iter = (*had_iter)->current_candidate->defacto.account_transactions.begin();
				auto end = (*had_iter)->current_candidate->defacto.account_transactions.end();
				if (at_index < std::distance(at_iter,end))
				std::advance(at_iter,at_index);
				result = *at_iter;
			}
		}
		return result;
	}

	void erease_selected_had() {
		if (auto had_iter = to_had_iter(this->had_index)) {
			this->heading_amount_date_entries.erase(*had_iter);
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

using PromptOptionsList = std::vector<std::string>;

std::ostream& operator<<(std::ostream& os,PromptOptionsList const& pol) {
	for (auto const& option : pol) {
		os << "\n" << option;
	}
	return os;
}

PromptOptionsList options_list_of_prompt_state(PromptState const& prompt_state) {
	std::vector<std::string> result{};
	result.push_back("Options:");
	switch (prompt_state) {
		case PromptState::Undefined: {result.push_back("DESIGN INSUFFICIENCY: options_list_of_prompt_state have no action for State PromptState::Undefined ");} break;
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
		case PromptState::HADIndex: {result.push_back("PromptState::HADIndex");} break;
		case PromptState::JEIndex: {result.push_back("PromptState::JEIndex");} break;
		case PromptState::GrossDebitorCreditOption: {
			result.push_back("0: As is ");
			result.push_back("1: (+) Force to Debit ");
			result.push_back("2: (-) Force to Credit ");
		} break;
		case PromptState::CounterTransactionsAggregateOption: {
			result.push_back("0: Gross counter transaction account aggregate");
			result.push_back("1: {Net, VAT} counter transaction accounts aggregate");
		} break;
		case PromptState::GrossAccountInput: {
			result.push_back("Please enter counter transaction account");
		} break;
		case PromptState::NetVATAccountInput: {
			result.push_back("Please enter Net + VAT counter accounts (separated by space");
		} break;
		case PromptState::JEAggregateOptionIndex: {
			result.push_back("0 1 x counter transactions aggregate");
			result.push_back("1 n x counter transactions aggregate");
			result.push_back("2 Edit account transactions");
			result.push_back("3 STAGE as-is");
		} break;
		case PromptState::EnterHA: {result.push_back("PromptState::EnterHA");} break;
		case PromptState::AccountIndex: {result.push_back("PromptState::AccountIndex");} break;
		case PromptState::Amount: {
			result.push_back("Please Enter Amount");
		} break;
		case PromptState::CounterAccountsEntry: {result.push_back("PromptState::CounterAccountsEntry");} break;
		case PromptState::SKVEntryIndex: {result.push_back("PromptState::SKVEntryIndex");} break;
		case PromptState::QuarterOptionIndex: {result.push_back("PromptState::QuarterOptionIndex");} break;
		case PromptState::SKVTaxReturnEntryIndex: {result.push_back("PromptState::SKVTaxReturnEntryIndex");} break;
		case PromptState::EnterContact: {result.push_back("PromptState::EnterContact");} break;
		case PromptState::EnterEmployeeID: {result.push_back("PromptState::EnterEmployeeID");} break;
		case PromptState::Unknown: {result.push_back("DESIGN INSUFFICIENCY: options_list_of_prompt_state have no action for State PromptState::Unknown ");} break;
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
	SIE::OStream sieos{os};
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
	prompt << options_list_of_prompt_state(prompt_state);
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
		case PromptState::QuarterOptionIndex: {
			prompt << ":skv:tax_return:period";
		} break;
		case PromptState::SKVTaxReturnEntryIndex: {
			prompt << ":skv:tax_return";
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
				std::cout << "\nAct on ix = " << *signed_ix << " in state:" << static_cast<int>(model->prompt_state);
				size_t ix = std::abs(*signed_ix);
				bool do_remove = (*signed_ix<0);
				// Act on prompt state index input
				switch (model->prompt_state) {
					case PromptState::Root: {
					} break;
					case PromptState::HADIndex: {
						model->had_index = ix;
						if (auto had_iter = model->selected_had()) {
							auto& had = *(*had_iter);
							prompt << "\n" << had;
							if (do_remove) {
								model->heading_amount_date_entries.erase(*had_iter);
								prompt << " REMOVED";
								model->prompt_state = PromptState::Root;
							}
							else {
								// selected HAD and list template options
								if (had.current_candidate) {
									prompt << "\n\t" << *had.current_candidate;
									if (had.counter_ats_producer) {
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
										model->template_candidates.push_back(to_typed_meta_entry(me));
									}
									// List options
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
					case PromptState::JEIndex: {
						if (auto had_iter = model->selected_had()) {
							auto& had = *(*had_iter);
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
								prompt << "\ncandidate:" << me;
								model->prompt_state = PromptState::GrossDebitorCreditOption;
							}
							else {
								// Assume user selected an entry as base for a template
								auto tme_iter = model->template_candidates.begin();
								auto tme_end = model->template_candidates.end();
								auto at_iter = model->at_candidates.begin();
								auto at_end = model->at_candidates.end();
								if (ix < std::distance(tme_iter,tme_end)) {
									std::advance(tme_iter,ix);
									auto tp = to_template(*tme_iter);
									if (tp) {
										auto me = to_journal_entry(had,*tp);
										prompt << "\ncandidate " << me;
										had.current_candidate = me;
										model->prompt_state = PromptState::JEAggregateOptionIndex;
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
							if (had.current_candidate) {
								if (had.current_candidate->defacto.account_transactions.size()==1) {
									switch (ix) {
										case 0: {
											// As is
											model->prompt_state = PromptState::CounterTransactionsAggregateOption;
										} break;
										case 1: {
											// Force debit
											had.current_candidate->defacto.account_transactions[0].amount = std::abs(had.current_candidate->defacto.account_transactions[0].amount);
											model->prompt_state = PromptState::CounterTransactionsAggregateOption;
										} break;
										case 2: {
												// Force credit
											had.current_candidate->defacto.account_transactions[0].amount = -1.0f * std::abs(had.current_candidate->defacto.account_transactions[0].amount);
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
								prompt << "\ncandidate:" << *had.current_candidate;
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
							if (had.current_candidate) {
								if (had.current_candidate->defacto.account_transactions.size()==1) {
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
								prompt << "\ncandidate:" << *had.current_candidate;
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
							if (had.current_candidate) {
								if (had.current_candidate->defacto.account_transactions.size()==1) {
									if (ast.size() == 1) {
										auto gross_counter_account_no = BAS::to_account_no(ast[0]);
										if (gross_counter_account_no) {
											Amount gross_transaction_amount = had.current_candidate->defacto.account_transactions[0].amount;
											had.current_candidate->defacto.account_transactions.push_back(BAS::anonymous::AccountTransaction{
												.account_no = *gross_counter_account_no
												,.amount = -1.0f * gross_transaction_amount
											}
											);
											prompt << "\nmutated candidate:" << *had.current_candidate;
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
								prompt << "\ncandidate:" << *had.current_candidate;
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
							if (had.current_candidate) {
								if (had.current_candidate->defacto.account_transactions.size()==1) {
									if (ast.size() == 2) {
										auto net_counter_account_no = BAS::to_account_no(ast[0]);
										auto vat_counter_account_no = BAS::to_account_no(ast[1]);
										if (net_counter_account_no and vat_counter_account_no) {
											Amount gross_transaction_amount = had.current_candidate->defacto.account_transactions[0].amount;
											had.current_candidate->defacto.account_transactions.push_back(BAS::anonymous::AccountTransaction{
												.account_no = *net_counter_account_no
												,.amount = -0.8f * gross_transaction_amount
											}
											);
											had.current_candidate->defacto.account_transactions.push_back(BAS::anonymous::AccountTransaction{
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
								prompt << "\ncandidate:" << *had.current_candidate;
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
						std::cout << "\ncase PromptState::JEAggregateOptionIndex: {";
						if (auto had_iter = model->selected_had()) {
							auto& had = *(*had_iter);
							if (had.current_candidate) {
								std::cout << "\nif (had.current_candidate) {";
								// We need a typed entry to do some clever decisions
								auto tme = to_typed_meta_entry(*had.current_candidate);
								prompt << "\n" << tme;
								switch (to_vat_type(tme)) {
									case 0: {
										// No VAT in candidate. 
										// Continue with 
										// 1) Some propose gross account transactions
										// 2) a n x gross Counter aggregate
									} break;
									case 1: {
										// Swedish VAT detcted in candidate.
										// Continue with 
										// 2) a n x {net,vat} counter aggregate
									} break;
									case 2: {
										// EU VAT detected in candidate.
										// Continue with a 
										// 2) n x gross counter aggregate + an EU VAT Returns "virtual" aggregate
									} break;
									case 3: {
										// Import VAT detected in candidate
										// Continue with a
										// 1) Import cost gross transaction
										// 2) Import Cost counter aggregate
									}
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
										if (std::any_of(had.current_candidate->defacto.account_transactions.begin(),had.current_candidate->defacto.account_transactions.end(),[](BAS::anonymous::AccountTransaction const& at){
											return std::abs(at.amount) < 1.0;
										})) {
											// Assume the user need to specify rounding by editing proposed account transactions
											unsigned int i{};
											std::for_each(had.current_candidate->defacto.account_transactions.begin(),had.current_candidate->defacto.account_transactions.end(),[&i,&prompt](auto const& at){
												prompt << "\n  " << i++ << " " << at;
											});
											model->prompt_state = PromptState::AccountIndex;
										}
										else {
											// Stage as-is
											if (auto staged_me = model->sie["current"].stage(*had.current_candidate)) {
												prompt << "\n" << *staged_me << " STAGED";
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
										BAS::anonymous::OptionalAccountTransaction net_at;
										BAS::anonymous::OptionalAccountTransaction vat_at;
										for (auto const& [at,props] : tme.defacto.account_transactions) {
											if (props.contains("net")) net_at = at;
											if (props.contains("vat")) vat_at = at;
										}
										if (!net_at) std::cerr << "\nNo net_at";
										if (!vat_at) std::cerr << "\nNo vat_at";
										if (net_at and vat_at) {
											had.counter_ats_producer = ToNetVatAccountTransactions{*net_at,*vat_at};
											
											BAS::anonymous::AccountTransactions ats_to_keep{};
											std::remove_copy_if(
												had.current_candidate->defacto.account_transactions.begin()
												,had.current_candidate->defacto.account_transactions.end()
												,std::back_inserter(ats_to_keep)
												,[&net_at,&vat_at](auto const& at){
													return ((at.account_no == net_at->account_no) or (at.account_no == vat_at->account_no));
											});
											had.current_candidate->defacto.account_transactions = ats_to_keep;
										}
										prompt << "\ncadidate: " << *had.current_candidate;
										model->prompt_state = PromptState::EnterHA;
									} break;
									case 2: {
										// Allow the user to edit individual account transactions
										unsigned int i{};
										std::for_each(had.current_candidate->defacto.account_transactions.begin(),had.current_candidate->defacto.account_transactions.end(),[&i,&prompt](auto const& at){
											prompt << "\n  " << i++ << " " << at;
										});
										model->prompt_state = PromptState::AccountIndex;
									} break;
									case 3: {
										// Stage the candidate
										if (auto staged_me = model->sie["current"].stage(*had.current_candidate)) {
											prompt << "\n" << *staged_me << " STAGED";
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
					case PromptState::AccountIndex: {
						if (auto at = model->selected_had_at(ix)) {
							model->at = *at;
							prompt << "\nTransaction:" << model->at;
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
								auto current_qr = to_quarter_range(today);
								auto previous_qr = to_previous_quarter(current_qr);
								auto quarter_before_previous_qr = to_previous_quarter(previous_qr);
								auto two_previous_quarters = DateRange{quarter_before_previous_qr.begin(),previous_qr.end()};

								prompt << "\n0: Track Current Quarter " << current_qr;
								prompt << "\n1: Report Previous Quarter " << previous_qr;
								prompt << "\n2: Check Previous two Quarters " << two_previous_quarters;
								model->prompt_state = PromptState::QuarterOptionIndex;								
							} break;
							default: {prompt << "\nPlease enter a valid index";} break;
						}
					} break;

					case PromptState::QuarterOptionIndex: {
						auto today = to_today();
						auto current_qr = to_quarter_range(today);
						auto previous_qr = to_previous_quarter(current_qr);
						auto quarter_before_previous_qr = to_previous_quarter(previous_qr);
						auto two_previous_quarters = DateRange{quarter_before_previous_qr.begin(),previous_qr.end()};
						OptionalDateRange period_range{};
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
								prompt << "\nPrevious two Quarters " << two_previous_quarters << " (to check)";
								period_range = two_previous_quarters;
							} break;
							default: {
								prompt << "\nPlease select a valid option (it seems option " << ix << " is unknown to me";
							} break;
						}
						if (period_range) {
							// Create VAT Returns form for selected period
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
							default: {prompt << "\nPlease enter a valid index";} break;
						}
					} break;

					case PromptState::EnterHA:
					case PromptState::EnterContact:
					case PromptState::EnterEmployeeID:
					case PromptState::Amount:
					case PromptState::Undefined:
					case PromptState::Unknown:
						prompt << "\nPlease enter \"word\" like text (index option not available in this state)";
						break;
				}
			}
			else if (ast[0] == "-bas") {
				std::cout << " :)";
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
					prompt << model->sie["current"];				
					// std::cout << model->sie["current"];
				}
				else if (ast.size()==2) {
					if (auto bas_account_no = BAS::to_account_no(ast[1])) {
						prompt << "\nFilter journal entries that has a transaction to account no " << *bas_account_no;
						prompt << "\nTIP: If you meant filter on amount please re-enter using '.00' to distinguish it from an account no.";
						FilteredSIEEnvironment filtered_sie{model->sie["current"],BAS::filter::HasTransactionToAccount{*bas_account_no}};
						prompt << filtered_sie;
					}
					else if (auto gross_amount = to_amount(ast[1])) {
						prompt << "\nFilter journal entries that match gross amount " << *gross_amount;
						FilteredSIEEnvironment filtered_sie{model->sie["current"],BAS::filter::HasGrossAmount{*gross_amount}};
						prompt << filtered_sie;
					}
					else if (ast[1]=="*") {
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
							// Update the list of staged entries
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
						// Group on Type Topology
						// using TypedMetaEntryMap = std::map<BAS::kind::AccountTransactionTypeTopology,std::vector<BAS::TypedMetaEntry>>;
						// using MetaEntryTopologyMap = std::map<std::size_t,TypedMetaEntryMap>;
						// MetaEntryTopologyMap meta_entry_topology_map{};
						// auto h = [&meta_entry_topology_map](BAS::TypedMetaEntry const& tme){
						// 	auto types_topology = BAS::kind::to_types_topology(tme);
						// 	auto signature = BAS::kind::to_signature(types_topology);
						// 	meta_entry_topology_map[signature][types_topology].push_back(tme);							
						// };
						// for_each_typed_meta_entry(model->sie,h);
						auto meta_entry_topology_map = to_meta_entry_topology_map(model->sie);
						// Prepare to record journal entries we could not use as template for new entries
						std::vector<BAS::TypedMetaEntry> failed_tmes{};
						// List grouped on type topology
						for (auto const& [signature,tme_map] : meta_entry_topology_map) {
							for (auto const& [topology,tmes] : tme_map) {
								prompt << "\n[" << topology << "] ";
								// Group tmes on BAS Accounts topology
								auto accounts_topology_map = to_accounts_topology_map(tmes);
								// List grouped BAS Accounts topology
								for (auto const& [signature,bat_map] : accounts_topology_map) {
									for (auto const& [topology,tmes] : bat_map) {
										prompt << "\n    [" << topology << "] ";
										for (auto const& tme : tmes) {
											prompt << "\n       VAT Type:" << to_vat_type(tme);
											prompt << "\n      " << tme.meta << " " << std::quoted(tme.defacto.caption) << " " << tme.defacto.date;
											prompt << IndentedOnNewLine{tme.defacto.account_transactions,10};
											auto test_result = test_typed_meta_entry(model->sie,tme);
											prompt << "\n       TEST: " << test_result;
											if (test_result.failed) failed_tmes.push_back(tme);											
										}
									}
								}
							}
						}
						// LOG tmes we failed to identify as templates for new journal entries
						prompt << "\n<DESIGN INSUFFICIENCY: FAILED TO IDENTIFY AND USE THESE ENTRIES AS TEMPLATE>";
						for (auto const& tme : failed_tmes) {
							auto types_topology = BAS::kind::to_types_topology(tme);
							prompt << "\n" << types_topology << " " << tme.meta;
						}
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
				std::sort(model->heading_amount_date_entries.begin(),model->heading_amount_date_entries.end(),falling_date);
				if (ast.size()==1) {
					// Expose current hads (Heading Amount Date transaction entries) to the user
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
						for (auto const& [account_no,am] : model->sie["current"].account_metas()) {
							if ((*to_match_account_no == account_no) or (am.sru_code and (*to_match_account_no == *am.sru_code))) {
								prompt << "\n  " << account_no << " " << std::quoted(am.name);
								if (am.sru_code) prompt << " SRU:" << *am.sru_code;
							}
						}
					}
					else {
						// Assume match to account name
						for (auto const& [account_no,am] : model->sie["current"].account_metas()) {
							if (first_in_second_case_insensitive(ast[1],am.name)) {
								prompt << "\n  " << account_no << " " << std::quoted(am.name);
								if (am.sru_code) prompt << " SRU:" << *am.sru_code;
							}
						}
					}
				}
				else {
					// list all metas
					for (auto const& [account_no,am] : model->sie["current"].account_metas()) {
						prompt << "\n  " << account_no << " " << std::quoted(am.name);
						if (am.sru_code) prompt << " SRU:" << *am.sru_code;
					}
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
					prompt << "\n0: Arbetsgivardeklaration (TAX Returns)";
					prompt << "\n1: Momsrapport (VAT Returns)";
					model->prompt_state = PromptState::SKVEntryIndex;
				}
				else if (ast.size() == 2) {
					// Assume Tax Returns form
					// Assume second argument is period
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
				std::cout << "\nCSV :)";
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
					std::cout << "\ncsv file " << csv_file_path;
					std::ifstream ifs{csv_file_path};
					if (std::filesystem::exists(csv_file_path)) {
						std::cout << "\ncsv file exists ok";
						CSV::NORDEA::istream in{ifs};
						OptionalBASAccountNo gross_bas_account_no{};
						if (ast.size()>2) {
							std::cout << "\n ast[2]:)" << ast[2];
							if (auto bas_account_no = BAS::to_account_no(ast[2])) {
								gross_bas_account_no = *bas_account_no;
								std::cout << "\n gross_bas_account_no:" << *gross_bas_account_no;
							}
							else {
								prompt << "\nPlease enter a valid BAS account no for gross amount transaction. " << std::quoted(ast[2]) << " is not a valid BAS account no";
							}
						}
						auto hads = CSV::from_stream(in,gross_bas_account_no);
						std::copy(hads.begin(),hads.end(),std::back_inserter(model->heading_amount_date_entries));
					}
					else {
						prompt << "\nERROR - Please provide a path to an existing file. Can't find " << csv_file_path;
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
				if ((model->prompt_state == PromptState::NetVATAccountInput) or (model->prompt_state == PromptState::GrossAccountInput))  {
					// Assume the user has enterd text to search for suitable accounts
					// Assume match to account name
					for (auto const& [account_no,am] : model->sie["current"].account_metas()) {
						if (first_in_second_case_insensitive(ast[1],am.name)) {
							prompt << "\n  " << account_no << " " << std::quoted(am.name);
							if (am.sru_code) prompt << " SRU:" << *am.sru_code;
						}
					}
				}
				else if (model->prompt_state == PromptState::EnterHA) {
					if (auto had_iter = model->selected_had()) {
						auto& had = *(*had_iter);
						if (!had.current_candidate) std::cerr << "\nNo had.current_candidate";
						if (!had.counter_ats_producer) std::cerr << "\nNo had.counter_ats_producer";
						if (had.current_candidate and had.counter_ats_producer) {
							auto gross_positive_amount = to_positive_gross_transaction_amount(had.current_candidate->defacto);
							auto gross_negative_amount = to_negative_gross_transaction_amount(had.current_candidate->defacto);
							auto gross_amounts_diff = gross_positive_amount + gross_negative_amount;
							std::cout << "\ngross_positive_amount:" << gross_positive_amount << " gross_negative_amount:" << gross_negative_amount << " gross_amounts_diff:" << gross_amounts_diff;

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
										auto ats = (*had.counter_ats_producer)(std::abs(gross_amounts_diff),ast[0]);
										std::copy(ats.begin(),ats.end(),std::back_inserter(had.current_candidate->defacto.account_transactions));
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
											auto ats = (*had.counter_ats_producer)(std::abs(gross_amounts_diff),ast[0],amount);
											std::copy(ats.begin(),ats.end(),std::back_inserter(had.current_candidate->defacto.account_transactions));
											prompt << "\nAdded negative transactions aggregate" << ats;
										}
										else if (gross_amounts_diff < 0) {
											// We need to balance up with positive account transaction aggregates
											auto ats = (*had.counter_ats_producer)(std::abs(gross_amounts_diff),ast[0],amount);
											std::copy(ats.begin(),ats.end(),std::back_inserter(had.current_candidate->defacto.account_transactions));
											prompt << "\nAdded positive transaction aggregate";
										}
										else if (std::abs(gross_amounts_diff) < 1.0) {
											// Consider a cents rounding account transaction
											prompt << "\nAdded cents rounding to account 3740";
											auto cents_rounding_at = BAS::anonymous::AccountTransaction{
												.account_no = 3740
												,.amount = -gross_amounts_diff
											};
											had.current_candidate->defacto.account_transactions.push_back(cents_rounding_at);
										}
										else {
											// The journal entry candidate balances. Consider to stage it
											prompt << "\nTODO: Stage balancing journal entry";
										}
									}
								} break;
							}
							prompt << "\ncandidate:" << *had.current_candidate;
							gross_positive_amount = to_positive_gross_transaction_amount(had.current_candidate->defacto);
							gross_negative_amount = to_negative_gross_transaction_amount(had.current_candidate->defacto);
							gross_amounts_diff = gross_positive_amount + gross_negative_amount;
							prompt << "\n-------------------------------";
							prompt << "\ndiff:" << gross_amounts_diff;
							if (gross_amounts_diff == 0) {
								// Stage the journal entry
								auto me = model->sie["current"].stage(*had.current_candidate);
								if (me) {
									prompt << "\n" << *me << " STAGED";
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
				else if (model->prompt_state == PromptState::JEIndex) {
					// Assume the user has entered a new search criteria for template candidates
					model->template_candidates = this->all_years_template_candidates([&command](BAS::anonymous::JournalEntry const& aje){
						return strings_share_tokens(command,aje.caption);
					});
					int ix{0};
					for (int i = 0; i < model->template_candidates.size(); ++i) {
						prompt << "\n    " << ix++ << " " << model->template_candidates[i];
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
					for (int i=0;i < model->at_candidates.size();++i) {
						prompt << "\n    " << ix++<< " " << model->at_candidates[i];
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
					if (auto had_iter = model->selected_had()) {
						if ((*had_iter)->current_candidate) {
							unsigned int i{};
							prompt << "\n" << (*had_iter)->current_candidate->defacto.caption << " " << (*had_iter)->current_candidate->defacto.date;
							for (auto const& at : (*had_iter)->current_candidate->defacto.account_transactions) {
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
					if (auto had_iter = model->selected_had()) {
						if (auto amount = to_amount(command)) {
							prompt << "\nAmount " << *amount;
							model->at.amount = *amount;
							if ((*had_iter)->current_candidate) {
								(*had_iter)->current_candidate = updated_entry(*(*had_iter)->current_candidate,model->at);
								model->prompt_state = PromptState::JEAggregateOptionIndex;
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
		auto help = options_list_of_prompt_state(model->prompt_state);
		for (auto const& line : help) prompt << "\n" << line;
		prompt << prompt_line(model->prompt_state);
		model->prompt = prompt.str();
		return {};
	}	
private:
	BAS::TypedMetaEntries all_years_template_candidates(auto const& matches) {
		BAS::TypedMetaEntries result{};
		auto meta_entry_topology_map = to_meta_entry_topology_map(model->sie);
		for (auto const& [signature,tme_map] : meta_entry_topology_map) {
			for (auto const& [topology,tmes] : tme_map) {
				auto accounts_topology_map = to_accounts_topology_map(tmes);
				for (auto const& [signature,bat_map] : accounts_topology_map) {
					for (auto const& [topology,tmes] : bat_map) {
						for (auto const& tme : tmes) {
							auto me = to_meta_entry(tme);
							if (matches(me.defacto)) result.push_back(tme);
						}
					}
				}
			}
		}
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
		model->prompt += prompt.str();
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
		std::ostringstream prompt{};
		if (auto val_iter = environment.find("sie_file");val_iter != environment.end()) {
			for (auto const& [year_key,sie_file_name] : val_iter->second) {
				std::filesystem::path sie_file_path{sie_file_name};
				if (auto sie_environment = from_sie_file(sie_file_path)) {
					model->sie[year_key] = std::move(*sie_environment);
					model->sie_file_path[year_key] = {sie_file_name};
					prompt << "\nsie[" << year_key << "] from " << sie_file_path;
				}
			}
		}
		if (auto sse = from_sie_file(model->staged_sie_file_path)) {
			if (sse->journals().size()>0) {
				prompt << "\n<STAGED>";
				prompt << *sse;
			}
			auto unstaged = model->sie["current"].stage(*sse);
			for (auto const& je : unstaged) {
				prompt << "\nnow posted " << je; 
			}
		}
		model->heading_amount_date_entries = this->hads_from_environment(environment);
		model->organisation_contacts = this->contacts_from_environment(environment);
		model->employee_birth_ids = this->employee_birth_ids_from_environment(environment);
		model->prompt = prompt.str();
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
	if (false) {
		// Log current locale and test charachter encoding.
		// TODO: Activate to adjust for cross platform handling 
			std::wcout << "\nDeafult (current) locale setting is " << std::locale().name().c_str();
			std::string sHello{"Hallå Åland! Ömsom ödmjuk. Ärligt äkta."}; // This source file expected to be in UTF-8
			std::cout << "\ncout:" << sHello; // macOS handles UTF-8 ok (how about other platforms?)
			for (auto const& ch : sHello) {
				std::cout << "\n" << ch << " " << std::hex << static_cast<int>(ch) << std::dec; // check stream and console encoding, std::locale behaviour
			}
			std::ofstream os{"cp435_test.txt"};
			SIE::OStream sieos{os};
			sieos << sHello; // We expect SIE file encoding of CP435
			return 0;
			std::string sPATH{std::getenv("PATH")};
			std::cout << "\nPATH=" << sPATH;
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

namespace charset {
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
	} // namespace CP435
}

namespace SKV {
	namespace XML {

		namespace TAXReturns {
			SKV::XML::XMLMap tax_returns_template{
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
		} // namespace TAXReturns
	}
}

namespace BAS {
	// The following string literal is the "raw" output of:
	// 1) In macOS Numbers opening excel file downlaoded from https://www.bas.se/kontoplaner/
	// 2) Export as csv-file
	// See project resource ./resources/Kontoplan-2022.csv
	char const* bas_2022_account_plan_csv{R"(;Kontoplan – BAS 2022;;;;;;;;
;;;;;;;;;
;;;;|;;= Ändring eller tillägg jämfört med föregående år. Mer information finns på BAS webbplats (bas.se).;;;
;;;;■;;= Kontot ingår i det urval av konton som för de flesta företag är tillräckligt för en grundläggande bokföring.;;;
;;;;[Ej K2];;= Kontot används inte av de företag som valt att tillämpa K2-regler.;;;
;;;;;;;;;
;;;Huvudkonton;;;Underkonton;;;
;;;;;;;;;
;;1;Tillgångar;;;;;;
;;10;Immateriella anläggningstillgångar;;;;;;
;[Ej K2];101;Utvecklingsutgifter;[Ej K2];1010;Utvecklingsutgifter;;;
;;;;[Ej K2];1011;Balanserade utgifter för forskning och utveckling;;;
;;;;[Ej K2];1012;Balanserade utgifter för programvaror;;;
;;;;[Ej K2];1018;Ackumulerade nedskrivningar på balanserade utgifter;;;
;;;;[Ej K2];1019;Ackumulerade avskrivningar på balanserade utgifter;;;
;;102;Koncessioner m.m.;;1020;Koncessioner m.m.;;;
;;;;;1028;Ackumulerade nedskrivningar på koncessioner m.m.;;;
;;;;;1029;Ackumulerade avskrivningar på koncessioner m.m.;;;
;■;103;Patent;■;1030;Patent;;;
;;;;;1038;Ackumulerade nedskrivningar på patent;;;
;;;;■;1039;Ackumulerade avskrivningar på patent;;;
;;104;Licenser;;1040;Licenser;;;
;;;;;1048;Ackumulerade nedskrivningar på licenser;;;
;;;;;1049;Ackumulerade avskrivningar på licenser;;;
;;105;Varumärken;;1050;Varumärken;;;
;;;;;1058;Ackumulerade nedskrivningar på varumärken;;;
;;;;;1059;Ackumulerade avskrivningar på varumärken;;;
;■;106;Hyresrätter, tomträtter och liknande;■;1060;Hyresrätter, tomträtter och liknande;;;
;;;;;1068;Ackumulerade nedskrivningar på hyresrätter, tomträtter och liknande;;;
;;;;■;1069;Ackumulerade avskrivningar på hyresrätter, tomträtter och liknande;;;
;;107;Goodwill;;1070;Goodwill;;;
;;;;;1078;Ackumulerade nedskrivningar på goodwill;;;
;;;;;1079;Ackumulerade avskrivningar på goodwill;;;
;;108;Förskott för immateriella anläggningstillgångar;;1080;Förskott för immateriella anläggningstillgångar;;;
;;;;[Ej K2];1081;Pågående projekt för immateriella anläggningstillgångar;;;
;;;;;1088;Förskott för immateriella anläggningstillgångar;;;
;;11;Byggnader och mark;;;;;;
;■;111;Byggnader;■;1110;Byggnader;;;
;;;;;1111;Byggnader på egen mark;;;
;;;;;1112;Byggnader på annans mark;;;
;;;;;1118;Ackumulerade nedskrivningar på byggnader;;;
;;;;■;1119;Ackumulerade avskrivningar på byggnader;;;
;;112;Förbättringsutgifter på annans fastighet;;1120;Förbättringsutgifter på annans fastighet;;;
;;;;;1129;Ackumulerade avskrivningar på förbättringsutgifter på annans fastighet;;;
;■;113;Mark;■;1130;Mark;;;
;;114;Tomter och obebyggda markområden;;1140;Tomter och obebyggda markområden;;;
;■;115;Markanläggningar;■;1150;Markanläggningar;;;
;;;;;1158;Ackumulerade nedskrivningar på markanläggningar;;;
;;;;■;1159;Ackumulerade avskrivningar på markanläggningar;;;
;;118;Pågående nyanläggningar och förskott för byggnader och mark;;1180;Pågående nyanläggningar och förskott för byggnader och mark;;;
;;;;;1181;Pågående ny-, till- och ombyggnad;;;
;;;;;1188;Förskott för byggnader och mark;;;
;;12;Maskiner och inventarier;;;;;;
;■;121;Maskiner och andra tekniska anläggningar;■;1210;Maskiner och andra tekniska anläggningar;;;
;;;;;1211;Maskiner;;;
;;;;;1213;Andra tekniska anläggningar;;;
;;;;;1218;Ackumulerade nedskrivningar på maskiner och andra tekniska anläggningar;;;
;;;;■;1219;Ackumulerade avskrivningar på maskiner och andra tekniska anläggningar;;;
;■;122;Inventarier och verktyg;■;1220;Inventarier och verktyg;;;
;;;;;1221;Inventarier;;;
;;;;;1222;Byggnadsinventarier;;;
;;;;;1223;Markinventarier;;;
;;;;;1225;Verktyg;;;
;;;;;1228;Ackumulerade nedskrivningar på inventarier och verktyg;;;
;;;;■;1229;Ackumulerade avskrivningar på inventarier och verktyg;;;
;;123;Installationer;;1230;Installationer;;;
;;;;;1231;Installationer på egen fastighet;;;
;;;;;1232;Installationer på annans fastig het;;;
;;;;;1238;Ackumulerade nedskrivningar på installationer;;;
;;;;;1239;Ackumulerade avskrivningar på installationer;;;
;■;124;Bilar och andra transportmedel;■;1240;Bilar och andra transportmedel;;;
;;;;;1241;Personbilar;;;
;;;;;1242;Lastbilar;;;
;;;;;1243;Truckar;;;
;;;;;1244;Arbetsmaskiner;;;
;;;;;1245;Traktorer;;;
;;;;;1246;Motorcyklar, mopeder och skotrar;;;
;;;;;1247;Båtar, flygplan och helikoptrar;;;
;;;;;1248;Ackumulerade nedskrivningar på bilar och andra transportmedel;;;
;;;;■;1249;Ackumulerade avskrivningar på bilar och andra transportmedel;;;
;■;125;Datorer;■;1250;Datorer;;;
;;;;;1251;Datorer, företaget;;;
;;;;;1257;Datorer, personal;;;
;;;;;1258;Ackumulerade nedskrivningar på datorer;;;
;;;;■;1259;Ackumulerade avskrivningar på datorer;;;
;[Ej K2];126;Leasade tillgångar;[Ej K2];1260;Leasade tillgångar;;;
;;;;[Ej K2];1269;Ackumulerade avskrivningar på leasade tillgångar;;;
;;128;Pågående nyanläggningar och förskott för maskiner och inventarier;;1280;Pågående nyanläggningar och förskott för maskiner och inventarier;;;
;;;;;1281;Pågående nyanläggningar, maskiner och inventarier;;;
;;;;;1288;Förskott för maskiner och inventarier;;;
;■;129;Övriga materiella anläggningstillgångar;■;1290;Övriga materiella anläggningstillgångar;;;
;;;;■;1291;Konst och liknande tillgångar;;;
;;;;;1292;Djur som klassificeras som anläggningstillgång;;;
;;;;;1298;Ackumulerade nedskrivningar på övriga materiella anläggningstillgångar;;;
;;;;■;1299;Ackumulerade avskrivningar på övriga materiella anläggningstillgångar;;;
;;13;Finansiella anläggningstillgångar;;;;;;
;;131;Andelar i koncernföretag;;1310;Andelar i koncernföretag;;;
;;;;;1311;Aktier i noterade svenska koncernföretag;;;
;;;;;1312;Aktier i onoterade svenska koncernföretag;;;
;;;;;1313;Aktier i noterade utländska koncernföretag;;;
;;;;;1314;Aktier i onoterade utländska koncernföretag;;;
;;;;;1316;Andra andelar i svenska koncernföretag;;;
;;;;;1317;Andra andelar i utländska koncernförertag;;;
;;;;;1318;Ackumulerade nedskrivningar av andelar i koncernföretag;;;
;;132;Långfristiga fordringar hos koncernföretag;;1320;Långfristiga fordringar hos koncernföretag;;;
;;;;;1321;Långfristiga fordringar hos moderföretag;;;
;;;;;1322;Långfristiga fordringar hos dotterföretag;;;
;;;;;1323;Långfristiga fordringar hos andra koncernföretag;;;
;;;;;1328;Ackumulerade nedskrivningar av långfristiga fordringar hos koncernföretag;;;
;;133;Andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;1330;Andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;1331;Andelar i intresseföretag;;;
;;;;;1332;Ackumulerade nedskrivningar av andelar i intresseföretag;;;
;;;;;1333;Andelar i gemensamt styrda företag;;;
;;;;;1334;Ackumulerade nedskrivningar av andelar i gemensamt styrda företag;;;
;;;;;1336;Andelar i övriga företag som det finns ett ägarintresse i;;;
;;;;;1337;Ackumulerade nedskrivningar av andelar i övriga företag som det finns ett ägarintresse i;;;
;;;;;1338;Ackumulerade nedskrivningar av andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;134;Långfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;1340;Långfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;1341;Långfristiga fordringar hos intresseföretag;;;
;;;;;1342;Ackumulerade nedskrivningar av långfristiga fordringar hos intresseföretag;;;
;;;;;1343;Långfristiga fordringar hos gemensamt styrda företag;;;
;;;;;1344;Ackumulerade nedskrivningar av långfristiga fordringar hos gemensamt styrda företag;;;
;;;;;1346;Långfristiga fordringar hos övriga företag som det finns ett ägarintresse i;;;
;;;;;1347;Ackumulerade nedskrivningar av långfristiga fordringar hos övriga företag som det finns ett ägarintresse i;;;
;;;;;1348;Ackumulerade nedskrivningar av långfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;■;135;Andelar och värdepapper i andra företag;■;1350;Andelar och värdepapper i andra företag;;;
;;;;;1351;Andelar i noterade företag;;;
;;;;;1352;Andra andelar;;;
;;;;;1353;Andelar i bostadsrättsföreningar;;;
;;;;;1354;Obligationer;;;
;;;;;1356;Andelar i ekonomiska föreningar, övriga företag;;;
;;;;;1357;Andelar i handelsbolag, andra företag;;;
;;;;;1358;Ackumulerade nedskrivningar av andra andelar och värdepapper;;;
;;136;Lån till delägare eller närstående enligt ABL, långfristig del;;1360;Lån till delägare eller närstående enligt ABL, långfristig del;;;
;;;;;1369;Ackumulerade nedskrivningar av lån till delägare eller närstående enligt ABL, långfristig del;;;
;[Ej K2];137;Uppskjuten skattefordran;[Ej K2];1370;Uppskjuten skattefordran;;;
;■;138;Andra långfristiga fordringar;■;1380;Andra långfristiga fordringar;;;
;;;;;1381;Långfristiga reversfordringar;;;
;;;;;1382;Långfristiga fordringar hos anställda;;;
;;;;;1383;Lämnade depositioner, långfristiga;;;
;;;;;1384;Derivat;;;
;;;;;1385;Kapitalförsäkring;|;;
;;;;;1387;Långfristiga kontraktsfordringar;;;
;;;;;1388;Långfristiga kundfordringar;;;
;;;;;1389;Ackumulerade nedskrivningar av andra långfristiga fordringar;;;
;;14;Lager, produkter i arbete och pågående arbeten;;;;;;
;■;141;Lager av råvaror;■;1410;Lager av råvaror;;;
;;;;■;1419;Förändring av lager av råvaror;;;
;;142;Lager av tillsatsmaterial och förnödenheter;;1420;Lager av tillsatsmaterial och förnödenheter;;;
;;;;;1429;Förändring av lager av tillsatsmaterial och förnödenheter;;;
;■;144;Produkter i arbete;■;1440;Produkter i arbete;;;
;;;;■;1449;Förändring av produkter i arbete;;;
;■;145;Lager av färdiga varor;■;1450;Lager av färdiga varor;;;
;;;;■;1459;Förändring av lager av färdiga varor;;;
;■;146;Lager av handelsvaror;■;1460;Lager av handelsvaror;;;
;;;;;1465;Lager av varor VMB;;;
;;;;;1466;Nedskrivning av varor VMB;;;
;;;;;1467;Lager av varor VMB förenklad;;;
;;;;■;1469;Förändring av lager av handelsvaror;;;
;■;147;Pågående arbeten;■;1470;Pågående arbeten;;;
;;;;;1471;Pågående arbeten, nedlagda kostnader;;;
;;;;;1478;Pågående arbeten, fakturering;;;
;;;;■;1479;Förändring av pågående arbeten;;;
;■;148;Förskott för varor och tjänster;■;1480;Förskott för varor och tjänster;;;
;;;;;1481;Remburser;;;
;;;;;1489;Övriga förskott till leverantörer;;;
;■;149;Övriga lagertillgångar;■;1490;Övriga lagertillgångar;;;
;;;;;1491;Lager av värdepapper;;;
;;;;;1492;Lager av fastigheter;;;
;;;;;1493;Djur som klassificeras som omsättningstillgång;;;
;;15;Kundfordringar;;;;;;
;■;151;Kundfordringar;■;1510;Kundfordringar;;;
;;;;;1511;Kundfordringar;;;
;;;;;1512;Belånade kundfordringar (factoring);;;
;;;;■;1513;Kundfordringar – delad faktura;;;
;;;;;1516;Tvistiga kundfordringar;;;
;;;;;1518;Ej reskontraförda kundfordringar;;;
;;;;■;1519;Nedskrivning av kundfordringar;;;
;;152;Växelfordringar;;1520;Växelfordringar;;;
;;;;;1525;Osäkra växelfordringar;;;
;;;;;1529;Nedskrivning av växelfordringar;;;
;;153;Kontraktsfordringar;;1530;Kontraktsfordringar;;;
;;;;;1531;Kontraktsfordringar;;;
;;;;;1532;Belånade kontraktsfordringar;;;
;;;;;1536;Tvistiga kontraktsfordringar;;;
;;;;;1539;Nedskrivning av kontraktsfordringar;;;
;;155;Konsignationsfordringar;;1550;Konsignationsfordringar;;;
;;156;Kundfordringar hos koncernföretag;;1560;Kundfordringar hos koncernföretag;;;
;;;;;1561;Kundfordringar hos moderföretag;;;
;;;;;1562;Kundfordringar hos dotterföretag;;;
;;;;;1563;Kundfordringar hos andra koncernföretag;;;
;;;;;1568;Ej reskontraförda kundfordringar hos koncernföretag;;;
;;;;;1569;Nedskrivning av kundfordringar hos koncernföretag;;;
;;157;Kundfordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;1570;Kundfordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;1571;Kundfordringar hos intresseföretag;;;
;;;;;1572;Kundfordringar hos gemensamt styrda företag;;;
;;;;;1573;Kundfordringar hos övriga företag som det finns ett ägarintresse i;;;
;■;158;Fordringar för kontokort och kuponger;■;1580;Fordringar för kontokort och kuponger;;;
;;16;Övriga kortfristiga fordringar;;;;;;
;■;161;Kortfristiga fordringar hos anställda;■;1610;Kortfristiga fordringar hos anställda;;;
;;;;;1611;Reseförskott;;;
;;;;;1612;Kassaförskott;;;
;;;;;1613;Övriga förskott;;;
;;;;;1614;Tillfälliga lån till anställda;;;
;;;;;1619;Övriga fordringar hos anställda;;;
;;162;Upparbetad men ej fakturerad intäkt;;1620;Upparbetad men ej fakturerad intäkt;;;
;■;163;Avräkning för skatter och avgifter (skattekonto);■;1630;Avräkning för skatter och avgifter (skattekonto);;;
;■;164;Skattefordringar;■;1640;Skattefordringar;;;
;■;165;Momsfordran;■;1650;Momsfordran;;;
;;166;Kortfristiga fordringar hos koncernföretag;;1660;Kortfristiga fordringar hos koncernföretag;;;
;;;;;1661;Kortfristiga fordringar hos moderföretag;;;
;;;;;1662;Kortfristiga fordringar hos dotterföretag;;;
;;;;;1663;Kortfristiga fordringar hos andra koncernföretag;;;
;;167;Kortfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;1670;Kortfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;1671;Kortfristiga fordringar hos intresseföretag;;;
;;;;;1672;Kortfristiga fordringar hos gemensamt styrda företag;;;
;;;;;1673;Kortfristiga fordringar hos övriga företag som det finns ett ägarintresse i;;;
;■;168;Andra kortfristiga fordringar;■;1680;Andra kortfristiga fordringar;;;
;;;;;1681;Utlägg för kunder;;;
;;;;;1682;Kortfristiga lånefordringar;;;
;;;;;1683;Derivat;;;
;;;;;1684;Kortfristiga fordringar hos leverantörer;;;
;;;;;1685;Kortfristiga fordringar hos delägare eller närstående;;;
;;;;;1687;Kortfristig del av långfristiga fordringar;;;
;;;;;1688;Fordran arbetsmarknadsförsäkringar;;;
;;;;;1689;Övriga kortfristiga fordringar;;;
;;169;Fordringar för tecknat men ej inbetalt aktiekapital;;1690;Fordringar för tecknat men ej inbetalt aktiekapital;;;
;;17;Förutbetalda kostnader och upplupna intäkter;;;;;;
;■;171;Förutbetalda hyreskostnader;■;1710;Förutbetalda hyreskostnader;;;
;■;172;Förutbetalda leasingavgifter;■;1720;Förutbetalda leasingavgifter;;;
;■;173;Förutbetalda försäkringspremier;■;1730;Förutbetalda försäkringspremier;;;
;■;174;Förutbetalda räntekostnader;■;1740;Förutbetalda räntekostnader;;;
;■;175;Upplupna hyresintäkter;■;1750;Upplupna hyresintäkter;;;
;■;176;Upplupna ränteintäkter;■;1760;Upplupna ränteintäkter;;;
;;177;Tillgångar av kostnadsnatur;;1770;Tillgångar av kostnadsnatur;;;
;;178;Upplupna avtalsintäkter;;1780;Upplupna avtalsintäkter;;;
;■;179;Övriga förutbetalda kostnader och upplupna intäkter;■;1790;Övriga förutbetalda kostnader och upplupna intäkter;;;
;;18;Kortfristiga placeringar;;;;;;
;■;181;Andelar i börsnoterade företag;■;1810;Andelar i börsnoterade företag;;;
;;182;Obligationer;;1820;Obligationer;;;
;;183;Konvertibla skuldebrev;;1830;Konvertibla skuldebrev;;;
;;186;Andelar i koncernföretag, kortfristigt;;1860;Andelar i koncernföretag, kortfristigt;;;
;■;188;Andra kortfristiga placeringar;■;1880;Andra kortfristiga placeringar;;;
;;;;;1886;Derivat;;;
;;;;;1889;Andelar i övriga företag;;;
;■;189;Nedskrivning av kortfristiga placeringar;■;1890;Nedskrivning av kortfristiga placeringar;;;
;;19;Kassa och bank;;;;;;
;■;191;Kassa;■;1910;Kassa;;;
;;;;;1911;Huvudkassa;;;
;;;;;1912;Kassa 2;;;
;;;;;1913;Kassa 3;;;
;■;192;PlusGiro;■;1920;PlusGiro;;;
;■;193;Företagskonto/checkkonto/affärskonto;■;1930;Företagskonto/checkkonto/affärskonto;;;
;■;194;Övriga bankkonton;■;1940;Övriga bankkonton;;;
;;195;Bankcertifikat;;1950;Bankcertifikat;;;
;;196;Koncernkonto moderföretag;;1960;Koncernkonto moderföretag;;;
;;197;Särskilda bankkonton;;1970;Särskilda bankkonton;;;
;;;;;1972;Upphovsmannakonto;;;
;;;;;1973;Skogskonto;;;
;;;;;1974;Spärrade bankmedel;;;
;;;;;1979;Övriga särskilda bankkonton;;;
;;198;Valutakonton;;1980;Valutakonton;;;
;;199;Redovisningsmedel;;1990;Redovisningsmedel;;;
;;2;Eget kapital och skulder;;;;;;
;;20;Eget kapital;;;;;;
;;201;Eget kapital (enskild firma);■;2010;Eget kapital;;;
;;;;■;2011;Egna varuuttag;;;
;;;;■;2013;Övriga egna uttag;;;
;;;;■;2017;Årets kapitaltillskott;;;
;;;;■;2018;Övriga egna insättningar;;;
;;;;■;2019;Årets resultat;;;
;;201;Eget kapital, delägare 1;■;2010;Eget kapital;;;
;;;;■;2011;Egna varuuttag;;;
;;;;■;2013;Övriga egna uttag;;;
;;;;■;2017;Årets kapitaltillskott;;;
;;;;■;2018;Övriga egna insättningar;;;
;;;;■;2019;Årets resultat, delägare 1;;;
;;202;Eget kapital, delägare 2;■;2020;Eget kapital;;;
;;;;■;2021;Egna varuuttag;;;
;;;;■;2023;Övriga egna uttag;;;
;;;;■;2027;Årets kapitaltillskott;;;
;;;;■;2028;Övriga egna insättningar;;;
;;;;■;2029;Årets resultat, delägare 2;;;
;;203;Eget kapital, delägare 3;■;2030;Eget kapital;;;
;;;;■;2031;Egna varuuttag;;;
;;;;■;2033;Övriga egna uttag;;;
;;;;■;2037;Årets kapitaltillskott;;;
;;;;■;2038;Övriga egna insättningar;;;
;;;;■;2039;Årets resultat, delägare 3;;;
;;204;Eget kapital, delägare 4;■;2040;Eget kapital;;;
;;;;■;2041;Egna varuuttag;;;
;;;;■;2043;Övriga egna uttag;;;
;;;;■;2047;Årets kapitaltillskott;;;
;;;;■;2048;Övriga egna insättningar;;;
;;;;■;2049;Årets resultat, delägare 4;;;
;;205;Avsättning till expansionsfond;;2050;Avsättning till expansionsfond;;;
;■;206;Eget kapital i ideella föreningar, stiftelser och registrerade trossamfund;■;2060;Eget kapital i ideella föreningar, stiftelser och registrerade trossamfund;;;
;;;;;2061;Eget kapital/stiftelsekapital/grundkapital;;;
;;;;;2065;Förändring i fond för verkligt värde;;;
;;;;;2066;Värdesäkringsfond;;;
;;;;;2067;Balanserad vinst eller förlust/balanserat kapital;;;
;;;;;2068;Vinst eller förlust från föregående år;;;
;;;;;2069;Årets resultat;;;
;■;207;Ändamålsbestämda medel;■;2070;Ändamålsbestämda medel;;;
;;;;;2071;Ändamål 1;;;
;;;;;2072;Ändamål 2;;;
;;208;Bundet eget kapital;;2080;Bundet eget kapital;;;
;;;;■;2081;Aktiekapital;;;
;;;;;2082;Ej registrerat aktiekapital;;;
;;;;■;2083;Medlemsinsatser;;;
;;;;;2084;Förlagsinsatser;;;
;;;;;2085;Uppskrivningsfond;;;
;;;;■;2086;Reservfond;;;
;;;;;2087;Insatsemission;;;
;;;;;2087;Bunden överkursfond;;;
;;;;;2088;Fond för yttre underhåll;;;
;;;;[Ej K2];2089;Fond för utvecklingsutgifter;;;
;■;209;Fritt eget kapital;■;2090;Fritt eget kapital;;;
;;;;■;2091;Balanserad vinst eller förlust;;;
;;;;[Ej K2];2092;Mottagna/lämnade koncernbidrag;;;
;;;;;2093;Erhållna aktieägartillskott;;;
;;;;;2094;Egna aktier;;;
;;;;;2095;Fusionsresultat;;;
;;;;[Ej K2];2096;Fond för verkligt värde;;;
;;;;;2097;Fri överkursfond;;;
;;;;■;2098;Vinst eller förlust från föregående år;;;
;;;;■;2099;Årets resultat;;;
;;21;Obeskattade reserver;;;;;;
;;211;Periodiseringsfonder;;2110;Periodiseringsfonder;;;
;■;212;Periodiseringsfond 2020;■;2120;Periodiseringsfond 2020;;;
;;;;■;2121;Periodiseringsfond 2021;;;
;;;;■;2122;Periodiseringsfond 2022;;;
;;;;■;2123;Periodiseringsfond 2023;;;
;;;;■;2125;Periodiseringsfond 2015;;;
;;;;■;2126;Periodiseringsfond 2016;;;
;;;;■;2127;Periodiseringsfond 2017;;;
;;;;■;2128;Periodiseringsfond 2018;;;
;;;;■;2129;Periodiseringsfond 2019;;;
;;213;Periodiseringsfond 2020 – nr 2;;2130;Periodiseringsfond 2020 – nr 2;;;
;;;;;2131;Periodiseringsfond 2021 – nr 2;;;
;;;;;2132;Periodiseringsfond 2022 – nr 2;;;
;;;;;2133;Periodiseringsfond 2023 – nr 2;;;
;;;;;2134;Periodiseringsfond 2024 – nr 2;;;
;;;;;2135;Periodiseringsfond 2015 – nr 2;;;
;;;;;2136;Periodiseringsfond 2016 – nr 2;;;
;;;;;2137;Periodiseringsfond 2017 – nr 2;;;
;;;;;2138;Periodiseringsfond 2018 – nr 2;;;
;;;;;2139;Periodiseringsfond 2019 – nr 2;;;
;■;215;Ackumulerade överavskrivningar;■;2150;Ackumulerade överavskrivningar;;;
;;;;;2151;Ackumulerade överavskrivningar på immateriella anläggningstillgångar;;;
;;;;;2152;Ackumulerade överavskrivningar på byggnader och markanläggningar;;;
;;;;;2153;Ackumulerade överavskrivningar på maskiner och inventarier;;;
;;216;Ersättningsfond;;2160;Ersättningsfond;;;
;;;;;2161;Ersättningsfond maskiner och inventarier;;;
;;;;;2162;Ersättningsfond byggnader och markanläggningar;;;
;;;;;2164;Ersättningsfond för djurlager i jordbruk och renskötsel;;;
;;219;Övriga obeskattade reserver;;2190;Övriga obeskattade reserver;;;
;;;;;2196;Lagerreserv;;;
;;;;;2199;Övriga obeskattade reserver;;;
;;22;Avsättningar;;;;;;
;■;221;Avsättningar för pensioner enligt tryggandelagen;■;2210;Avsättningar för pensioner enligt tryggandelagen;;;
;■;222;Avsättningar för garantier;■;2220;Avsättningar för garantier;;;
;;223;Övriga avsättningar för pensioner och liknande förpliktelser;;2230;Övriga avsättningar för pensioner och liknande förpliktelser;;;
;[Ej K2];224;Avsättningar för uppskjutna skatter;[Ej K2];2240;Avsättningar för uppskjutna skatter;;;
;;225;Övriga avsättningar för skatter;;2250;Övriga avsättningar för skatter;;;
;;;;;2252;Avsättningar för tvistiga skatter;;;
;;;;;2253;Avsättningar särskild löneskatt, deklarationspost;;;
;■;229;Övriga avsättningar;■;2290;Övriga avsättningar;;;
;;23;Långfristiga skulder;;;;;;
;;231;Obligations- och förlagslån;;2310;Obligations- och förlagslån;;;
;;232;Konvertibla lån och liknande;;2320;Konvertibla lån och liknande;;;
;;;;;2321;Konvertibla lån;;;
;;;;;2322;Lån förenade med optionsrätt;;;
;;;;;2323;Vinstandelslån;;;
;;;;;2324;Kapitalandelslån;;;
;■;233;Checkräkningskredit;■;2330;Checkräkningskredit;;;
;;;;;2331;Checkräkningskredit 1;;;
;;;;;2332;Checkräkningskredit 2;;;
;;234;Byggnadskreditiv;;2340;Byggnadskreditiv;;;
;■;235;Andra långfristiga skulder till kreditinstitut;■;2350;Andra långfristiga skulder till kreditinstitut;;;
;;;;;2351;Fastighetslån, långfristig del;;;
;;;;;2355;Långfristiga lån i utländsk valuta från kreditinstitut;;;
;;;;;2359;Övriga långfristiga lån från kreditinstitut;;;
;;236;Långfristiga skulder till koncernföretag;;2360;Långfristiga skulder till koncernföretag;;;
;;;;;2361;Långfristiga skulder till moderföretag;;;
;;;;;2362;Långfristiga skulder till dotterföretag;;;
;;;;;2363;Långfristiga skulder till andra koncernföretag;;;
;;237;Långfristiga skulder till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;2370;Långfristiga skulder till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;2371;Långfristiga skulder till intresseföretag;;;
;;;;;2372;Långfristiga skulder till gemensamt styrda företag;;;
;;;;;2373;Långfristiga skulder till övriga företag som det finns ett ägarintresse i;;;
;■;239;Övriga långfristiga skulder;■;2390;Övriga långfristiga skulder;;;
;;;;;2391;Avbetalningskontrakt, långfristig del;;;
;;;;;2392;Villkorliga långfristiga skulder;;;
;;;;■;2393;Lån från närstående personer, långfristig del;;;
;;;;;2394;Långfristiga leverantörskrediter;;;
;;;;;2395;Andra långfristiga lån i utländsk valuta;;;
;;;;;2396;Derivat;;;
;;;;;2397;Mottagna depositioner, långfristiga;;;
;;;;;2399;Övriga långfristiga skulder;;;
;;24;Kortfristiga skulder till kreditinstitut, kunder och leverantörer;;;;;;
;■;241;Andra kortfristiga låneskulder till kreditinstitut;■;2410;Andra kortfristiga låneskulder till kreditinstitut;;;
;;;;;2411;Kortfristiga lån från kreditinstitut;;;
;;;;;2412;Byggnadskreditiv, kortfristig del;;;
;;;;;2417;Kortfristig del av långfristiga skulder till kreditinstitut;;;
;;;;;2419;Övriga kortfristiga skulder till kreditinstitut;;;
;■;242;Förskott från kunder;■;2420;Förskott från kunder;;;
;;;;;2421;Ej inlösta presentkort;;;
;;;;;2429;Övriga förskott från kunder;;;
;;243;Pågående arbeten;;2430;Pågående arbeten;;;
;;;;;2431;Pågående arbeten, fakturering;;;
;;;;;2438;Pågående arbeten, nedlagda kostnader;;;
;;;;;2439;Beräknad förändring av pågående arbeten;;;
;■;244;Leverantörsskulder;■;2440;Leverantörsskulder;;;
;;;;;2441;Leverantörsskulder;;;
;;;;;2443;Konsignationsskulder;;;
;;;;;2445;Tvistiga leverantörsskulder;;;
;;;;;2448;Ej reskontraförda leverantörsskulder;;;
;;245;Fakturerad men ej upparbetad intäkt;;2450;Fakturerad men ej upparbetad intäkt;;;
;;246;Leverantörsskulder till koncernföretag;;2460;Leverantörsskulder till koncernföretag;;;
;;;;;2461;Leverantörsskulder till moderföretag;;;
;;;;;2462;Leverantörsskulder till dotterföretag;;;
;;;;;2463;Leverantörsskulder till andra koncernföretag;;;
;;247;Leverantörsskulder till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;2470;Leverantörsskulder till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;2471;Leverantörsskulder till intresseföretag;;;
;;;;;2472;Leverantörsskulder till gemensamt styrda företag;;;
;;;;;2473;Leverantörsskulder till övriga företag som det finns ett ägarintresse i;;;
;■;248;Checkräkningskredit, kortfristig;■;2480;Checkräkningskredit, kortfristig;;;
;■;249;Övriga kortfristiga skulder till kreditinstitut, kunder och leverantörer;■;2490;Övriga kortfristiga skulder till kreditinstitut, kunder och leverantörer;;;
;;;;;2491;Avräkning spelarrangörer;;;
;;;;;2492;Växelskulder;;;
;;;;;2499;Andra övriga kortfristiga skulder;;;
;;25;Skatteskulder;;;;;;
;■;251;Skatteskulder;■;2510;Skatteskulder;;;
;;;;;2512;Beräknad inkomstskatt;;;
;;;;;2513;Beräknad fastighetsskatt/fastighetsavgift;;;
;;;;;2514;Beräknad särskild löneskatt på pensionskostnader;;;
;;;;;2515;Beräknad avkastningsskatt;;;
;;;;;2517;Beräknad utländsk skatt;;;
;;;;;2518;Betald F-skatt;;;
;;26;Moms och särskilda punktskatter;;;;;;
;■;261;Utgående moms, 25 %;■;2610;Utgående moms, 25 %;;;
;;;;■;2611;Utgående moms på försäljning inom Sverige, 25 %;;;
;;;;■;2612;Utgående moms på egna uttag, 25 %;;;
;;;;■;2613;Utgående moms för uthyrning, 25 %;;;
;;;;■;2614;Utgående moms omvänd skattskyldighet, 25 %;;;
;;;;■;2615;Utgående moms import av varor, 25 %;;;
;;;;■;2616;Utgående moms VMB 25 %;;;
;;;;;2618;Vilande utgående moms, 25 %;;;
;■;262;Utgående moms, 12 %;■;2620;Utgående moms, 12 %;;;
;;;;■;2621;Utgående moms på försäljning inom Sverige, 12 %;;;
;;;;■;2622;Utgående moms på egna uttag, 12 %;;;
;;;;■;2623;Utgående moms för uthyrning, 12 %;;;
;;;;■;2624;Utgående moms omvänd skattskyldighet, 12 %;;;
;;;;■;2625;Utgående moms import av varor, 12 %;;;
;;;;■;2626;Utgående moms VMB 12 %;;;
;;;;;2628;Vilande utgående moms, 12 %;;;
;■;263;Utgående moms, 6 %;■;2630;Utgående moms, 6 %;;;
;;;;■;2631;Utgående moms på försäljning inom Sverige, 6 %;;;
;;;;■;2632;Utgående moms på egna uttag, 6 %;;;
;;;;■;2633;Utgående moms för uthyrning, 6 %;;;
;;;;■;2634;Utgående moms omvänd skattskyldighet, 6 %;;;
;;;;■;2635;Utgående moms import av varor, 6 %;;;
;;;;■;2636;Utgående moms VMB 6 %;;;
;;;;;2638;Vilande utgående moms, 6 %;;;
;■;264;Ingående moms;■;2640;Ingående moms;;;
;;;;■;2641;Debiterad ingående moms;;;
;;;;■;2642;Debiterad ingående moms i anslutning till frivillig skattskyldighet;;;
;;;;■;2645;Beräknad ingående moms på förvärv från utlandet;;;
;;;;■;2646;Ingående moms på uthyrning;;;
;;;;■;2647;Ingående moms omvänd skattskyldighet varor och tjänster i Sverige;;;
;;;;■;2648;Vilande ingående moms;;;
;;;;■;2649;Ingående moms, blandad verksamhet;;;
;■;265;Redovisningskonto för moms;■;2650;Redovisningskonto för moms;;;
;;266;Särskilda punktskatter;;2660;Särskilda punktskatter;;;
;;;;;2661;Reklamskatt;;;
;;;;;2669;Övriga punktskatter;;;
;;27;Personalens skatter, avgifter och löneavdrag;;;;;;
;■;271;Personalskatt;■;2710;Personalskatt;;;
;■;273;Lagstadgade sociala avgifter och särskild löneskatt;■;2730;Lagstadgade sociala avgifter och särskild löneskatt;;;
;;;;;2731;Avräkning lagstadgade sociala avgifter;;;
;;;;;2732;Avräkning särskild löneskatt;;;
;■;274;Avtalade sociala avgifter;■;2740;Avtalade sociala avgifter;;;
;;275;Utmätning i lön m.m.;;2750;Utmätning i lön m.m.;;;
;;276;Semestermedel;;2760;Semestermedel;;;
;;;;;2761;Avräkning semesterlöner;;;
;;;;;2762;Semesterlönekassa;;;
;■;279;Övriga löneavdrag;■;2790;Övriga löneavdrag;;;
;;;;;2791;Personalens intressekonto;;;
;;;;;2792;Lönsparande;;;
;;;;;2793;Gruppförsäkringspremier;;;
;;;;;2794;Fackföreningsavgifter;;;
;;;;;2795;Mätnings- och granskningsarvoden;;;
;;;;;2799;Övriga löneavdrag;;;
;;28;Övriga kortfristiga skulder;;;;;;
;;281;Avräkning för factoring och belånade kontraktsfordringar;;2810;Avräkning för factoring och belånade kontraktsfordringar;;;
;;;;;2811;Avräkning för factoring;;;
;;;;;2812;Avräkning för belånade kontraktsfordringar;;;
;■;282;Kortfristiga skulder till anställda;■;2820;Kortfristiga skulder till anställda;;;
;;;;;2821;Löneskulder;;;
;;;;;2822;Reseräkningar;;;
;;;;;2823;Tantiem, gratifikationer;;;
;;;;;2829;Övriga kortfristiga skulder till anställda;;;
;;283;Avräkning för annans räkning;;2830;Avräkning för annans räkning;;;
;■;284;Kortfristiga låneskulder;■;2840;Kortfristiga låneskulder;;;
;;;;;2841;Kortfristig del av långfristiga skulder;;;
;;;;;2849;Övriga kortfristiga låneskulder;;;
;;285;Avräkning för skatter och avgifter (skattekonto);;2850;Avräkning för skatter och avgifter (skattekonto);;;
;;;;;2852;Anståndsbelopp för moms, arbetsgivaravgifter och personalskatt;;;
;;286;Kortfristiga skulder till koncernföretag;;2860;Kortfristiga skulder till koncernföretag;;;
;;;;;2861;Kortfristiga skulder till moderföretag;;;
;;;;;2862;Kortfristiga skulder till dotterföretag;;;
;;;;;2863;Kortfristiga skulder till andra koncernföretag;;;
;;287;Kortfristiga skulder till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;2870;Kortfristiga skulder till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;2871;Kortfristiga skulder till intresseföretag;;;
;;;;;2872;Kortfristiga skulder till gemensamt styrda företag;;;
;;;;;2873;Kortfristiga skulder till övriga företag som det finns ett ägarintresse i;;;
;;288;Skuld erhållna bidrag;;2880;Skuld erhållna bidrag;;;
;■;289;Övriga kortfristiga skulder;■;2890;Övriga kortfristiga skulder;;;
;;;;;2891;Skulder under indrivning;;;
;;;;;2892;Inre reparationsfond/underhållsfond;;;
;;;;;2893;Skulder till närstående personer, kortfristig del;;;
;;;;;2895;Derivat (kortfristiga skulder);;;
;;;;;2897;Mottagna depositioner, kortfristiga;;;
;;;;;2898;Outtagen vinstutdelning;;;
;;;;;2899;Övriga kortfristiga skulder;;;
;;29;Upplupna kostnader och förutbetalda intäkter;;;;;;
;■;291;Upplupna löner;■;2910;Upplupna löner;;;
;;;;;2911;Löneskulder;;;
;;;;;2912;Ackordsöverskott;;;
;;;;;2919;Övriga upplupna löner;;;
;■;292;Upplupna semesterlöner;■;2920;Upplupna semesterlöner;;;
;;293;Upplupna pensionskostnader;;2930;Upplupna pensionskostnader;;;
;;;;;2931;Upplupna pensionsutbetalningar;;;
;■;294;Upplupna lagstadgade sociala och andra avgifter;■;2940;Upplupna lagstadgade sociala och andra avgifter;;;
;;;;;2941;Beräknade upplupna lagstadgade sociala avgifter;;;
;;;;;2942;Beräknad upplupen särskild löneskatt;;;
;;;;;2943;Beräknad upplupen särskild löneskatt på pensionskostnader, deklarationspost;;;
;;;;;2944;Beräknad upplupen avkastningsskatt på pensionskostnader;;;
;■;295;Upplupna avtalade sociala avgifter;■;2950;Upplupna avtalade sociala avgifter;;;
;;;;;2951;Upplupna avtalade arbetsmarknadsförsäkringar;;;
;;;;;2959;Upplupna avtalade pensionsförsäkringsavgifter, deklarationspost;;;
;■;296;Upplupna räntekostnader;■;2960;Upplupna räntekostnader;;;
;■;297;Förutbetalda intäkter;■;2970;Förutbetalda intäkter;;;
;;;;;2971;Förutbetalda hyresintäkter;;;
;;;;;2972;Förutbetalda medlemsavgifter;;;
;;;;;2979;Övriga förutbetalda intäkter;;;
;;298;Upplupna avtalskostnader;;2980;Upplupna avtalskostnader;;;
;■;299;Övriga upplupna kostnader och förutbetalda intäkter;■;2990;Övriga upplupna kostnader och förutbetalda intäkter;;;
;;;;;2991;Beräknat arvode för bokslut;;;
;;;;;2992;Beräknat arvode för revision;;;
;;;;;2993;Ospecificerad skuld till leverantörer;;;
;;;;;2998;Övriga upplupna kostnader och förutbetalda intäkter;;;
;;;;■;2999;OBS-konto;;;
;;3;Rörelsens inkomster/intäkter;;;;;;
;;30;Huvudintäkter;;;;;;
;■;300;Försäljning inom Sverige;■;3000;Försäljning inom Sverige;;;
;;;;■;3001;Försäljning inom Sverige, 25 % moms;;;
;;;;■;3002;Försäljning inom Sverige, 12 % moms;;;
;;;;■;3003;Försäljning inom Sverige, 6 % moms;;;
;;;;■;3004;Försäljning inom Sverige, momsfri;;;
;;31;Huvudintäkter;;;;;;
;■;310;Försäljning av varor utanför Sverige;■;3100;Försäljning av varor utanför Sverige;;;
;;;;■;3105;Försäljning varor till land utanför EU;;;
;;;;■;3106;Försäljning varor till annat EU-land, momspliktig;;;
;;;;■;3108;Försäljning varor till annat EU-land, momsfri;;;
;;32;Huvudintäkter;;;;;;
;■;320;Försäljning VMB och omvänd moms;■;3200;Försäljning VMB och omvänd moms;;;
;■;321;Försäljning positiv VMB 25 %;■;3211;Försäljning positiv VMB 25 %;;;
;;;;■;3212;Försäljning negativ VMB 25 %;;;
;■;323;Försäljning inom byggsektorn, omvänd skattskyldighet moms;■;3231;Försäljning inom byggsektorn, omvänd skattskyldighet moms;;;
;;33;Huvudintäkter;;;;;;
;■;330;Försäljning av tjänster utanför Sverige;■;3300;Försäljning av tjänster utanför Sverige;;;
;;;;■;3305;Försäljning tjänster till land utanför EU;;;
;;;;■;3308;Försäljning tjänster till annat EU-land;;;
;;34;Huvudintäkter;;;;;;
;■;340;Försäljning, egna uttag;■;3400;Försäljning, egna uttag;;;
;;;;■;3401;Egna uttag momspliktiga, 25 %;;;
;;;;■;3402;Egna uttag momspliktiga, 12 %;;;
;;;;■;3403;Egna uttag momspliktiga, 6 %;;;
;;;;■;3404;Egna uttag, momsfria;;;
;;35;Fakturerade kostnader;;;;;;
;■;350;Fakturerade kostnader (gruppkonto);■;3500;Fakturerade kostnader (gruppkonto);;;
;■;351;Fakturerat emballage;■;3510;Fakturerat emballage;;;
;;;;;3511;Fakturerat emballage;;;
;;;;;3518;Returnerat emballage;;;
;■;352;Fakturerade frakter;■;3520;Fakturerade frakter;;;
;;;;■;3521;Fakturerade frakter, EU-land;;;
;;;;■;3522;Fakturerade frakter, export;;;
;■;353;Fakturerade tull- och speditionskostnader m.m.;■;3530;Fakturerade tull- och speditionskostnader m.m.;;;
;■;354;Faktureringsavgifter;■;3540;Faktureringsavgifter;;;
;;;;■;3541;Faktureringsavgifter, EU-land;;;
;;;;■;3542;Faktureringsavgifter, export;;;
;;355;Fakturerade resekostnader;;3550;Fakturerade resekostnader;;;
;;356;Fakturerade kostnader till koncernföretag;;3560;Fakturerade kostnader till koncernföretag;;;
;;;;;3561;Fakturerade kostnader till moderföretag;;;
;;;;;3562;Fakturerade kostnader till dotterföretag;;;
;;;;;3563;Fakturerade kostnader till andra koncernföretag;;;
;;357;Fakturerade kostnader till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;3570;Fakturerade kostnader till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;359;Övriga fakturerade kostnader;;3590;Övriga fakturerade kostnader;;;
;;36;Rörelsens sidointäkter;;;;;;
;■;360;Rörelsens sidointäkter (gruppkonto);■;3600;Rörelsens sidointäkter (gruppkonto);;;
;;361;Försäljning av material;;3610;Försäljning av material;;;
;;;;;3611;Försäljning av råmaterial;;;
;;;;;3612;Försäljning av skrot;;;
;;;;;3613;Försäljning av förbrukningsmaterial;;;
;;;;;3619;Försäljning av övrigt material;;;
;;362;Tillfällig uthyrning av personal;;3620;Tillfällig uthyrning av personal;;;
;;363;Tillfällig uthyrning av transportmedel;;3630;Tillfällig uthyrning av transportmedel;;;
;;367;Intäkter från värdepapper;;3670;Intäkter från värdepapper;;;
;;;;;3671;Försäljning av värdepapper;;;
;;;;;3672;Utdelning från värdepapper;;;
;;;;;3679;Övriga intäkter från värdepapper;;;
;;368;Management fees;;3680;Management fees;;;
;;369;Övriga sidointäkter;;3690;Övriga sidointäkter;;;
;;37;Intäktskorrigeringar;;;;;;
;;370;Intäktskorrigeringar (gruppkonto);;3700;Intäktskorrigeringar (gruppkonto);;;
;;371;Ofördelade intäktsreduktioner;;3710;Ofördelade intäktsreduktioner;;;
;■;373;Lämnade rabatter;■;3730;Lämnade rabatter;;;
;;;;;3731;Lämnade kassarabatter;;;
;;;;;3732;Lämnade mängdrabatter;;;
;■;374;Öres- och kronutjämning;■;3740;Öres- och kronutjämning;;;
;;375;Punktskatter;;3750;Punktskatter;;;
;;;;;3751;Intäktsförda punktskatter (kreditkonto);;;
;;;;;3752;Skuldförda punktskatter (debetkonto);;;
;;379;Övriga intäktskorrigeringar;;3790;Övriga intäktskorrigeringar;;;
;;38;Aktiverat arbete för egen räkning;;;;;;
;■;380;Aktiverat arbete för egen räkning (gruppkonto);■;3800;Aktiverat arbete för egen räkning (gruppkonto);;;
;;384;Aktiverat arbete (material);;3840;Aktiverat arbete (material);;;
;;385;Aktiverat arbete (omkostnader);;3850;Aktiverat arbete (omkostnader);;;
;;387;Aktiverat arbete (personal);;3870;Aktiverat arbete (personal);;;
;;39;Övriga rörelseintäkter;;;;;;
;■;390;Övriga rörelseintäkter (gruppkonto);■;3900;Övriga rörelseintäkter (gruppkonto);;;
;;391;Hyres- och arrendeintäkter;;3910;Hyres- och arrendeintäkter;;;
;;;;;3911;Hyresintäkter;;;
;;;;;3912;Arrendeintäkter;;;
;;;;■;3913;Frivilligt momspliktiga hyresintäkter;;;
;;;;;3914;Övriga momspliktiga hyresintäkter;;;
;;392;Provisionsintäkter, licensintäkter och royalties;;3920;Provisionsintäkter, licensintäkter och royalties;;;
;;;;;3921;Provisionsintäkter;;;
;;;;;3922;Licensintäkter och royalties;;;
;;;;;3925;Franchiseintäkter;;;
;[Ej K2];394;Orealiserade negativa/positiva värdeförändringar på säkringsinstrument;[Ej K2];3940;Orealiserade negativa/positiva värdeförändringar på säkringsinstrument;;;
;;395;Återvunna, tidigare avskrivna kundfordringar;;3950;Återvunna, tidigare avskrivna kundfordringar;;;
;■;396;Valutakursvinster på fordringar och skulder av rörelsekaraktär;■;3960;Valutakursvinster på fordringar och skulder av rörelsekaraktär;;;
;■;397;Vinst vid avyttring av immateriella och materiella anläggningstillgångar;■;3970;Vinst vid avyttring av immateriella och materiella anläggningstillgångar;;;
;;;;;3971;Vinst vid avyttring av immateriella anläggningstillgångar;;;
;;;;;3972;Vinst vid avyttring av byggnader och mark;;;
;;;;;3973;Vinst vid avyttring av maskiner och inventarier;;;
;■;398;Erhållna offentliga bidrag;■;3980;Erhållna offentliga bidrag;|;;
;;;;;3981;Erhållna EU-bidrag;;;
;;;;;3985;Erhållna statliga bidrag;|;;
;;;;;3987;Erhållna kommunala bidrag;;;
;;;;;3988;Erhållna offentliga bidrag för personal;|;;
;;;;;3989;Övriga erhållna offentliga bidrag;|;;
;;399;Övriga ersättningar, bidrag och intäkter;;3990;Övriga ersättningar, bidrag och intäkter;|;;
;;;;;3991;Konfliktersättning;;;
;;;;;3992;Erhållna skadestånd;;;
;;;;;3993;Erhållna donationer och gåvor;;;
;;;;;3994;Försäkringsersättningar;;;
;;;;;3995;Erhållet ackord på skulder av rörelsekaraktär;;;
;;;;;3996;Erhållna reklambidrag;;;
;;;;;3997;Sjuklöneersättning;|;;
;;;;;3998;Återbäring av överskott från försäkringsföretag;|;;
;;;;;3999;Övriga rörelseintäkter;;;
;;4;Utgifter/kostnader för varor, material och vissa köpta tjänster;;;;;;
;;40;Inköp av varor och material;;;;;;
;■;400;Inköp av varor från Sverige;■;4000;Inköp av varor från Sverige;;;
;;41;Inköp av varor och material;;;;;;
;;42;Inköp av varor och material;;;;;;
;■;420;Sålda varor VMB;■;4200;Sålda varor VMB;;;
;■;421;Sålda varor positiv VMB 25 %;■;4211;Sålda varor positiv VMB 25 %;;;
;;;;■;4212;Sålda varor negativ VMB 25 %;;;
;;43;Inköp av varor och material;;;;;;
;;44;Inköp av varor och material;;;;;;
;■;440;Momspliktiga inköp i Sverige;■;4400;Momspliktiga inköp i Sverige;;;
;■;441;Inköpta varor i Sverige, omvänd skattskyldighet, 25 % moms;■;4415;Inköpta varor i Sverige, omvänd skattskyldighet, 25 % moms;;;
;;;;;4416;Inköpta varor i Sverige, omvänd skattskyldighet, 12 % moms;;;
;;;;;4417;Inköpta varor i Sverige, omvänd skattskyldighet, 6 % moms;;;
;;442;Inköpta tjänster i Sverige, omvänd skattskyldighet, 25 % moms;;4425;Inköpta tjänster i Sverige, omvänd skattskyldighet, 25 % moms;;;
;;;;■;4426;Inköpta tjänster i Sverige, omvänd skattskyldighet, 12 %;;;
;;;;■;4427;Inköpta tjänster i Sverige, omvänd skattskyldighet, 6 %;;;
;;45;Inköp av varor och material;;;;;;
;■;450;Övriga momspliktiga inköp;■;4500;Övriga momspliktiga inköp;;;
;■;451;Inköp av varor från annat EU-land, 25 %;■;4515;Inköp av varor från annat EU-land, 25 %;;;
;;;;■;4516;Inköp av varor från annat EU-land, 12 %;;;
;;;;■;4517;Inköp av varor från annat EU-land, 6 %;;;
;;;;■;4518;Inköp av varor från annat EU-land, momsfri;;;
;■;453;Inköp av tjänster från ett land utanför EU, 25 % moms;■;4531;Inköp av tjänster från ett land utanför EU, 25 % moms;;;
;;;;■;4532;Inköp av tjänster från ett land utanför EU, 12 % moms;;;
;;;;■;4533;Inköp av tjänster från ett land utanför EU, 6 % moms;;;
;;;;■;4535;Inköp av tjänster från annat EU-land, 25 %;;;
;;;;■;4536;Inköp av tjänster från annat EU-land, 12 %;;;
;;;;■;4537;Inköp av tjänster från annat EU-land, 6 %;;;
;;;;■;4538;Inköp av tjänster från annat EU-land, momsfri;;;
;■;454;Import av varor, 25 % moms;■;4545;Import av varor, 25 % moms;;;
;;;;■;4546;Import av varor, 12 % moms;;;
;;;;■;4547;Import av varor, 6 % moms;;;
;;46;Legoarbeten, underentreprenader;;;;;;
;■;460;Legoarbeten och underentreprenader (gruppkonto);■;4600;Legoarbeten och underentreprenader (gruppkonto);;;
;;47;Reduktion av inköpspriser;;;;;;
;■;470;Reduktion av inköpspriser (gruppkonto);■;4700;Reduktion av inköpspriser (gruppkonto);;;
;;473;Erhållna rabatter;;4730;Erhållna rabatter;;;
;;;;;4731;Erhållna kassarabatter;;;
;;;;;4732;Erhållna mängdrabatter (inkl. bonus);;;
;;;;;4733;Erhållet aktivitetsstöd;;;
;;479;Övriga reduktioner av inköpspriser;;4790;Övriga reduktioner av inköpspriser;;;
;;48;(Fri kontogrupp);;;;;;
;;49;Förändring av lager, produkter i arbete och pågående arbeten;;;;;;
;■;490;Förändring av lager (gruppkonto);■;4900;Förändring av lager (gruppkonto);;;
;■;491;Förändring av lager av råvaror;■;4910;Förändring av lager av råvaror;;;
;■;492;Förändring av lager av tillsatsmaterial och förnödenheter;■;4920;Förändring av lager av tillsatsmaterial och förnödenheter;;;
;■;494;Förändring av produkter i arbete;■;4940;Förändring av produkter i arbete;;;
;;;;;4944;Förändring av produkter i arbete, material och utlägg;;;
;;;;;4945;Förändring av produkter i arbete, omkostnader;;;
;;;;;4947;Förändring av produkter i arbete, personalkostnader;;;
;■;495;Förändring av lager av färdiga varor;■;4950;Förändring av lager av färdiga varor;;;
;■;496;Förändring av lager av handelsvaror;■;4960;Förändring av lager av handelsvaror;;;
;■;497;Förändring av pågående arbeten, nedlagda kostnader;■;4970;Förändring av pågående arbeten, nedlagda kostnader;;;
;;;;;4974;Förändring av pågående arbeten, material och utlägg;;;
;;;;;4975;Förändring av pågående arbeten, omkostnader;;;
;;;;;4977;Förändring av pågående arbeten, personalkostnader;;;
;;498;Förändring av lager av värdepapper;;4980;Förändring av lager av värdepapper;;;
;;;;;4981;Sålda värdepappers anskaffningsvärde;;;
;;;;;4987;Nedskrivning av värdepapper;;;
;;;;;4988;Återföring av nedskrivning av värdepapper;;;
;;5;Övriga externa rörelseutgifter/ kostnader;;;;;;
;;50;Lokalkostnader;;;;;;
;;500;Lokalkostnader (gruppkonto);;5000;Lokalkostnader (gruppkonto);;;
;■;501;Lokalhyra;■;5010;Lokalhyra;;;
;;;;;5011;Hyra för kontorslokaler;;;
;;;;;5012;Hyra för garage;;;
;;;;;5013;Hyra för lagerlokaler;;;
;■;502;El för belysning;■;5020;El för belysning;;;
;■;503;Värme;■;5030;Värme;;;
;■;504;Vatten och avlopp;■;5040;Vatten och avlopp;;;
;;505;Lokaltillbehör;;5050;Lokaltillbehör;;;
;■;506;Städning och renhållning;■;5060;Städning och renhållning;;;
;;;;;5061;Städning;;;
;;;;;5062;Sophämtning;;;
;;;;;5063;Hyra för sopcontainer;;;
;;;;;5064;Snöröjning;;;
;;;;;5065;Trädgårdsskötsel;;;
;■;507;Reparation och underhåll av lokaler;■;5070;Reparation och underhåll av lokaler;;;
;;509;Övriga lokalkostnader;;5090;Övriga lokalkostnader;;;
;;;;;5098;Övriga lokalkostnader, avdragsgilla;;;
;;;;;5099;Övriga lokalkostnader, ej avdragsgilla;;;
;;51;Fastighetskostnader;;;;;;
;;510;Fastighetskostnader (gruppkonto);;5100;Fastighetskostnader (gruppkonto);;;
;;511;Tomträttsavgäld/arrende;;5110;Tomträttsavgäld/arrende;;;
;■;512;El för belysning;■;5120;El för belysning;;;
;■;513;Värme;■;5130;Värme;;;
;;;;;5131;Uppvärmning;;;
;;;;;5132;Sotning;;;
;■;514;Vatten och avlopp;■;5140;Vatten och avlopp;;;
;■;516;Städning och renhållning;■;5160;Städning och renhållning;;;
;;;;;5161;Städning;;;
;;;;;5162;Sophämtning;;;
;;;;;5163;Hyra för sopcontainer;;;
;;;;;5164;Snöröjning;;;
;;;;;5165;Trädgårdsskötsel;;;
;■;517;Reparation och underhåll av fastighet;■;5170;Reparation och underhåll av fastighet;;;
;;519;Övriga fastighetskostnader;;5190;Övriga fastighetskostnader;;;
;;;;;5191;Fastighetsskatt/fastighetsavgift;;;
;;;;;5192;Fastighetsförsäkringspremier;;;
;;;;;5193;Fastighetsskötsel och förvaltning;;;
;;;;;5198;Övriga fastighetskostnader, avdragsgilla;;;
;;;;;5199;Övriga fastighetskostnader, ej avdragsgilla;;;
;;52;Hyra av anläggningstillgångar;;;;;;
;■;520;Hyra av anläggningstillgångar (gruppkonto);■;5200;Hyra av anläggningstillgångar (gruppkonto);;;
;;521;Hyra av maskiner och andra tekniska anläggningar;;5210;Hyra av maskiner och andra tekniska anläggningar;;;
;;;;;5211;Korttidshyra av maskiner och andra tekniska anläggningar;;;
;;;;;5212;Leasing av maskiner och andra tekniska anläggningar;;;
;;522;Hyra av inventarier och verktyg;;5220;Hyra av inventarier och verktyg;;;
;;;;;5221;Korttidshyra av inventarier och verktyg;;;
;;;;;5222;Leasing av inventarier och verktyg;;;
;;525;Hyra av datorer;;5250;Hyra av datorer;;;
;;;;;5251;Korttidshyra av datorer;;;
;;;;;5252;Leasing av datorer;;;
;;529;Övriga hyreskostnader för anläggningstillgångar;;5290;Övriga hyreskostnader för anläggningstillgångar;;;
;;53;Energikostnader;;;;;;
;■;530;Energikostnader (gruppkonto);■;5300;Energikostnader (gruppkonto);;;
;;531;El för drift;;5310;El för drift;;;
;;532;Gas;;5320;Gas;;;
;;533;Eldningsolja;;5330;Eldningsolja;;;
;;534;Stenkol och koks;;5340;Stenkol och koks;;;
;;535;Torv, träkol, ved och annat träbränsle;;5350;Torv, träkol, ved och annat träbränsle;;;
;;536;Bensin, fotogen och motorbrännolja;;5360;Bensin, fotogen och motorbrännolja;;;
;;537;Fjärrvärme, kyla och ånga;;5370;Fjärrvärme, kyla och ånga;;;
;;538;Vatten;;5380;Vatten;;;
;;539;Övriga energikostnader;;5390;Övriga energikostnader;;;
;;54;Förbrukningsinventarier och förbrukningsmaterial;;;;;;
;;540;Förbrukningsinventarier och förbrukningsmaterial (gruppkonto);;5400;Förbrukningsinventarier och förbrukningsmaterial (gruppkonto);;;
;■;541;Förbrukningsinventarier;■;5410;Förbrukningsinventarier;;;
;;;;;5411;Förbrukningsinventarier med en livslängd på mer än ett år;;;
;;;;;5412;Förbrukningsinventarier med en livslängd på ett år eller mindre;;;
;■;542;Programvaror;■;5420;Programvaror;;;
;;543;Transportinventarier;;5430;Transportinventarier;;;
;;544;Förbrukningsemballage;;5440;Förbrukningsemballage;;;
;■;546;Förbrukningsmaterial;■;5460;Förbrukningsmaterial;;;
;;548;Arbetskläder och skyddsmaterial;;5480;Arbetskläder och skyddsmaterial;;;
;;549;Övriga förbrukningsinventarier och förbrukningsmaterial;;5490;Övriga förbrukningsinventarier och förbrukningsmaterial;;;
;;;;;5491;Övriga förbrukningsinventarier med en livslängd på mer än ett år;;;
;;;;;5492;Övriga förbrukningsinventarier med en livslängd på ett år eller mindre;;;
;;;;;5493;Övrigt förbrukningsmaterial;;;
;;55;Reparation och underhåll;;;;;;
;■;550;Reparation och underhåll (gruppkonto);■;5500;Reparation och underhåll (gruppkonto);;;
;;551;Reparation och underhåll av maskiner och andra tekniska anläggningar;;5510;Reparation och underhåll av maskiner och andra tekniska anläggningar;;;
;;552;Reparation och underhåll av inventarier, verktyg och datorer m.m.;;5520;Reparation och underhåll av inventarier, verktyg och datorer m.m.;;;
;;553;Reparation och underhåll av installationer;;5530;Reparation och underhåll av installationer;;;
;;555;Reparation och underhåll av förbrukningsinventarier;;5550;Reparation och underhåll av förbrukningsinventarier;;;
;;558;Underhåll och tvätt av arbetskläder;;5580;Underhåll och tvätt av arbetskläder;;;
;;559;Övriga kostnader för reparation och underhåll;;5590;Övriga kostnader för reparation och underhåll;;;
;;56;Kostnader för transportmedel;;;;;;
;■;560;Kostnader för transportmedel (gruppkonto);■;5600;Kostnader för transportmedel (gruppkonto);;;
;;561;Personbilskostnader;;5610;Personbilskostnader;;;
;;;;■;5611;Drivmedel för personbilar;;;
;;;;■;5612;Försäkring och skatt för personbilar;;;
;;;;■;5613;Reparation och underhåll av personbilar;;;
;;;;■;5615;Leasing av personbilar;;;
;;;;;5616;Trängselskatt, avdragsgill;;;
;;;;;5619;Övriga personbilskostnader;;;
;;562;Lastbilskostnader;;5620;Lastbilskostnader;;;
;;563;Truckkostnader;;5630;Truckkostnader;;;
;;564;Kostnader för arbetsmaskiner;;5640;Kostnader för arbetsmaskiner;;;
;;565;Traktorkostnader;;5650;Traktorkostnader;;;
;;566;Motorcykel-, moped- och skoterkostnader;;5660;Motorcykel-, moped- och skoterkostnader;;;
;;567;Båt-, flygplans- och helikopterkostnader;;5670;Båt-, flygplans- och helikopterkostnader;;;
;;569;Övriga kostnader för transportmedel;;5690;Övriga kostnader för transportmedel;;;
;;57;Frakter och transporter;;;;;;
;■;570;Frakter och transporter (gruppkonto);■;5700;Frakter och transporter (gruppkonto);;;
;;571;Frakter, transporter och försäkringar vid varudistribution;;5710;Frakter, transporter och försäkringar vid varudistribution;;;
;;572;Tull- och speditionskostnader m.m.;;5720;Tull- och speditionskostnader m.m.;;;
;;573;Arbetstransporter;;5730;Arbetstransporter;;;
;;579;Övriga kostnader för frakter och transporter;;5790;Övriga kostnader för frakter och transporter;;;
;;58;Resekostnader;;;;;;
;■;580;Resekostnader (gruppkonto);■;5800;Resekostnader (gruppkonto);;;
;■;581;Biljetter;■;5810;Biljetter;;;
;■;582;Hyrbilskostnader;■;5820;Hyrbilskostnader;;;
;;583;Kost och logi;;5830;Kost och logi;;;
;;;;■;5831;Kost och logi i Sverige;;;
;;;;■;5832;Kost och logi i utlandet;;;
;;589;Övriga resekostnader;;5890;Övriga resekostnader;;;
;;59;Reklam och PR;;;;;;
;■;590;Reklam och PR (gruppkonto);■;5900;Reklam och PR (gruppkonto);;;
;;591;Annonsering;;5910;Annonsering;;;
;;592;Utomhus- och trafikreklam;;5920;Utomhus- och trafikreklam;;;
;;593;Reklamtrycksaker och direktreklam;;5930;Reklamtrycksaker och direktreklam;;;
;;594;Utställningar och mässor;;5940;Utställningar och mässor;;;
;;595;Butiksreklam och återförsäljarreklam;;5950;Butiksreklam och återförsäljarreklam;;;
;;596;Varuprover, reklamgåvor, presentreklam och tävlingar;;5960;Varuprover, reklamgåvor, presentreklam och tävlingar;;;
;;597;Film-, radio-, TV- och Internetreklam;;5970;Film-, radio-, TV- och Internetreklam;;;
;;598;PR, institutionell reklam och sponsring;;5980;PR, institutionell reklam och sponsring;;;
;;599;Övriga kostnader för reklam och PR;;5990;Övriga kostnader för reklam och PR;;;
;;6;Övriga externa rörelseutgifter/ kostnader;;;;;;
;;60;Övriga försäljningskostnader;;;;;;
;;600;Övriga försäljningskostnader (gruppkonto);;6000;Övriga försäljningskostnader (gruppkonto);;;
;;601;Kataloger, prislistor m.m.;;6010;Kataloger, prislistor m.m.;;;
;;602;Egna facktidskrifter;;6020;Egna facktidskrifter;;;
;;603;Speciella orderkostnader;;6030;Speciella orderkostnader;;;
;;604;Kontokortsavgifter;;6040;Kontokortsavgifter;;;
;;605;Försäljningsprovisioner;;6050;Försäljningsprovisioner;;;
;;;;;6055;Franchisekostnader o.dyl.;;;
;;606;Kreditförsäljningskostnader;;6060;Kreditförsäljningskostnader;;;
;;;;;6061;Kreditupplysning;;;
;;;;;6062;Inkasso och KFM-avgifter;;;
;;;;;6063;Kreditförsäkringspremier;;;
;;;;;6064;Factoringavgifter;;;
;;;;;6069;Övriga kreditförsäljningskostnader;;;
;;607;Representation;;6070;Representation;;;
;;;;■;6071;Representation, avdragsgill;;;
;;;;■;6072;Representation, ej avdragsgill;;;
;;608;Bankgarantier;;6080;Bankgarantier;;;
;■;609;Övriga försäljningskostnader;■;6090;Övriga försäljningskostnader;;;
;;61;Kontorsmateriel och trycksaker;;;;;;
;■;610;Kontorsmateriel och trycksaker (gruppkonto);■;6100;Kontorsmateriel och trycksaker (gruppkonto);;;
;;611;Kontorsmateriel;;6110;Kontorsmateriel;;;
;;615;Trycksaker;;6150;Trycksaker;;;
;;62;Tele och post;;;;;;
;;620;Tele och post (gruppkonto);;6200;Tele och post (gruppkonto);;;
;■;621;Telekommunikation;■;6210;Telekommunikation;;;
;;;;;6211;Fast telefoni;;;
;;;;;6212;Mobiltelefon;;;
;;;;;6213;Mobilsökning;;;
;;;;;6214;Fax;;;
;;;;;6215;Telex;;;
;;623;Datakommunikation;;6230;Datakommunikation;;;
;■;625;Postbefordran;■;6250;Postbefordran;;;
;;63;Företagsförsäkringar och övriga riskkostnader;;;;;;
;;630;Företagsförsäkringar och övriga riskkostnader (gruppkonto);;6300;Företagsförsäkringar och övriga riskkostnader (gruppkonto);;;
;■;631;Företagsförsäkringar;■;6310;Företagsförsäkringar;;;
;;632;Självrisker vid skada;;6320;Självrisker vid skada;;;
;;633;Förluster i pågående arbeten;;6330;Förluster i pågående arbeten;;;
;;634;Lämnade skadestånd;;6340;Lämnade skadestånd;;;
;;;;;6341;Lämnade skadestånd, avdragsgilla;;;
;;;;;6342;Lämnade skadestånd, ej avdragsgilla;;;
;■;635;Förluster på kundfordringar;■;6350;Förluster på kundfordringar;;;
;;;;;6351;Konstaterade förluster på kundfordringar;;;
;;;;;6352;Befarade förluster på kundfordringar;;;
;;636;Garantikostnader;;6360;Garantikostnader;;;
;;;;;6361;Förändring av garantiavsättning;;;
;;;;;6362;Faktiska garantikostnader;;;
;;637;Kostnader för bevakning och larm;;6370;Kostnader för bevakning och larm;;;
;;638;Förluster på övriga kortfristiga fordringar;;6380;Förluster på övriga kortfristiga fordringar;;;
;■;639;Övriga riskkostnader;■;6390;Övriga riskkostnader;;;
;;64;Förvaltningskostnader;;;;;;
;;640;Förvaltningskostnader (gruppkonto);;6400;Förvaltningskostnader (gruppkonto);;;
;■;641;Styrelsearvoden som inte är lön;■;6410;Styrelsearvoden som inte är lön;;;
;■;642;Ersättningar till revisor;■;6420;Ersättningar till revisor;;;
;;;;;6421;Revision;;;
;;;;;6422;Revisonsverksamhet utöver revision;;;
;;;;;6423;Skatterådgivning – revisor;;;
;;;;;6424;Övriga tjänster – revisor;;;
;;643;Management fees;;6430;Management fees;;;
;;644;Årsredovisning och delårsrapporter;;6440;Årsredovisning och delårsrapporter;;;
;;645;Bolagsstämma/års- eller föreningsstämma;;6450;Bolagsstämma/års- eller föreningsstämma;;;
;;649;Övriga förvaltningskostnader;;6490;Övriga förvaltningskostnader;;;
;;65;Övriga externa tjänster;;;;;;
;;650;Övriga externa tjänster (gruppkonto);;6500;Övriga externa tjänster (gruppkonto);;;
;;651;Mätningskostnader;;6510;Mätningskostnader;;;
;;652;Ritnings- och kopieringskostnader;;6520;Ritnings- och kopieringskostnader;;;
;■;653;Redovisningstjänster;■;6530;Redovisningstjänster;;;
;■;654;IT-tjänster;■;6540;IT-tjänster;;;
;■;655;Konsultarvoden;■;6550;Konsultarvoden;;;
;;;;;6551;Arkitekttjänster;|;;
;;;;;6552;Teknisk provning och analys;|;;
;;;;;6553;Tekniska konsulttjänster;|;;
;;;;;6554;Finansiell- och övrig ekonomisk rådgivning;|;;
;;;;;6555;Skatterådgivning inkl. insolvens- och konkursförvaltning;|;;
;;;;;6556;Köpta tjänster avseende forskning och utveckling;|;;
;;;;;6559;Övrig konsultverksamhet;|;;
;■;656;Serviceavgifter till branschorganisationer;■;6560;Serviceavgifter till branschorganisationer;;;
;■;657;Bankkostnader;■;6570;Bankkostnader;;;
;;658;Advokat- och rättegångskostnader;;6580;Advokat- och rättegångskostnader;;;
;■;659;Övriga externa tjänster;■;6590;Övriga externa tjänster;;;
;;66;(Fri kontogrupp);;;;;;
;;67;(Fri kontogrupp);;;;;;
;;68;Inhyrd personal;;;;;;
;■;680;Inhyrd personal (gruppkonto);■;6800;Inhyrd personal (gruppkonto);;;
;;681;Inhyrd produktionspersonal;;6810;Inhyrd produktionspersonal;;;
;;682;Inhyrd lagerpersonal;;6820;Inhyrd lagerpersonal;;;
;;683;Inhyrd transportpersonal;;6830;Inhyrd transportpersonal;;;
;;684;Inhyrd kontors- och ekonomipersonal;;6840;Inhyrd kontors- och ekonomipersonal;;;
;;685;Inhyrd IT-personal;;6850;Inhyrd IT-personal;;;
;;686;Inhyrd marknads- och försäljningspersonal;;6860;Inhyrd marknads- och försäljningspersonal;;;
;;687;Inhyrd restaurang- och butikspersonal;;6870;Inhyrd restaurang- och butikspersonal;;;
;;688;Inhyrda företagsledare;;6880;Inhyrda företagsledare;;;
;;689;Övrig inhyrd personal;;6890;Övrig inhyrd personal;;;
;;69;Övriga externa kostnader;;;;;;
;;690;Övriga externa kostnader (gruppkonto);;6900;Övriga externa kostnader (gruppkonto);;;
;;691;Licensavgifter och royalties;;6910;Licensavgifter och royalties;;;
;;692;Kostnader för egna patent;;6920;Kostnader för egna patent;;;
;;693;Kostnader för varumärken m.m.;;6930;Kostnader för varumärken m.m.;;;
;;694;Kontroll-, provnings- och stämpelavgifter;;6940;Kontroll-, provnings- och stämpelavgifter;;;
;;695;Tillsynsavgifter myndigheter;;6950;Tillsynsavgifter myndigheter;;;
;■;697;Tidningar, tidskrifter och facklitteratur;■;6970;Tidningar, tidskrifter och facklitteratur;;;
;■;698;Föreningsavgifter;■;6980;Föreningsavgifter;;;
;;;;;6981;Föreningsavgifter, avdragsgilla;;;
;;;;;6982;Föreningsavgifter, ej avdragsgilla;;;
;;699;Övriga externa kostnader;;6990;Övriga externa kostnader;;;
;;;;■;6991;Övriga externa kostnader, avdragsgilla;;;
;;;;■;6992;Övriga externa kostnader, ej avdragsgilla;;;
;;;;;6993;Lämnade bidrag och gåvor;;;
;;;;;6996;Betald utländsk inkomstskatt;;;
;;;;;6997;Obetald utländsk inkomstskatt;;;
;;;;;6998;Utländsk moms;;;
;;;;;6999;Ingående moms, blandad verksamhet;;;
;;7;Utgifter/kostnader för personal, avskrivningar m.m.;;;;;;
;;70;Löner till kollektivanställda;;;;;;
;;700;Löner till kollektivanställda (gruppkonto);;7000;Löner till kollektivanställda (gruppkonto);;;
;■;701;Löner till kollektivanställda;■;7010;Löner till kollektivanställda;;;
;;;;;7011;Löner till kollektivanställda;;;
;;;;;7012;Vinstandelar till kollektivanställda;;;
;;;;;7013;Lön växa-stöd kollektivanställda 10,21 %;;;
;;;;;7017;Avgångsvederlag till kollektivanställda;;;
;;;;;7018;Bruttolöneavdrag, kollektivanställda;;;
;;;;;7019;Upplupna löner och vinstandelar till kollektivanställda;;;
;;703;Löner till kollektivanställda (utlandsanställda);;7030;Löner till kollektivanställda (utlandsanställda);;;
;;;;;7031;Löner till kollektivanställda (utlandsanställda);;;
;;;;;7032;Vinstandelar till kollektivanställda (utlandsanställda);;;
;;;;;7037;Avgångsvederlag till kollektivanställda (utlandsanställda);;;
;;;;;7038;Bruttolöneavdrag, kollektivanställda (utlandsanställda);;;
;;;;;7039;Upplupna löner och vinstandelar till kollektivanställda (utlandsanställda);;;
;;708;Löner till kollektivanställda för ej arbetad tid;;7080;Löner till kollektivanställda för ej arbetad tid;;;
;;;;;7081;Sjuklöner till kollektivanställda;;;
;;;;;7082;Semesterlöner till kollektivanställda;;;
;;;;;7083;Föräldraersättning till kollektivanställda;;;
;;;;;7089;Övriga löner till kollektivanställda för ej arbetad tid;;;
;■;709;Förändring av semesterlöneskuld;■;7090;Förändring av semesterlöneskuld;;;
;;71;(Fri kontogrupp);;;;;;
;;72;Löner till tjänstemän och företagsledare;;;;;;
;;720;Löner till tjänstemän och företagsledare (gruppkonto);;7200;Löner till tjänstemän och företagsledare (gruppkonto);;;
;■;721;Löner till tjänstemän;■;7210;Löner till tjänstemän;;;
;;;;;7211;Löner till tjänstemän;;;
;;;;;7212;Vinstandelar till tjänstemän;;;
;;;;;7213;Lön växa-stöd tjänstemän 10,21 %;;;
;;;;;7217;Avgångsvederlag till tjänstemän;;;
;;;;;7218;Bruttolöneavdrag, tjänstemän;;;
;;;;;7219;Upplupna löner och vinstandelar till tjänstemän;;;
;■;722;Löner till företagsledare;■;7220;Löner till företagsledare;;;
;;;;;7221;Löner till företagsledare;;;
;;;;;7222;Tantiem till företagsledare;;;
;;;;;7227;Avgångsvederlag till företagsledare;;;
;;;;;7228;Bruttolöneavdrag, företagsledare;;;
;;;;;7229;Upplupna löner och tantiem till företagsledare;;;
;;723;Löner till tjänstemän och ftgsledare (utlandsanställda);;7230;Löner till tjänstemän och ftgsledare (utlandsanställda);;;
;;;;;7231;Löner till tjänstemän och ftgsledare (utlandsanställda);;;
;;;;;7232;Vinstandelar till tjänstemän och ftgsledare (utlandsanställda);;;
;;;;;7237;Avgångsvederlag till tjänstemän och ftgsledare (utlandsanställda);;;
;;;;;7238;Bruttolöneavdrag, tjänstemän och ftgsledare (utlandsanställda);;;
;;;;;7239;Upplupna löner och vinstandelar till tjänstemän och ftgsledare (utlandsanställda);;;
;■;724;Styrelsearvoden;■;7240;Styrelsearvoden;;;
;;728;Löner till tjänstemän och företagsledare för ej arbetad tid;;7280;Löner till tjänstemän och företagsledare för ej arbetad tid;;;
;;;;;7281;Sjuklöner till tjänstemän;;;
;;;;;7282;Sjuklöner till företagsledare;;;
;;;;;7283;Föräldraersättning till tjänstemän;;;
;;;;;7284;Föräldraersättning till företagsledare;;;
;;;;;7285;Semesterlöner till tjänstemän;;;
;;;;;7286;Semesterlöner till företagsledare;;;
;;;;;7288;Övriga löner till tjänstemän för ej arbetad tid;;;
;;;;;7289;Övriga löner till företagsledare för ej arbetad tid;;;
;■;729;Förändring av semesterlöneskuld;■;7290;Förändring av semesterlöneskuld;;;
;;;;;7291;Förändring av semesterlöneskuld till tjänstemän;;;
;;;;;7292;Förändring av semesterlöneskuld till företagsledare;;;
;;73;Kostnadsersättningar och förmåner;;;;;;
;;730;Kostnadsersättningar och förmåner (gruppkonto);;7300;Kostnadsersättningar och förmåner (gruppkonto);;;
;■;731;Kontanta extraersättningar;■;7310;Kontanta extraersättningar;;;
;;;;;7311;Ersättningar för sammanträden m.m.;;;
;;;;;7312;Ersättningar för förslagsverksamhet och uppfinningar;;;
;;;;;7313;Ersättningar för/bidrag till bostadskostnader;;;
;;;;;7314;Ersättningar för/bidrag till måltidskostnader;;;
;;;;;7315;Ersättningar för/bidrag till resor till och från arbetsplatsen;;;
;;;;;7316;Ersättningar för/bidrag till arbetskläder;;;
;;;;;7317;Ersättningar för/bidrag till arbetsmaterial och arbetsverktyg;;;
;;;;;7318;Felräkningspengar;;;
;;;;;7319;Övriga kontanta extraersättningar;;;
;;732;Traktamenten vid tjänsteresa;;7320;Traktamenten vid tjänsteresa;;;
;;;;■;7321;Skattefria traktamenten, Sverige;;;
;;;;■;7322;Skattepliktiga traktamenten, Sverige;;;
;;;;■;7323;Skattefria traktamenten, utlandet;;;
;;;;■;7324;Skattepliktiga traktamenten, utlandet;;;
;;733;Bilersättningar;;7330;Bilersättningar;;;
;;;;■;7331;Skattefria bilersättningar;;;
;;;;■;7332;Skattepliktiga bilersättningar;;;
;;;;;7333;Ersättning för trängselskatt, skattefri;;;
;;735;Ersättningar för föreskrivna arbetskläder;;7350;Ersättningar för föreskrivna arbetskläder;;;
;;737;Representationsersättningar;;7370;Representationsersättningar;;;
;■;738;Kostnader för förmåner till anställda;■;7380;Kostnader för förmåner till anställda;;;
;;;;;7381;Kostnader för fri bostad;;;
;;;;;7382;Kostnader för fria eller subventionerade måltider;;;
;;;;;7383;Kostnader för fria resor till och från arbetsplatsen;;;
;;;;;7384;Kostnader för fria eller subventionerade arbetskläder;;;
;;;;■;7385;Kostnader för fri bil;;;
;;;;;7386;Subventionerad ränta;;;
;;;;;7387;Kostnader för lånedatorer;;;
;;;;;7388;Anställdas ersättning för erhållna förmåner;;;
;;;;;7389;Övriga kostnader för förmåner;;;
;■;739;Övriga kostnadsersättningar och förmåner;■;7390;Övriga kostnadsersättningar och förmåner;;;
;;;;;7391;Kostnad för trängselskatteförmån;;;
;;;;;7392;Kostnad för förmån av hushållsnära tjänster;;;
;;74;Pensionskostnader;;;;;;
;;740;Pensionskostnader (gruppkonto);;7400;Pensionskostnader (gruppkonto);;;
;■;741;Pensionsförsäkringspremier;■;7410;Pensionsförsäkringspremier;;;
;;;;;7411;Premier för kollektiva pensionsförsäkringar;;;
;;;;;7412;Premier för individuella pensionsförsäkringar;;;
;;742;Förändring av pensionsskuld;;7420;Förändring av pensionsskuld;;;
;;743;Avdrag för räntedel i pensionskostnad;;7430;Avdrag för räntedel i pensionskostnad;;;
;;744;Förändring av pensionsstiftelsekapital;;7440;Förändring av pensionsstiftelsekapital;;;
;;;;;7441;Överföring av medel till pensionsstiftelse;|;;
;;;;;7448;Gottgörelse från pensionsstiftelse;;;
;;746;Pensionsutbetalningar;;7460;Pensionsutbetalningar;;;
;;;;;7461;Pensionsutbetalningar till f.d. kollektivanställda;;;
;;;;;7462;Pensionsutbetalningar till f.d. tjänstemän;;;
;;;;;7463;Pensionsutbetalningar till f.d. företagsledare;;;
;;747;Förvaltnings- och kreditförsäkringsavgifter;;7470;Förvaltnings- och kreditförsäkringsavgifter;;;
;■;749;Övriga pensionskostnader;■;7490;Övriga pensionskostnader;;;
;;75;Sociala och andra avgifter enligt lag och avtal;;;;;;
;;750;Sociala och andra avgifter enligt lag och avtal (gruppkonto);;7500;Sociala och andra avgifter enligt lag och avtal (gruppkonto);;;
;;751;Arbetsgivaravgifter 31,42 %;;7510;Arbetsgivaravgifter 31,42 %;;;
;;;;■;7511;Arbetsgivaravgifter för löner och ersättningar;;;
;;;;■;7512;Arbetsgivaravgifter för förmånsvärden;;;
;;;;;7515;Arbetsgivaravgifter på skattepliktiga kostnadsersättningar;;;
;;;;;7516;Arbetsgivaravgifter på arvoden;;;
;;;;;7518;Arbetsgivaravgifter på bruttolöneavdrag m.m.;;;
;;;;■;7519;Arbetsgivaravgifter för semester- och löneskulder;;;
;■;753;Särskild löneskatt;■;7530;Särskild löneskatt;;;
;;;;;7531;Särskild löneskatt för vissa försäkringsersättningar m.m.;;;
;;;;;7532;Särskild löneskatt pensionskostnader, deklarationspost;;;
;;;;;7533;Särskild löneskatt för pensionskostnader;;;
;■;755;Avkastningsskatt på pensionsmedel;■;7550;Avkastningsskatt på pensionsmedel;;;
;;;;;7551;Avkastningsskatt 15 % försäkringsföretag m.fl. samt avsatt till pensioner;;;
;;;;;7552;Avkastningsskatt 15 % utländska pensionsförsäkringar;;;
;;;;;7553;Avkastningsskatt 30 % utländska försäkringsföretag m.fl.;;;
;;;;;7554;Avkastningsskatt 30 % utländska kapitalförsäkringar;;;
;■;757;Premier för arbetsmarknadsförsäkringar;■;7570;Premier för arbetsmarknadsförsäkringar;;;
;;;;;7571;Arbetsmarknadsförsäkringar;;;
;;;;;7572;Arbetsmarknadsförsäkringar pensionsförsäkringspremier, deklarationspost;;;
;■;758;Gruppförsäkringspremier;■;7580;Gruppförsäkringspremier;;;
;;;;;7581;Grupplivförsäkringspremier;;;
;;;;;7582;Gruppsjukförsäkringspremier;;;
;;;;;7583;Gruppolycksfallsförsäkringspremier;;;
;;;;;7589;Övriga gruppförsäkringspremier;;;
;■;759;Övriga sociala och andra avgifter enligt lag och avtal;■;7590;Övriga sociala och andra avgifter enligt lag och avtal;;;
;;76;Övriga personalkostnader;;;;;;
;■;760;Övriga personalkostnader (gruppkonto);■;7600;Övriga personalkostnader (gruppkonto);;;
;■;761;Utbildning;■;7610;Utbildning;;;
;;762;Sjuk- och hälsovård;;7620;Sjuk- och hälsovård;;;
;;;;■;7621;Sjuk- och hälsovård, avdragsgill;;;
;;;;■;7622;Sjuk- och hälsovård, ej avdragsgill;;;
;;;;;7623;Sjukvårdsförsäkring, ej avdragsgill;;;
;;763;Personalrepresentation;;7630;Personalrepresentation;;;
;;;;■;7631;Personalrepresentation, avdragsgill;;;
;;;;■;7632;Personalrepresentation, ej avdragsgill;;;
;;765;Sjuklöneförsäkring;;7650;Sjuklöneförsäkring;;;
;;767;Förändring av personalstiftelsekapital;;7670;Förändring av personalstiftelsekapital;;;
;;;;;7671;Avsättning till personalstiftelse;;;
;;;;;7678;Gottgörelse från personalstiftelse;;;
;;769;Övriga personalkostnader;;7690;Övriga personalkostnader;;;
;;;;;7691;Personalrekrytering;;;
;;;;;7692;Begravningshjälp;;;
;;;;;7693;Fritidsverksamhet;;;
;;;;;7699;Övriga personalkostnader;;;
;;77;Nedskrivningar och återföring av nedskrivningar;;;;;;
;;771;Nedskrivningar av immateriella anläggningstillgångar;;7710;Nedskrivningar av immateriella anläggningstillgångar;;;
;■;772;Nedskrivningar av byggnader och mark;■;7720;Nedskrivningar av byggnader och mark;;;
;■;773;Nedskrivningar av maskiner och inventarier;■;7730;Nedskrivningar av maskiner och inventarier;;;
;;774;Nedskrivningar av vissa omsättningstillgångar;;7740;Nedskrivningar av vissa omsättningstillgångar;;;
;;776;Återföring av nedskrivningar av immateriella anläggningstillgångar;;7760;Återföring av nedskrivningar av immateriella anläggningstillgångar;;;
;;777;Återföring av nedskrivningar av byggnader och mark;;7770;Återföring av nedskrivningar av byggnader och mark;;;
;;778;Återföring av nedskrivningar av maskiner och inventarier;;7780;Återföring av nedskrivningar av maskiner och inventarier;;;
;;779;Återföring av nedskrivningar av vissa omsättningstillgångar;;7790;Återföring av nedskrivningar av vissa omsättningstillgångar;;;
;;78;Avskrivningar enligt plan;;;;;;
;■;781;Avskrivningar på immateriella anläggningstillgångar;■;7810;Avskrivningar på immateriella anläggningstillgångar;;;
;;;;;7811;Avskrivningar på balanserade utgifter;;;
;;;;;7812;Avskrivningar på koncessioner m.m.;;;
;;;;;7813;Avskrivningar på patent;;;
;;;;;7814;Avskrivningar på licenser;;;
;;;;;7815;Avskrivningar på varumärken;;;
;;;;;7816;Avskrivningar på hyresrätter;;;
;;;;;7817;Avskrivningar på goodwill;;;
;;;;;7819;Avskrivningar på övriga immateriella anläggningstillgångar;;;
;■;782;Avskrivningar på byggnader och markanläggningar;■;7820;Avskrivningar på byggnader och markanläggningar;;;
;;;;;7821;Avskrivningar på byggnader;;;
;;;;;7824;Avskrivningar på markanläggningar;;;
;;;;;7829;Avskrivningar på övriga byggnader;;;
;■;783;Avskrivningar på maskiner och inventarier;■;7830;Avskrivningar på maskiner och inventarier;;;
;;;;;7831;Avskrivningar på maskiner och andra tekniska anläggningar;;;
;;;;;7832;Avskrivningar på inventarier och verktyg;;;
;;;;;7833;Avskrivningar på installationer;;;
;;;;;7834;Avskrivningar på bilar och andra transportmedel;;;
;;;;;7835;Avskrivningar på datorer;;;
;;;;;7836;Avskrivningar på leasade tillgångar;;;
;;;;;7839;Avskrivningar på övriga maskiner och inventarier;;;
;;784;Avskrivningar på förbättringsutgifter på annans fastighet;;7840;Avskrivningar på förbättringsutgifter på annans fastighet;;;
;;79;Övriga rörelsekostnader;;;;;;
;[Ej K2];794;Orealiserade positiva/negativa värdeförändringar på säkringsinstrument;[Ej K2];7940;Orealiserade positiva/negativa värdeförändringar på säkringsinstrument;;;
;;796;Valutakursförluster på fordringar och skulder av rörelsekaraktär;;7960;Valutakursförluster på fordringar och skulder av rörelsekaraktär;;;
;■;797;Förlust vid avyttring av immateriella och materiella anläggningstillgångar;■;7970;Förlust vid avyttring av immateriella och materiella anläggningstillgångar;;;
;;;;;7971;Förlust vid avyttring av immateriella anläggningstillgångar;;;
;;;;;7972;Förlust vid avyttring av byggnader och mark;;;
;;;;;7973;Förlust vid avyttring av maskiner och inventarier;;;
;■;799;Övriga rörelsekostnader;■;7990;Övriga rörelsekostnader;;;
;;8;Finansiella och andra inkomster/ intäkter och utgifter/kostnader;;;;;;
;;80;Resultat från andelar i koncernföretag;;;;;;
;;801;Utdelning på andelar i koncernföretag;;8010;Utdelning på andelar i koncernföretag;;;
;;;;;8012;Utdelning på andelar i dotterföretag;;;
;;;;;8016;Emissionsinsats, koncernföretag;;;
;;802;Resultat vid försäljning av andelar i koncernföretag;;8020;Resultat vid försäljning av andelar i koncernföretag;;;
;;;;;8022;Resultat vid försäljning av andelar i dotterföretag;;;
;;803;Resultatandelar från handelsbolag (dotterföretag);;8030;Resultatandelar från handelsbolag (dotterföretag);;;
;;807;Nedskrivningar av andelar i och långfristiga fordringar hos koncernföretag;;8070;Nedskrivningar av andelar i och långfristiga fordringar hos koncernföretag;;;
;;;;;8072;Nedskrivningar av andelar i dotterföretag;;;
;;;;;8076;Nedskrivningar av långfristiga fordringar hos moderföretag;;;
;;;;;8077;Nedskrivningar av långfristiga fordringar hos dotterföretag;;;
;;808;Återföringar av nedskrivningar av andelar i och långfristiga fordringar hos koncernföretag;;8080;Återföringar av nedskrivningar av andelar i och långfristiga fordringar hos koncernföretag;;;
;;;;;8082;Återföringar av nedskrivningar av andelar i dotterföretag;;;
;;;;;8086;Återföringar av nedskrivningar av långfristiga fordringar hos moderföretag;;;
;;;;;8087;Återföringar av nedskrivningar av långfristiga fordringar hos dotterföretag;;;
;;81;Resultat från andelar i intresseföretag;;;;;;
;;811;Utdelningar på andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;8110;Utdelningar på andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;8111;Utdelningar på andelar i intresseföretag;;;
;;;;;8112;Utdelningar på andelar i gemensamt styrda företag;;;
;;;;;8113;Utdelningar på andelar i övriga företag som det finns ett ägarintresse i;;;
;;;;;8116;Emissionsinsats, intresseföretag;;;
;;;;;8117;Emissionsinsats, gemensamt styrda företag;;;
;;;;;8118;Emissionsinsats, övriga företag som det finns ett ägarintresse i;;;
;;812;Resultat vid försäljning av andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;8120;Resultat vid försäljning av andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;8121;Resultat vid försäljning av andelar i intresseföretag;;;
;;;;;8122;Resultat vid försäljning av andelar i gemensamt styrda företag;;;
;;;;;8123;Resultat vid försäljning av andelar i övriga företag som det finns ett ägarintresse i;;;
;;813;Resultatandelar från handelsbolag (intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i);;8130;Resultatandelar från handelsbolag (intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i);;;
;;;;;8131;Resultatandelar från handelsbolag (intresseföretag);;;
;;;;;8132;Resultatandelar från handelsbolag (gemensamt styrda företag);;;
;;;;;8133;Resultatandelar från handelsbolag (övriga företag som det finns ett ägarintresse i);;;
;;817;Nedskrivningar av andelar i och långfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;8170;Nedskrivningar av andelar i och långfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;8171;Nedskrivningar av andelar i intresseföretag;;;
;;;;;8172;Nedskrivningar av långfristiga fordringar hos intresseföretag;;;
;;;;;8173;Nedskrivningar av andelar i gemensamt styrda företag;;;
;;;;;8174;Nedskrivningar av långfristiga fordringar hos gemensamt styrda företag;;;
;;;;;8176;Nedskrivningar av andelar i övriga företag som det finns ett ägarintresse i;;;
;;;;;8177;Nedskrivningar av långfristiga fordringar hos övriga företag som det finns ett ägarintresse i;;;
;;818;Återföringar av nedskrivningar av andelar i och långfristiga fordringar hos intresseföretag;;8180;Återföringar av nedskrivningar av andelar i och långfristiga fordringar hos intresseföretag;;;
;;;;;8181;Återföringar av nedskrivningar av andelar i intresseföretag;;;
;;;;;8182;Återföringar av nedskrivningar av långfristiga fordringar hos intresseföretag;;;
;;;;;8183;Återföringar av nedskrivningar av andelar i gemensamt styrda företag;;;
;;;;;8184;Återföringar av nedskrivningar av långfristiga fordringar hos gemensamt styrda företag;;;
;;;;;8186;Återföringar av nedskrivningar av andelar i övriga företag som det finns ett ägarintresse i;;;
;;;;;8187;Återföringar av nedskrivningar av långfristiga fordringar hos övriga företag som det finns ett ägarintresse i;;;
;;82;Resultat från övriga värdepapper och långfristiga fordringar (anläggningstillgångar);;;;;;
;■;821;Utdelningar på andelar i andra företag;■;8210;Utdelningar på andelar i andra företag;;;
;;;;;8212;Utdelningar, övriga företag;;;
;;;;;8216;Insatsemissioner, övriga företag;;;
;■;822;Resultat vid försäljning av värdepapper i och långfristiga fordringar hos andra företag;■;8220;Resultat vid försäljning av värdepapper i och långfristiga fordringar hos andra företag;;;
;;;;;8221;Resultat vid försäljning av andelar i andra företag;;;
;;;;;8222;Resultat vid försäljning av långfristiga fordringar hos andra företag;;;
;;;;;8223;Resultat vid försäljning av derivat (långfristiga värdepappersinnehav);;;
;;823;Valutakursdifferenser på långfristiga fordringar;;8230;Valutakursdifferenser på långfristiga fordringar;;;
;;;;;8231;Valutakursvinster på långfristiga fordringar;;;
;;;;;8236;Valutakursförluster på långfristiga fordringar;;;
;;824;Resultatandelar från handelsbolag (andra företag);;8240;Resultatandelar från handelsbolag (andra företag);;;
;■;825;Ränteintäkter från långfristiga fordringar hos och värdepapper i andra företag;■;8250;Ränteintäkter från långfristiga fordringar hos och värdepapper i andra företag;;;
;;;;;8251;Ränteintäkter från långfristiga fordringar;;;
;;;;;8252;Ränteintäkter från övriga värdepapper;;;
;;;;;8254;Skattefria ränteintäkter, långfristiga tillgångar;;;
;;;;;8255;Avkastningsskatt kapitalplacering;;;
;;826;Ränteintäkter från långfristiga fordringar hos koncernföretag;;8260;Ränteintäkter från långfristiga fordringar hos koncernföretag;;;
;;;;;8261;Ränteintäkter från långfristiga fordringar hos moderföretag;;;
;;;;;8262;Ränteintäkter från långfristiga fordringar hos dotterföretag;;;
;;;;;8263;Ränteintäkter från långfristiga fordringar hos andra koncernföretag;;;
;■;827;Nedskrivningar av innehav av andelar i och långfristiga fordringar hos andra företag;■;8270;Nedskrivningar av innehav av andelar i och långfristiga fordringar hos andra företag;;;
;;;;;8271;Nedskrivningar av andelar i andra företag;;;
;;;;;8272;Nedskrivningar av långfristiga fordringar hos andra företag;;;
;;;;;8273;Nedskrivningar av övriga värdepapper hos andra företag;;;
;;828;Återföringar av nedskrivningar av andelar i och långfristiga fordringar hos andra företag;;8280;Återföringar av nedskrivningar av andelar i och långfristiga fordringar hos andra företag;;;
;;;;;8281;Återföringar av nedskrivningar av andelar i andra företag;;;
;;;;;8282;Återföringar av nedskrivningar av långfristiga fordringar hos andra företag;;;
;;;;;8283;Återföringar av nedskrivningar av övriga värdepapper i andra företag;;;
;[Ej K2];829;Värdering till verkligt värde, anläggningstillgångar;[Ej K2];8290;Värdering till verkligt värde, anläggningstillgångar;;;
;;;;[Ej K2];8291;Orealiserade värdeförändringar på anläggningstillgångar;;;
;;;;[Ej K2];8295;Orealiserade värdeförändringar på derivatinstrument;;;
;;83;Övriga ränteintäkter och liknande resultatposter;;;;;;
;■;831;Ränteintäkter från omsättningstillgångar;■;8310;Ränteintäkter från omsättningstillgångar;;;
;;;;;8311;Ränteintäkter från bank;;;
;;;;;8312;Ränteintäkter från kortfristiga placeringar;;;
;;;;;8313;Ränteintäkter från kortfristiga fordringar;;;
;;;;■;8314;Skattefria ränteintäkter;;;
;;;;;8317;Ränteintäkter för dold räntekompensation;;;
;;;;;8319;Övriga ränteintäkter från omsättningstillgångar;;;
;[Ej K2];832;Värdering till verkligt värde, omsättningstillgångar;[Ej K2];8320;Värdering till verkligt värde, omsättningstillgångar;;;
;;;;[Ej K2];8321;Orealiserade värdeförändringar på omsättningstillgångar;;;
;;;;[Ej K2];8325;Orealiserade värdeförändringar på derivatinstrument (oms.-tillg.);;;
;■;833;Valutakursdifferenser på kortfristiga fordringar och placeringar;■;8330;Valutakursdifferenser på kortfristiga fordringar och placeringar;;;
;;;;;8331;Valutakursvinster på kortfristiga fordringar och placeringar;;;
;;;;;8336;Valutakursförluster på kortfristiga fordringar och placeringar;;;
;■;834;Utdelningar på kortfristiga placeringar;■;8340;Utdelningar på kortfristiga placeringar;;;
;■;835;Resultat vid försäljning av kortfristiga placeringar;■;8350;Resultat vid försäljning av kortfristiga placeringar;;;
;;836;Övriga ränteintäkter från koncernföretag;;8360;Övriga ränteintäkter från koncernföretag;;;
;;;;;8361;Övriga ränteintäkter från moderföretag;;;
;;;;;8362;Övriga ränteintäkter från dotterföretag;;;
;;;;;8363;Övriga ränteintäkter från andra koncernföretag;;;
;;837;Nedskrivningar av kortfristiga placeringar;;8370;Nedskrivningar av kortfristiga placeringar;;;
;;838;Återföringar av nedskrivningar av kortfristiga placeringar;;8380;Återföringar av nedskrivningar av kortfristiga placeringar;;;
;■;839;Övriga finansiella intäkter;■;8390;Övriga finansiella intäkter;;;
;;84;Räntekostnader och liknande resultatposter;;;;;;
;;840;Räntekostnader (gruppkonto);;8400;Räntekostnader (gruppkonto);;;
;■;841;Räntekostnader för långfristiga skulder;■;8410;Räntekostnader för långfristiga skulder;;;
;;;;;8411;Räntekostnader för obligations-, förlags- och konvertibla lån;;;
;;;;;8412;Räntedel i årets pensionskostnad;;;
;;;;;8413;Räntekostnader för checkräkningskredit;;;
;;;;;8415;Räntekostnader för andra skulder till kreditinstitut;;;
;;;;;8417;Räntekostnader för dold räntekompensation m.m.;;;
;;;;;8418;Avdragspost för räntesubventioner;;;
;;;;;8419;Övriga räntekostnader för långfristiga skulder;;;
;■;842;Räntekostnader för kortfristiga skulder;■;8420;Räntekostnader för kortfristiga skulder;;;
;;;;;8421;Räntekostnader till kreditinstitut;;;
;;;;■;8422;Dröjsmålsräntor för leverantörsskulder;;;
;;;;■;8423;Räntekostnader för skatter och avgifter;;;
;;;;;8424;Räntekostnader byggnadskreditiv;;;
;;;;;8429;Övriga räntekostnader för kortfristiga skulder;;;
;■;843;Valutakursdifferenser på skulder;■;8430;Valutakursdifferenser på skulder;;;
;;;;;8431;Valutakursvinster på skulder;;;
;;;;;8436;Valutakursförluster på skulder;;;
;;844;Erhållna räntebidrag;;8440;Erhållna räntebidrag;;;
;[Ej K2];845;Orealiserade värdeförändringar på skulder;[Ej K2];8450;Orealiserade värdeförändringar på skulder;;;
;;;;[Ej K2];8451;Orealiserade värdeförändringar på skulder;;;
;;;;[Ej K2];8455;Orealiserade värdeförändringar på säkringsinstrument;;;
;;846;Räntekostnader till koncernföretag;;8460;Räntekostnader till koncernföretag;;;
;;;;;8461;Räntekostnader till moderföretag;;;
;;;;;8462;Räntekostnader till dotterföretag;;;
;;;;;8463;Räntekostnader till andra koncernföretag;;;
;[Ej K2];848;Aktiverade ränteutgifter;[Ej K2];8480;Aktiverade ränteutgifter;;;
;;849;Övriga skuldrelaterade poster;;8490;Övriga skuldrelaterade poster;;;
;;;;;8491;Erhållet ackord på skulder till kreditinstitut m.m.;;;
;;85;(Fri kontogrupp);;;;;;
;;86;(Fri kontogrupp);;;;;;
;;87;(Fri kontogrupp);;;;;;
;;88;Bokslutsdispositioner;;;;;;
;;881;Förändring av periodiseringsfond;;8810;Förändring av periodiseringsfond;;;
;;;;■;8811;Avsättning till periodiseringsfond;;;
;;;;■;8819;Återföring från periodiseringsfond;;;
;;882;Mottagna koncernbidrag;;8820;Mottagna koncernbidrag;;;
;;883;Lämnade koncernbidrag;;8830;Lämnade koncernbidrag;;;
;;884;Lämnade gottgörelser;;8840;Lämnade gottgörelser;;;
;■;885;Förändring av överavskrivningar;■;8850;Förändring av överavskrivningar;;;
;;;;;8851;Förändring av överavskrivningar, immateriella anläggningstillgångar;;;
;;;;;8852;Förändring av överavskrivningar, byggnader och markanläggningar;;;
;;;;;8853;Förändring av överavskrivningar, maskiner och inventarier;;;
;;886;Förändring av ersättningsfond;;8860;Förändring av ersättningsfond;;;
;;;;;8861;Avsättning till ersättningsfond för inventarier;;;
;;;;;8862;Avsättning till ersättningsfond för byggnader och markanläggningar;;;
;;;;;8864;Avsättning till ersättningsfond för djurlager i jordbruk och renskötsel;;;
;;;;;8865;Ianspråktagande av ersättningsfond för avskrivningar;;;
;;;;;8866;Ianspråktagande av ersättningsfond för annat än avskrivningar;;;
;;;;;8869;Återföring från ersättningsfond;;;
;;889;Övriga bokslutsdispositioner;;8890;Övriga bokslutsdispositioner;;;
;;;;;8892;Nedskrivningar av konsolideringskaraktär av anläggningstillgångar;;;
;;;;;8896;Förändring av lagerreserv;;;
;;;;;8899;Övriga bokslutsdispositioner;;;
;;89;Skatter och årets resultat;;;;;;
;■;891;Skatt som belastar årets resultat;■;8910;Skatt som belastar årets resultat;;;
;;892;Skatt på grund av ändrad beskattning;;8920;Skatt på grund av ändrad beskattning;;;
;;893;Restituerad skatt;;8930;Restituerad skatt;;;
;[Ej K2];894;Uppskjuten skatt;[Ej K2];8940;Uppskjuten skatt;;;
;;898;Övriga skatter;;8980;Övriga skatter;;;
;■;899;Resultat;■;8990;Resultat;;;
;;;;■;8999;Årets resultat;;;
	)"};
}
