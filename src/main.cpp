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
		const std::regex amount_regex("^\\d+([.,]\\d\\d?)?$"); // 123 123,1 123.1 123,12 123.12
		if (std::regex_match(s,amount_regex)) return TokenID::Amount;
		const std::regex caption_regex("[ -~]+");
		if (std::regex_match(s,caption_regex)) return TokenID::Caption;
		return TokenID::Unknown; 
	}

	std::vector<std::string> splits(std::string const& s,SplitOn split_on) {
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
			result = int_amount;
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

namespace BAS {

	struct AccountTransaction {
		BASAccountNo account_no;
		std::optional<std::string> transtext{};
		Amount amount;
	};
	using AccountTransactions = std::vector<AccountTransaction>;

	namespace anonymous {
		struct JournalEntry {
			std::string caption{};
			std::chrono::year_month_day date{};
			AccountTransactions account_transactions;
		};

		using JournalEntries = std::vector<JournalEntry>;

	} // namespace anonymous

	using VerNo = unsigned int;
	using OptionalVerNo = std::optional<VerNo>;

	struct JournalEntry {
		char series;
		OptionalVerNo verno;
		anonymous::JournalEntry entry;
	};

	using JournalEntries = std::vector<JournalEntry>;

} // namespace BAS

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

struct HeadingAmountDateTransEntry {
	std::string heading{};
	Amount amount;
	std::chrono::year_month_day date{};
};

std::ostream& operator<<(std::ostream& os,HeadingAmountDateTransEntry const& had) {
	os << had.heading;
	os << " " << had.amount;
	os << " " << to_string(had.date); 
	return os;
}

using OptionalHeadingAmountDateTransEntry = std::optional<HeadingAmountDateTransEntry>;

using HeadingAmountDateTransEntries = std::vector<HeadingAmountDateTransEntry>;

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

	JournalEntryTemplate(char series,BAS::JournalEntry const& bje) : m_series{series} {
		auto gross_amount = std::accumulate(bje.entry.account_transactions.begin(),bje.entry.account_transactions.end(),Amount{},[](Amount acc,BAS::AccountTransaction const& account_transaction){
			acc += (account_transaction.amount>0)?account_transaction.amount:0;
			return acc;
		});
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

	char series() const {return m_series;}

	BAS::AccountTransactions operator()(Amount amount) const {
		BAS::AccountTransactions result{};
		std::transform(templates.begin(),templates.end(),std::back_inserter(result),[amount](AccountTransactionTemplate const& att){
			return att(amount);
		});
		return result;
	}
	friend std::ostream& operator<<(std::ostream&, JournalEntryTemplate const&);
private:
	char m_series;
	AccountTransactionTemplates templates{};
};

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
	result.entry.account_transactions = jet(had.amount);
	return result;
}

std::ostream& operator<<(std::ostream& os,JournalEntryTemplate const& entry) {
	os << "\ntemplate: series " << entry.series();
	std::for_each(entry.templates.begin(),entry.templates.end(),[&os](AccountTransactionTemplate const& t){
		os << "\n\t" << t.m_account_no << " " << t.m_percent;
	});
	return os;
}

