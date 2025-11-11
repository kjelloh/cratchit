#pragma once

#include "fiscal/BAS_SKV_Crossdependencies.hpp" 
#include "Key.hpp"
#include "fiscal/amount/AmountFramework.hpp" // Amount,
#include "MetaDefacto.hpp"
#include <optional>
#include <vector>

// NOTE: namespace snippets reflecting refactoring from zeroth/main.cpp
// TODO: Clean up when conveniant

namespace BAS {
	extern char const* bas_2022_account_plan_csv;
}

// TODO: Continue to refactor from zeroth/main.cpp as required for the BAS namespace
namespace BAS {
  using AccountNo = unsigned int;
  using OptionalAccountNo = std::optional<AccountNo>;
  using AccountNos = std::vector<AccountNo>;
  using OptionalAccountNos = std::optional<AccountNos>;

  OptionalAccountNo to_account_no(std::string const& s);

} // namespace BAS

inline unsigned first_digit(BAS::AccountNo account_no) {
  return account_no / 1000;
}

// -----------------------
// BEGIN Balance

struct Balance {
	BAS::AccountNo account_no;
	Amount opening_balance;
	Amount change;
	Amount end_balance;
};

std::ostream& operator<<(std::ostream& os,Balance const& balance);

using Balances = std::vector<Balance>;
using BalancesMap = std::map<Date,Balances>;

std::ostream& operator<<(std::ostream& os,BalancesMap const& balances_map);

// END Balance
// -----------------------

namespace BAS {


	// See https://bolagsverket.se/en/foretag/aktiebolag/arsredovisningforaktiebolag/delarochbilagoriarsredovisningen/faststallelseintyg.763.html
	// financial statements approval (fastställelseintyg)
	// See https://bolagsverket.se/en/foretag/aktiebolag/arsredovisningforaktiebolag/delarochbilagoriarsredovisningen.761.html
	// a directors’ report  (förvaltningsberättelse)
	// a profit and loss statement (resultaträkning)
	// a balance sheet (balansräkning)
	// notes (noter).	

	namespace detail {
		// "hidden" poor mans singleton instance creation
		inline Key::Paths bas_2022_account_plan_paths{};
		inline Key::Paths to_bas_2022_account_plan_paths() {
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
	inline Key::Paths const& to_bas_2022_account_plan_paths() {
		// Poor mans "singleton" instance
		static auto const& result = detail::to_bas_2022_account_plan_paths();
		return result;
	}

	void parse_bas_account_plan_csv(std::istream& in,std::ostream& prompt);

	inline Amount to_cents_amount(Amount const& amount) {
		return round((amount*100.0)/Amount{100.0}); // Amount / Amount = real number 
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

	inline OptionalAccountKind to_account_kind(BAS::AccountNo const& bas_account_no) {
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
					// TODO: BAS Accounts 8xxx are MIXED Debit/Credit so consider
					//       to device a way to pin-pint which are Debit and which are Credit?
				case '9': {/* NOT a BAS account (free for use for use as seen fit) */} break;
			}
		}
		return result;
	}

	struct AccountMeta {
		std::string name{};
		OptionalAccountKind account_kind{};
		SKV::SRU::OptionalAccountNo sru_code{};
	};
	using AccountMetas = std::map<BAS::AccountNo,BAS::AccountMeta>;

	namespace detail {
		// "Hidden" Global mapping between BAS account no and its registered meta data like name, SRU code etc (from SIE file(s) imported)
		// See accessor global_account_metas().
		inline AccountMetas global_account_metas{};
	}

	inline AccountMetas const& global_account_metas() {return detail::global_account_metas;}

	namespace anonymous {

		struct AccountTransaction {
			BAS::AccountNo account_no;
			std::optional<std::string> transtext{};
			Amount amount;

      // C++ compiler will generate comparison
      // operators on members in declaration order.
      // Which is ok for our purposes (date - then trans text - then amount)
      auto operator<=>(AccountTransaction const& other) const = default;

      // Replaced by operator<=> / 20251030
			// bool operator<(AccountTransaction const& other) const {
			// 	bool result{false};
			// 	if (account_no == other.account_no) {
			// 		if (amount == other.amount) {
			// 			result = (transtext < other.transtext);
			// 		}
			// 		else result = (amount < other.amount);
			// 	}
			// 	else result = (account_no < other.account_no);
			// 	return result;
			// }

		};
		using OptionalAccountTransaction = std::optional<AccountTransaction>;
		using AccountTransactions = std::vector<AccountTransaction>;

