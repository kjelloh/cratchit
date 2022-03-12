#include <iostream>
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

namespace tokenize {
	// returns s split into first,second on provided delimiter delim.
	// split fail returns first = "" and second = s
	std::pair<std::string,std::string> split(std::string s,char delim) {
		auto pos = s.find(delim);
		if (pos<s.size()) return {s.substr(0,pos),s.substr(pos+1)};
		else return {"",s}; // split failed (return right = unsplit string)
	}

	std::vector<std::string> splits(std::string s,char delim) {
		std::vector<std::string> result;
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
		const std::regex date_regex("[2-9]\\d{3}([0]\\d|[1][0-2])([0-2]\\d|[3][0-1])");
		if (std::regex_match(s,date_regex)) return TokenID::Date;
		const std::regex amount_regex("\\d+.\\d\\d");
		if (std::regex_match(s,amount_regex)) return TokenID::Amount;
		const std::regex caption_regex("[ -~]+");
		if (std::regex_match(s,caption_regex)) return TokenID::Caption;
		return TokenID::Unknown; 
	}

	std::vector<std::string> splits(std::string const& s,SplitOn split_on) {
		std::vector<std::string> result{};
		auto spaced_tokens = splits(s,' ');
		std::vector<TokenID> ids{};
		for (auto const& s : spaced_tokens) {
			ids.push_back(token_id_of(s));
		}
		for (int i=0;i<spaced_tokens.size();++i) {
			std::cout << "\n" << spaced_tokens[i] << " id:" << static_cast<int>(ids[i]);
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

    class ParseWord : public Parse<std::string> {
    public:
        using P = std::string;
        virtual ParseResult<P> operator()(In const& in) const {
            ParseResult<P> result{};
            P p{};
            auto iter=in.begin();
            for (;iter!=in.end();++iter) if (delimeters.find(*iter)==std::string::npos) break; // skip delimeters
            for (;iter!=in.end();++iter) {
                if (delimeters.find(*iter)!=std::string::npos) break;
                p += *iter;
            }
            if (p.size()>0) result = {p,In{iter,in.end()}}; // empty word = unsuccessful
            return result;
        }
    private:
        std::string const delimeters{" ,.;:="};
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

using Amount= float;
using optional_amount = std::optional<Amount>;
optional_amount to_amount(std::string const& sAmount) {
	std::cout << "\nto_amount " << sAmount;
	optional_amount result{};
	Amount amount{};
	std::istringstream is{sAmount};
	if (is >> amount) {
		result = amount;
		std::cout << "\nAmount = " << amount;
	}
	return result;
}

using BASAccountNo = unsigned int;
// The BAS Ledger keeps a record of BAS account changes as recorded in Journal(s) 
class BASLedger {
public:
	struct Entry {
		std::string caption{};
		Amount amount;
	};
	using Entries = std::map<int,Entry>;
	Entries& entries() {return m_entries;}
	Entries const& entries() const {return m_entries;}
private:
	Entries m_entries{};
};

// The BAS Journal keeps a record of all documented transactions of a specific type.
/*
Utskrivet 2022-02-17 17:43

Vernr: A 11
Bokföringsdatum: 2021-07-14
Benämning: Kjell&Company

Konto	Benämning									Debet		Kredit		
1920	PlusGiro												89,90
1227	Elektroniklabb - Lager V-slot 2040 Profil	71,92	
2641	Debiterad ingående moms						17,98	
*/

struct BASAccountTransaction {
	BASAccountNo account_no;
	std::optional<std::string> transtext{};
	Amount amount;
};
using BASAccountTransactions = std::vector<BASAccountTransaction>;

std::ostream& operator<<(std::ostream& os,BASAccountTransaction const& at) {
	os << at.account_no;
	os << " " << at.transtext;
	os << " " << at.amount;
	return os;
};

std::string to_string(BASAccountTransaction const& at) {
	std::ostringstream os{};
	os << at;
	return os.str();
};
struct BASJournalEntry {
	std::string caption{};
	std::chrono::year_month_day date{};
	BASAccountTransactions account_transactions;
};

using BASJournalEntries = std::vector<BASJournalEntry>;

std::ostream& operator<<(std::ostream& os,BASJournalEntry const& je) {
	os << je.caption;
	for (auto const& at : je.account_transactions) {
		os << "\n\t" << to_string(at); 
	}
	return os;
};

std::string to_string(BASJournalEntry const& je) {
	std::ostringstream os{};
	os << je;
	return os.str();
};

using BASJournal = std::map<int,BASJournalEntry>;
using BASJournals = std::map<char,BASJournal>; // Swedish BAS Journals named "Series" and labeled A,B,C,...

/*
	a) Heading + Amount + Date => HAD Entry List
	b) HDA Entry + Historic Journal Entry match => Journal Entry Template List
	c) User HDA Entry + selected Template => Journal Entry Candidates
	d) User select Journal Entry Candidate => Staged Journal Entries
	e) User Commits Staged Journal Entries to Series and Sequence Number

	I) We need to be able to match a Header Date Amount entry to historical Journal Entries
	II) We need to enbale the user to select the template to Apply and Stage a Journal Entry
	III) We need to enable the user to Commit Staged Journal Entries to a Series and valid sequence number
*/

struct HeadingAmountDateTransEntry {
	std::string heading{};
	Amount amount;
	std::chrono::year_month_day date{};
};
using OptionalHeadingAmountDateTransEntry = std::optional<HeadingAmountDateTransEntry>;

using HeadingAmountDateTransEntries = std::vector<HeadingAmountDateTransEntry>;


struct AccountTransactionTemplate {
	AccountTransactionTemplate(BASAccountNo account_no,Amount gross_amount,Amount account_amount) 
		: m_account_no{account_no},m_factor{account_amount / gross_amount}  {}
	BASAccountNo m_account_no;
	float m_factor;
	BASAccountTransaction operator()(Amount amount) const {
		BASAccountTransaction result{.account_no = m_account_no,.transtext="",.amount=amount*m_factor};
		return result;
	}
};
using AccountTransactionTemplates = std::vector<AccountTransactionTemplate>;

class JournalEntryTemplate {
public:

	JournalEntryTemplate(BASJournalEntry const bje) {
		auto gross_amount = std::accumulate(bje.account_transactions.begin(),bje.account_transactions.end(),Amount{},[](Amount acc,BASAccountTransaction const& account_transaction){
			acc += (account_transaction.amount>0)?account_transaction.amount:0;
			return acc;
		});
		if (gross_amount >= 0.01) {
			std::transform(bje.account_transactions.begin(),bje.account_transactions.end(),std::back_inserter(templates),[gross_amount](BASAccountTransaction const& account_transaction){
				AccountTransactionTemplate result(
					 account_transaction.account_no
					,gross_amount
					,account_transaction.amount
				);
				return result;
			});
		}
	}

	BASAccountTransactions operator()(Amount amount) const {
		BASAccountTransactions result{};
		std::transform(templates.begin(),templates.end(),std::back_inserter(result),[amount](AccountTransactionTemplate const& att){
			return att(amount);
		});
		return result;
	}
	friend std::ostream& operator<<(std::ostream&, JournalEntryTemplate const&);
private:
	AccountTransactionTemplates templates{};
};

using JournalEntryTemplateList = std::vector<JournalEntryTemplate>;
using OptionalJournalEntryTemplate = std::optional<JournalEntryTemplate>;

OptionalJournalEntryTemplate to_template(BASJournalEntry const& entry) {
	OptionalJournalEntryTemplate result(entry);
	return result;
}

BASJournalEntry to_journal_entry(HeadingAmountDateTransEntry const& had,JournalEntryTemplate const& jet) {
	BASJournalEntry result{};
	result.caption = had.heading;
	result.date = had.date;
	result.account_transactions = jet(had.amount);
	return result;
}

std::ostream& operator<<(std::ostream& os,JournalEntryTemplate const& entry) {
	os << "\ntemplate";
	std::for_each(entry.templates.begin(),entry.templates.end(),[&os](AccountTransactionTemplate const& t){
		os << "\n\t" << t.m_account_no << " " << t.m_factor;
	});
	return os;
}

class SIEEnvironment {
public:
	BASJournals& journals() {return m_journals;}
	BASJournals const& journals() const {return m_journals;}
	BASLedger& ledger() {return m_ledger;}
	BASLedger const& ledger() const {return m_ledger;}
private:
	BASJournals m_journals{};
	BASLedger m_ledger{};
};

using OptionalSIEEnvironment = std::optional<SIEEnvironment>;

using EnvironmentValue = std::map<std::string,std::string>;
using Environment = std::multimap<std::string,EnvironmentValue>;

OptionalJournalEntryTemplate template_of(OptionalHeadingAmountDateTransEntry const& had,SIEEnvironment const& sie_environ) {
	OptionalJournalEntryTemplate result{};
	if (had) {
		BASJournalEntries candidates{};
		for (auto const& je : sie_environ.journals()) {
			auto const& [series,journal] = je;
			for (auto const& [verno,entry] : journal) {								
				if (entry.caption.find(had->heading) != std::string::npos) {
					candidates.push_back(entry);
				}
			}
		}
		// select the entry with the latest date
		std::nth_element(candidates.begin(),candidates.begin(),candidates.end(),[](auto const& je1, auto const& je2){
			return (je1.date > je2.date);
		});
		result = to_template(candidates.front());
	}
	return result;
}


// "belopp=1389,50;datum=20221023;rubrik=Klarna"
std::string to_string(EnvironmentValue const& ev) {
	// Expand to variants when EnvironmentValue is no longer a simple string (if ever?)
	std::string result = std::accumulate(ev.begin(),ev.end(),std::string{},[](auto acc,auto const& entry){
		if (acc.size()>0) acc += ";"; // separator
		acc += entry.first;
		acc += "=";
		acc += entry.second;
		return acc;
	});
	return result;
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
	os << entry.first;
	os << " ";
	os << std::quoted(to_string(entry.second));
	return os.str();
}

using optional_year_month_day = std::optional<std::chrono::year_month_day>;
optional_year_month_day to_date(std::string const& sYYYYMMDD) {
	optional_year_month_day result{};
	if (sYYYYMMDD.size()==8) {
		result = std::chrono::year_month_day(
			std::chrono::year{std::stoi(sYYYYMMDD.substr(0,4))}
			,std::chrono::month{static_cast<unsigned>(std::stoul(sYYYYMMDD.substr(4,2)))}
			,std::chrono::day{static_cast<unsigned>(std::stoul(sYYYYMMDD.substr(6,2)))});
	}
	return result;
}
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

EnvironmentValue to_environment_value(HeadingAmountDateTransEntry const had) {
	std::cout << "\nhad.amount" << had.amount << " had.date" << had.date;
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


struct ConcreteModel {
	std::string user_input{};
    std::string prompt{};
	bool quit{};
	Environment environment{};
	SIEEnvironment sie{};
};

using Model = std::unique_ptr<ConcreteModel>; // "as if" immutable (pass around the same instance)

struct Key {char value;};
using Command = std::string;
struct Quit {};
struct Nop {};

using Msg = std::variant<Nop,Key,Quit,Command>;
struct Cmd {Msg msg;};
using Ux = std::vector<std::string>;

Cmd to_cmd(std::string const& user_input) {
	Cmd result{Nop{}};
	if (user_input == "quit" or user_input=="q") result.msg = Quit{};
	else if (user_input.size()>0) result.msg = Command{user_input};
	return result;
}

std::vector<std::string> help_for(std::string topic) {
	std::vector<std::string> result{};
	result.push_back("Enter 'Quit' or 'q' to quit");
	return result;
}

namespace SIE {
	using Stream = std::istream;
	struct Tag {
		std::string const expected;
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
		char series;
		unsigned int verno;
		std::chrono::year_month_day verdate;
		std::string vertext;
		std::optional<std::chrono::year_month_day> regdate{};
		std::optional<std::string> sign{};
		std::vector<Trans> transactions{};
	};
	struct AnonymousLine {};
	using SIEFileEntry = std::variant<Ver,Trans,AnonymousLine>;
	using SIEParseResult = std::optional<SIEFileEntry>;
	std::istream& operator>>(std::istream& in,Tag const& tag) {
		if (in.good()) {
			std::cout << "\n>>Tag[expected:" << tag.expected;
			auto pos = in.tellg();
			std::string token{};
			if (in >> token) {
				std::cout << ",read:" << token << "]";
				if (token != tag.expected) {
					in.seekg(pos); // Reset position for failed tag
					in.setstate(std::ios::failbit); // failed to read expected tag
					std::cout << " rejected";
				}
			}
			else {
				std::cout << "null";
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
			std::cout << "\nscraps:" << scraps;
		}
		return in;		
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
			std::cout << trans_tag << trans.account_no << " " << trans.amount;
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
				std::cout << "\nVer: " << ver.series << " " << ver.verno << " " << ver.vertext;
				while (true) {
					if (auto entry = parse_TRANS(in,"#TRANS")) {
						std::cout << "\nTRANS :)";	
						ver.transactions.push_back(std::get<Trans>(*entry));					
					}
					else if (auto entry = parse_TRANS(in,"#BTRANS")) {
						// Ignore
						std::cout << " Ignored";
					}
					else if (auto entry = parse_TRANS(in,"#RTRANS")) {
						// Ignore
						std::cout << " Ignored";
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
		}
		in.seekg(pos); // Reset position in case of failed parse
		in.clear(); // Reset failbit to allow try for other parse
		return result;
	}

	SIEParseResult parse_any_line(Stream& in) {
		if (in.fail()) {
			std::cout << "\nparse_any_line in==fail";
		}
		auto pos = in.tellg();
		std::string line{};
		if (std::getline(in,line)) {
			std::cout << "\n\tany=" << line;
			return AnonymousLine{};
		}
		else {
			std::cout << "\n\tany null";
			in.seekg(pos);
			return {};
		}
	}
}

std::ostream& operator<<(std::ostream& os,SIEEnvironment const& sie_environment) {
	for (auto const& je : sie_environment.journals()) {
		auto& [series,journal] = je;
		os << "\nJOURNAL " << series;
		for (auto const& [verno,entry] : journal) {
			os << "\nentry:" << verno << " " << to_string(entry);
		}
	}
	return os;
}

OptionalSIEEnvironment from_sie_file(std::filesystem::path const& sie_file_path) {
	OptionalSIEEnvironment result{};
	std::ifstream in{sie_file_path};
	if (in) {
		SIEEnvironment sie_environment{};
		while (true) {
			std::cout << "\nparse";
			if (auto opt_entry = SIE::parse_ver(in)) {
				SIE::Ver ver = std::get<SIE::Ver>(*opt_entry);
				std::cout << "\n\tVER!";
				auto& journal_entries = sie_environment.journals()[ver.series];
				journal_entries[ver.verno] = BASJournalEntry{
					.caption = ver.vertext
				};
				auto& this_journal_entry = journal_entries[ver.verno];
				for (auto const& trans : ver.transactions) {								
					this_journal_entry.account_transactions.push_back(BASAccountTransaction{
						.account_no = trans.account_no
						,.transtext = trans.transtext
						,.amount = trans.amount
					});
				}
			}
			else if (auto opt_entry = SIE::parse_any_line(in)) {
				std::cout << "\n\tANY";
			}
			else break;
		}
		result = std::move(sie_environment);
	}
	return result;
}

struct Updater {
	Model model;
	Cmd operator()(Key const& key) {
		// std::cout << "\noperator(Key)";
		if (model->user_input.size()==0) model->prompt = "\ncratchit>";
		Cmd cmd{Nop{}};
		if (std::isprint(key.value)) {
			model->user_input += key.value;
			model->prompt += key.value;
		}
		else {
			cmd = to_cmd(model->user_input);
			model->user_input.clear();
		}
		return cmd;
	}
	Cmd operator()(Command const& command) {
		// std::cout << "\noperator(Command)";
		std::ostringstream os{};
		auto ast = tokenize::splits(command,' ');
		if (ast.size() > 0) {
			// os << model->ux_state(ast) ??
			if (ast[0] == "-env") {
				for (auto const& entry : model->environment) {
					std::cout << "\n" << entry.first << " " << to_string(entry.second);
				}
			}
			else if (ast[0] == "-sie") {
				// Import sie and add as base of our environment
				if (ast.size()==1) {
					// List current sie environment
					std::cout << model->sie;
				}
				else if (ast.size()>1) {
					// assume -sie <file path>
					auto sie_file_name = ast[1];
					std::filesystem::path sie_file_path{sie_file_name};
					if (std::filesystem::exists(sie_file_path)) {
						if (auto sie_env = from_sie_file(sie_file_path)) {
							model->sie = std::move(*sie_env);
							if (model->environment.count("sie_file")>0) model->environment.erase("sie_file"); // There can be only one :)
							model->environment.insert(std::make_pair("sie_file",to_environment_value(std::string("path") + "=" + sie_file_path.string()))); // new one
							// Log
							std::cout << model->sie;
						}
						else {
							// failed to parse sie-file into an SIE Environment 
						}
					}
					else {
						model->prompt = "ERROR: File does not exist or is not a valid sie-file: " + sie_file_path.string();
					}
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
					auto ev = to_environment_value(had);
					model->environment.insert(std::make_pair("HeadingAmountDateTransEntry",ev));

					// TEST
					if (true) {
						// Find previous BAS Journal entries that may define how
						// we should account for this entry

						// LOG / Test to match HeadingAmountDateTransEntry (HAD) to a previous Journal Entry
						BASJournalEntries candidates{};
						for (auto const& je : model->sie.journals()) {
							auto const& [series,journal] = je;
							for (auto const& [verno,entry] : journal) {								
								if (entry.caption.find(ev["rubrik"]) != std::string::npos) {
									candidates.push_back(entry);
								}
							}
						}
						// Log / Test of using candidate Journal Entry as template for this HAD
						for (auto const& je : candidates) {
							std::cout << "\ncandidate:" << to_string(je);
							auto et = to_template(je);
							if (et) {
								std::cout << "\nTemplate" << *et;
								// Transform had to journal entry using this template candidate
								std::cout << "\nSIE " << to_journal_entry(had,*et);
							}
						}
					}
					if (auto iter = model->environment.find("HeadingAmountDateTransEntry");iter != model->environment.end()) {
						auto et = template_of(to_had(iter->second),model->sie);
						if (et) {
								std::cout << "\nTemplate" << *et;
								// Transform had to journal entry using this template candidate
								auto je = to_journal_entry(had,*et);
								std::cout << "\njournal entry " << je;
						}
					}

				}
				else {
					os << "\nERROR - Expected Caption + Amount + Date";
				}
			}
		}
		os << "\ncratchit:";
		model->prompt = os.str();
		return {Nop{}};
	}
	Cmd operator()(Quit const& quit) {
		// std::cout << "\noperator(Quit)";
		std::ostringstream os{};
		os << "\nBy for now :)";
		model->prompt = os.str();
		model->quit = true;
		return {Nop{}};
	}
	Cmd operator()(Nop const& nop) {
		// std::cout << "\noperator(Nop)";
		std::ostringstream os{};
		auto help = help_for("");
		for (auto const& line : help) os << "\n<Options>\n" << line;
		os << "\ncratchit:";
		model->prompt = os.str();
		return {Nop{}};
	}	
};

class Cratchit {
public:
	Cratchit(std::filesystem::path const& p) 
		: cratchit_file_path{p} {}

	Model init(Command const& command) {
		Model model = std::make_unique<ConcreteModel>();
		model->prompt += "\nInit from ";
		model->prompt += cratchit_file_path;
		model->environment = environment_from_file(cratchit_file_path);
		if (auto val_iter = model->environment.find("sie_file");val_iter != model->environment.end()) {
			if (auto key_iter = val_iter->second.find("path"); key_iter != val_iter->second.end()) {
				std::filesystem::path sie_file_path{key_iter->second};
				if (auto sie_environment = from_sie_file(sie_file_path)) {
					model->sie = std::move(*sie_environment);
				}
			}
		}
		model->prompt += "\ncratchit>";
		return model;
	}
	std::pair<Model,Cmd> update(Msg const& msg,Model&& model) {
		Updater updater{std::move(model)};
		auto cmd = std::visit(updater,msg);
		if (updater.model->quit) {
			environment_to_file(updater.model->environment,cratchit_file_path);
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
	bool is_value_line(std::string const& line) {
		return (line.size()==0)?false:(line.substr(0,2) != R"(//)");
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
			std::cerr << "\nERROR - Write to " << p << " failed. Exception:" << e.what();
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
			// Create the ux to view
			auto ux = cratchit.view(model);
			// Render the ux
			for (auto const&  row : ux) std::cout << row;
			if (model->quit) break; // Done
			// process events (user input)
			if (in.size()>0) {
				auto msg = in.front();
				in.pop();
				// std::cout << "\nmsg[" << msg.index() << "]";
				// Turn Event (Msg) into updated model
				auto [updated_model,cmd] = cratchit.update(msg,std::move(model));
				model = std::move(updated_model);
				if (std::holds_alternative<Nop>(cmd.msg) == false) in.push(cmd.msg);
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
    std::queue<Msg> in{};
};

int main(int argc, char *argv[])
{
    std::string sPATH{std::getenv("PATH")};
    std::cout << "\nPATH=" << sPATH;
    std::string command{};
    for (int i=1;i<argc;i++) command+= std::string{argv[i]} + " ";
    auto current_path = std::filesystem::current_path();
    auto environment_file_path = current_path / "cratchit.env";
    REPL repl{environment_file_path};
    repl.run(command);
    // std::cout << "\nBye for now :)";
    std::cout << std::endl;
    return 0;
}