bool share_tokens(std::string const& s1,std::string const& s2) {
	bool result{false};
	auto had_heading_words = tokenize::splits(s1);
	auto je_heading_words = tokenize::splits(s2);
	for (auto hadw : had_heading_words) {
		for (auto jew : je_heading_words) {
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

bool had_matches_trans(HeadingAmountDateTransEntry const& had,BAS::anonymous::JournalEntry const& je) {
	return share_tokens(had.heading,je.caption);
}
BAS::JournalEntries template_candidates(BASJournals const& journals,auto const& matches) {
	std::cout << "\ntemplate_candidates";
	BAS::JournalEntries result{};
	for (auto const& je : journals) {
		auto const& [series,journal] = je;
		for (auto const& [verno,entry] : journal) {								
			if (matches(entry)) result.push_back({series,verno,entry});
		}
	}
	std::ranges::sort(result,[](auto const& je1,auto const& je2){
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
	BASLedger& ledger() {return m_ledger;}
	BASLedger const& ledger() const {return m_ledger;}
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
private:
	BASJournals m_journals{};
	BASLedger m_ledger{};
	std::map<char,BAS::VerNo> verno_of_last_posted_to{};
	BAS::JournalEntry add(BAS::JournalEntry const& je) {
		BAS::JournalEntry result{je};
		// Assign "actual" sequence number
		auto verno = largest_verno(je.series) + 1;
		result.verno = verno;
		m_journals[je.series][verno] = je.entry;
		return result;
	}
	BAS::VerNo largest_verno(char series) {
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
	,Unknown
};

class ConcreteModel {
public:	
	std::string user_input{};
	PromptState prompt_state{PromptState::Root};
	size_t had_index{};
	BAS::JournalEntries template_candidates{};
	BAS::JournalEntry current_candidate{};
	BAS::AccountTransaction at{};
  std::string prompt{};
	bool quit{};
	SIEEnvironment sie{};
	HeadingAmountDateTransEntries heading_amount_date_entries{};
	// HeadingAmountDateTransEntries hads() {
	// 	HeadingAmountDateTransEntries result{};
	// 	auto [begin,end] = environment.equal_range("HeadingAmountDateTransEntry");
	// 	std::transform(begin,end,std::back_inserter(result),[](auto const& entry){
	// 		return *to_had(entry.second); // Assume success
	// 	});
	// 	return result;
	// }
	std::filesystem::path sie_file_path{};
	std::filesystem::path staged_sie_file_path{};
	// Environment environment{};
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
	result.push_back("<Available Entry Options>");
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
		BAS::VerNo verno;
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
		char series;
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
			if (auto opt_entry = SIE::parse_ver(in)) {
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
	std::ofstream os{p};
	SIE::ostream sieos{os};
	// auto now = std::chrono::utc_clock::now();
	auto now = std::chrono::system_clock::now();
	auto now_timet = std::chrono::system_clock::to_time_t(now);
	auto now_local = localtime(&now_timet);
	sieos.os << "\n" << "#GEN " << std::put_time(now_local, "%Y%m%d");
	for (auto const& entry : sie.unposted()) {
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

struct Updater {
	Model model;
	Cmd operator()(Key const& key) {
		// std::cout << "\noperator(Key)";
		if (model->user_input.size()==0) model->prompt = prompt_line(model->prompt_state);
		Cmd cmd{};
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
								model->template_candidates = template_candidates(model->sie.journals(),[had](BAS::anonymous::JournalEntry const& je){
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
						auto iter = model->template_candidates.begin();
						auto end = model->template_candidates.end();
						std::advance(iter,ix);
						if (iter != end) {
							auto tp = to_template(*iter);
							if (tp) {
								auto iter = model->heading_amount_date_entries.begin();
								auto end = model->heading_amount_date_entries.end();
								std::advance(iter,model->had_index);
								if (iter != end) {
									auto had = *iter;
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
										model->current_candidate = je;
										model->prompt_state = PromptState::AccountIndex;
									}
									else {
										auto staged_je = model->sie.stage(je);
										if (staged_je) {
											prompt << "\n" << *staged_je << " STAGED";
											model->heading_amount_date_entries.erase(iter);
											model->prompt_state = PromptState::HADIndex;
										}
										else {
											prompt << "\nSORRY - Failed to stage entry";
											model->prompt_state = PromptState::Root;
										}
									}
								}
							}
						}
						else {
							prompt << "\nPlease enter a valid index";
						}
					} break;
					case PromptState::AccountIndex: {
						auto iter = model->current_candidate.entry.account_transactions.begin();
						std::advance(iter,ix);
						prompt << "\n" << *iter;
						model->at = *iter;
						model->prompt_state = PromptState::Amount;
					}
				}
			}
			else if (ast[0] == "-sie") {
				// Import sie and add as base of our environment
				if (ast.size()==1) {
					// List current sie environment
					prompt << model->sie;					
					// std::cout << model->sie;
				}
				else if (ast.size()>1) {
					// assume -sie <file path>
					auto sie_file_name = ast[1];
					std::filesystem::path sie_file_path{sie_file_name};
					if (std::filesystem::exists(sie_file_path)) {
						if (auto sie_env = from_sie_file(sie_file_path)) {
							model->sie_file_path = sie_file_path;
							model->sie = std::move(*sie_env);
							// Log
							// std::cout << model->sie;
							if (auto sse = from_sie_file(model->staged_sie_file_path)) {
								auto unstaged = model->sie.stage(*sse);
								for (auto const& je : unstaged) {
									prompt << "\nnow posted " << je; 
								}
							}							
						}
						else {
							// failed to parse sie-file into an SIE Environment 
							prompt << "\nERROR - Failed to import sie file " << sie_file_path;
						}
					}
					else {
						prompt << "\nERROR: File does not exist or is not a valid sie-file: " << sie_file_path;
					}
				}
			}
			else if (ast[0] == "-had") {
				if (ast.size()==1) {
					// Expose current hads (Heading Amount Date transaction entries) to the user
					std::ranges::sort(model->heading_amount_date_entries,falling_date);
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
					model->template_candidates = template_candidates(model->sie.journals(),[command](BAS::anonymous::JournalEntry const& je){
						return share_tokens(command,je.caption);
					});
					for (int i = 0; i < model->template_candidates.size(); ++i) {
						prompt << "\n    " << i << " " << model->template_candidates[i];
					}
				}
				else if (model->prompt_state == PromptState::Amount) {
					std::cout << "\nPromptState::Amount " << std::quoted(command);
					try {
						auto amount = to_amount(command);
						if (amount) {
							prompt << "\nAmount " << *amount;
							model->at.amount = *amount;
							model->current_candidate = updated_entry(model->current_candidate,model->at);
							auto je = model->sie.stage(model->current_candidate);
							if (je) {
								prompt << "\n" << *je;
								// Erase the 'had' we just turned into journal entry and staged
								auto iter = model->heading_amount_date_entries.begin();
								auto end = model->heading_amount_date_entries.end();
								std::advance(iter,model->had_index);
								if (iter != end) {
									model->heading_amount_date_entries.erase(iter);
								}							
							}
							else {
								prompt << "\n" << "Sorry - This transaction redeemed a duplicate";
							}
							model->prompt_state = PromptState::Root;
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
			unposted_to_sie_file(updater.model->sie,updater.model->staged_sie_file_path);
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
	std::filesystem::path staged_sie_file_path{};
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
			if (auto key_iter = val_iter->second.find("path"); key_iter != val_iter->second.end()) {
				model->sie_file_path = {key_iter->second};
			}			
		}
		if (auto sie_environment = from_sie_file(model->sie_file_path)) {
			model->sie = std::move(*sie_environment);
		}
		if (auto sse = from_sie_file(model->staged_sie_file_path)) {
			model->sie.stage(*sse);
		}
		model->heading_amount_date_entries = this->hads_from_environment(environment);
		return model;
	}
	Environment environment_from_model(Model const& model) {
		Environment result{};
		for (auto const& entry :  model->heading_amount_date_entries) {
			result.insert({"HeadingAmountDateTransEntry",to_environment_value(entry)});
		}
		result.insert({"sie_file",to_environment_value(std::string("path") + "=" + model->sie_file_path.string())});
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
    std::queue<Msg> in{};
};

int main(int argc, char *argv[])
{
    std::string sPATH{std::getenv("PATH")};
    // std::cout << "\nPATH=" << sPATH;
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