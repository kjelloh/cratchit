#pragma once

#include "KEY.hpp"
#include "fiscal/amount/AmountFramework.hpp" // Amount,
#include "MetaDefacto.hpp"
#include <optional>
#include <vector>

namespace BAS {
	extern char const* bas_2022_account_plan_csv;
}

// TODO: Continue to refactor from zeroth/main.cpp as required for the BAS namespace
namespace BAS {
  using AccountNo = unsigned int;
  using OptionalAccountNo = std::optional<AccountNo>;
  using AccountNos = std::vector<AccountNo>;
  using OptionalAccountNos = std::optional<AccountNos>;

  OptionalAccountNo to_account_no(std::string const &s);
} // namespace BAS

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

	void parse_bas_account_plan_csv(std::istream& in,std::ostream& prompt);

	Amount to_cents_amount(Amount const& amount) {
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

	OptionalAccountKind to_account_kind(BAS::AccountNo const& bas_account_no) {
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
	BASAccountNumberPath to_bas__account_number_path(BAS::AccountNo const& bas_account_no) {
		BASAccountNumberPath result{};
		// TODO: Search 
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
		AccountMetas global_account_metas{};
	}

	AccountMetas const& global_account_metas() {return detail::global_account_metas;}

	namespace anonymous {

		struct AccountTransaction {
			BAS::AccountNo account_no;
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