		template <typename T>
		struct JournalEntry_t {
			std::string caption{};
			Date date{};
			T account_transactions{};

      // C++ 'don't pay for what the compiler generates'
      // means that my manual operator<=> below is NOT used
      // by the compiler to generate operator==?
      // For some reason the compiler can't trust my manual <=>
      // to be effective for equality comparison?
      // Anyhow, I need to provide the operator== also.
      bool operator==(const JournalEntry_t& other) const {
        return 
              (date == other.date)
          and (account_transactions == other.account_transactions)
          and (caption == other.caption);
      }

      // Manual operator<-> to define the comparison order
      // different from the declaration order.
      // That is, the compiler would compare members in declaration order
      // if I asked for the defailt one. And I like the declaration order
      // to match my mental model of an entry
      auto operator<=>(const JournalEntry_t& other) const {
        // -1 -> *this < other
        // 0 -> *this == other (But operator== NOT generated by compiler)
        // 1 -> *this > other
        // compare based on date, then transactions, finally caption
        if (auto cmp = date <=> other.date; cmp != 0) return cmp;
        if (auto cmp = account_transactions <=> other.account_transactions; cmp != 0) return cmp;
        return caption <=> other.caption;
      }
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

	// 'MD' = MetaDefacto journal entry
	using MDJournalEntry = MetaDefacto<JournalEntryMeta,BAS::anonymous::JournalEntry>;
	using OptionalMDJournalEntry = std::optional<MDJournalEntry>;
	using MDJournalEntries = std::vector<MDJournalEntry>;

	// MMD -> meta: MDJournalEntry D -> defacto: AccountTransaction
	// MMDD = Meta-MetaDefacto-Defacto Account Transaction

	// Replaced with MDAccountTransaction /20251030
  // using MMDDAccountTransaction = MetaDefacto<BAS::MDJournalEntry,BAS::anonymous::AccountTransaction>;

	struct AccountTransactionMeta {
		// <date> <jem> caption
		//          |
		//          <series> <verno>
		Date date;
		JournalEntryMeta jem; // Aggregate journal entry meta for convenience
		std::string caption;
	};
	AccountTransactionMeta to_account_transaction_meta(BAS::MDJournalEntry const& mdje);

	// meta: {date, jem:{series,verno}} defacto:AccountTransaction
	using MDAccountTransaction = MetaDefacto<BAS::AccountTransactionMeta,BAS::anonymous::AccountTransaction>;

	using OptionalMDAccountTransaction = std::optional<MDAccountTransaction>;
	using MDAccountTransactions = std::vector<MDAccountTransaction>;
} // namespace BAS

// Journal 'DAG' storage <journal id / series> -> <verno> -> record:entry {heading,date,transactions}
using BASJournal = std::map<BAS::VerNo,BAS::anonymous::JournalEntry>;
using BASJournalId = char; // The Id of a single BAS journal is a series character A,B,C,...
using BASJournals = std::map<BASJournalId,BASJournal>; // Swedish BAS Journals named "Series" and labeled with "Id" A,B,C,...

namespace BAS {
	Amount to_mdats_sum(BAS::MDAccountTransactions const& mdats);
} // namespace BAS


// A function object able to create Net+Vat BAS::anonymous::AccountTransactions
class ToNetVatAccountTransactions {
public:

	ToNetVatAccountTransactions(BAS::anonymous::AccountTransaction const& net_at, BAS::anonymous::AccountTransaction const& vat_at)
		:  m_net_at{net_at}
		  ,m_vat_at{vat_at}
			,m_gross_vat_rate{static_cast<float>((net_at.amount != 0)?vat_at.amount/(net_at.amount + vat_at.amount):1.0)}
			,m_sign{(net_at.amount<0)?-1:1} /* 0 gets sign + */ {}

	BAS::anonymous::AccountTransactions operator()(Amount remaining_counter_amount,std::string const& transtext,OptionalAmount const& inc_vat_amount = std::nullopt) {
		BAS::anonymous::AccountTransactions result{};
		Amount gross_amount = (inc_vat_amount)?*inc_vat_amount:remaining_counter_amount;
		BAS::anonymous::AccountTransaction net_at{
			.account_no = m_net_at.account_no
			,.transtext = transtext
			,.amount = BAS::to_cents_amount(static_cast<Amount>(m_sign * gross_amount * (1.0-m_gross_vat_rate)))
		};
		BAS::anonymous::AccountTransaction vat_at{
			.account_no = m_vat_at.account_no
			,.transtext = transtext
			,.amount = BAS::to_cents_amount(static_cast<Amount>(m_sign * gross_amount * m_gross_vat_rate))
		};
		result.push_back(net_at);
		result.push_back(vat_at);
		return result;
	}
private:
	BAS::anonymous::AccountTransaction m_net_at;
	BAS::anonymous::AccountTransaction m_vat_at;
	float m_gross_vat_rate;
	int m_sign;
};

// -----------------------------
// BEGIN Accounting

// TODO: Consider to try and refactor out a 'higher' domain
//       that define cratching accounting operations 
//       based on BAS <- SIE <- 'Accounting'? / 20251028

inline bool are_same_and_less_than_100_cents_apart(BAS::anonymous::AccountTransaction const& at1, BAS::anonymous::AccountTransaction const& at2) {
	return (     (at1.account_no == at2.account_no)
	         and (at1.transtext == at2.transtext)
					 and (are_same_and_less_than_100_cents_apart(at1.amount,at2.amount)));
}

inline bool are_same_and_less_than_100_cents_apart(BAS::anonymous::AccountTransactions const& ats1, BAS::anonymous::AccountTransactions const& ats2) {
	bool result{true};
	if (ats1.size() >= ats2.size()) {
		for (int i=0;i<ats1.size() and result;++i) {
			if (i<ats2.size()) {
				result = are_same_and_less_than_100_cents_apart(ats1[i],ats2[i]);
			}
			else {
				result = abs(ats1[i].amount) < 1.0; // Do not care about cents
			}
		}
	}
	else {
		return are_same_and_less_than_100_cents_apart(ats2,ats1); // Recurse with swapped arguments
	}
	return result;
}

inline bool are_same_and_less_than_100_cents_apart(BAS::MDJournalEntry const& me1, BAS::MDJournalEntry const& me2) {
	return (     	(me1.meta == me2.meta)
						and (me1.defacto.caption == me2.defacto.caption)
						and (me1.defacto.date == me2.defacto.date)
						and (are_same_and_less_than_100_cents_apart(me1.defacto.account_transactions,me2.defacto.account_transactions)));
}

Amount to_positive_gross_transaction_amount(BAS::anonymous::JournalEntry const& aje);
Amount to_negative_gross_transaction_amount(BAS::anonymous::JournalEntry const& aje);
void for_each_anonymous_account_transaction(BAS::anonymous::JournalEntry const& aje,auto& f) {
	for (auto const& at : aje.account_transactions) {
		f(at);
	}
}
bool does_balance(BAS::anonymous::JournalEntry const& aje);

// Pick the negative or positive gross amount and return it without sign
OptionalAmount to_gross_transaction_amount(BAS::anonymous::JournalEntry const& aje);
BAS::anonymous::OptionalAccountTransaction gross_account_transaction(BAS::anonymous::JournalEntry const& aje);
Amount to_account_transactions_sum(BAS::anonymous::AccountTransactions const& ats);

// END Accounting
// -----------------------------

// -----------------------------
// BGIN Accounting IO

std::ostream& operator<<(std::ostream& os,BAS::anonymous::AccountTransaction const& at);
std::string to_string(BAS::anonymous::AccountTransaction const& at);
std::ostream& operator<<(std::ostream& os,BAS::anonymous::AccountTransactions const& ats);
std::ostream& operator<<(std::ostream& os,BAS::anonymous::JournalEntry const& aje);
std::ostream& operator<<(std::ostream& os,BAS::OptionalVerNo const& verno);
std::ostream& operator<<(std::ostream& os,std::optional<bool> flag);
std::ostream& operator<<(std::ostream& os,BAS::JournalEntryMeta const& jem);
std::ostream& operator<<(std::ostream& os,BAS::AccountTransactionMeta const& atm);
std::ostream& operator<<(std::ostream& os,BAS::MDAccountTransaction const& mdat);
std::ostream& operator<<(std::ostream& os,BAS::MDJournalEntry const& mdje);
std::ostream& operator<<(std::ostream& os,BAS::MDJournalEntries const& mdjes);
std::string to_string(BAS::anonymous::JournalEntry const& aje);
std::string to_string(BAS::MDJournalEntry const& mdje);

// END Accounting IO
// -----------------------------
