#pragma once
#include "fiscal/BASFramework.hpp" // BAS::anonymous::JournalEntry,...
#include "fiscal/amount/HADFramework.hpp" // HeadingAmountDateTransEntry,...
#include "sie/SIEDocumentFramework.hpp" // SIEArchive,...

namespace BAS {
	// TYPED Journal Entries (to identify patterns of interest in how the individual account transactions of an entry is dispositioned in amount and on semantics of the account)
	namespace anonymous {
		using AccountPostingType = std::set<std::string>;
		using TypedAccountPostings = std::map<BAS::anonymous::AccountPosting,AccountPostingType>;
		using TypedAccountPosting = TypedAccountPostings::value_type;
		using TypedJournalEntry = BAS::anonymous::JournalEntry_t<TypedAccountPostings>;
	} // anonymous

	using MDTypedJournalEntry = MetaDefacto<BAS::WeakJournalEntryMeta,anonymous::TypedJournalEntry>;
	using TypedMetaEntries = std::vector<MDTypedJournalEntry>;

  void for_each_typed_account_transaction(BAS::MDTypedJournalEntry const& mdtje,auto& f) {
    for (auto const& tat : mdtje.defacto.account_transactions) {
      f(tat);
    }
	}

	namespace kind {

		using BASAccountTopology = std::set<BAS::AccountNo>;
		using AccountPostingTypeTopology = std::set<std::string>;

		enum class ATType {
			// NOTE: Restrict to 16 values (Or to_at_types_order stops being reliable)
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

    ATType to_at_type(std::string const& prop);
    std::size_t to_at_types_order(BAS::kind::AccountPostingTypeTopology const& topology);
		std::vector<std::string> sorted(AccountPostingTypeTopology const& topology);

		namespace detail {
			template <typename T>
			struct hash {};

			template <>
			struct hash<BASAccountTopology> {
				std::size_t operator()(BASAccountTopology const& bat) {
					std::size_t result{};
					for (auto const& account_no : bat) {
						auto h = std::hash<BAS::AccountNo>{}(account_no);
						result = result ^ (h << 1);
					}
					return result;
				}
			};

			template <>
			struct hash<AccountPostingTypeTopology> {
				std::size_t operator()(AccountPostingTypeTopology const& props) {
					std::size_t result{};
					for (auto const& prop : props) {
						auto h = std::hash<std::string>{}(prop);
						result = result ^ static_cast<std::size_t>((h << 1));
					}
					return result;
				}
			};
		} // namespace detail

		BASAccountTopology to_accounts_topology(MDJournalEntry const& mdje);
		BASAccountTopology to_accounts_topology(MDTypedJournalEntry const& tme);
		AccountPostingTypeTopology to_types_topology(MDTypedJournalEntry const& tme);
		std::size_t to_signature(BASAccountTopology const& bat);
		std::size_t to_signature(AccountPostingTypeTopology const& met);

	} // namespace kind

} // BAS

BAS::MDTypedJournalEntry to_typed_md_entry(BAS::MDJournalEntry const& mdje);

// Journal Entry VAT Type
enum class JournalEntryVATType {
	Undefined = -1
	,NoVAT = 0
	,SwedishVAT = 1
	,EUVAT = 2
	,VATReturns = 3
  ,VATClearing = 4 // VAT Returns Cleared by Swedish Skatteverket (SKV)
  ,SKVInterest = 6
  ,SKVFee = 7
	,VATTransfer = 8 // VAT Clearing and Settlement in one Journal Entry
	,Unknown
// 	case 0: {
		// No VAT in candidate.
// case 1: {
// 	// Swedish VAT detcted in candidate.
// case 2: {
// 	// EU VAT detected in candidate.
// case 3: {
		//  M2 "Momsrapport 2021-07-01 - 2021-09-30" 20210930
		// 	 vat = sort_code: 0x6 : "Utgående moms, 25 %":2610 "" 83300
		// 	 eu_vat vat = sort_code: 0x56 : "Utgående moms omvänd skattskyldighet, 25 %":2614 "" 1654.23
		// 	 vat = sort_code: 0x6 : "Ingående moms":2640 "" -1690.21
		// 	 vat = sort_code: 0x6 : "Debiterad ingående moms":2641 "" -849.52
		// 	 vat = sort_code: 0x6 : "Redovisningskonto för moms":2650 "" -82415
		// 	 cents = sort_code: 0x7 : "Öres- och kronutjämning":3740 "" 0.5
// case 4: {
	// 10  A2 "Utbetalning Moms från Skattekonto" 20210506
	// transfer = sort_code: 0x1 : "Avräkning för skatter och avgifter (skattekonto)":1630 "Utbetalning" -802
	// transfer = sort_code: 0x1 : "Avräkning för skatter och avgifter (skattekonto)":1630 "Momsbeslut" 802
	// transfer vat = sort_code: 0x16 : "Momsfordran":1650 "" -802
	// transfer = sort_code: 0x1 : "PlusGiro":1920 "" 802
};

std::ostream& operator<<(std::ostream& os,JournalEntryVATType const& vat_type);
JournalEntryVATType to_vat_type(BAS::MDTypedJournalEntry const& tme);

using TypedMDJournalEntryVisitorF = std::function<void(BAS::MDTypedJournalEntry const&)>;
void for_each_typed_md_entry(SIEArchive const& sie_archive,TypedMDJournalEntryVisitorF const& f);

// ==================================================================================
// Had -> journal_entry -> Template

BAS::MDJournalEntry to_md_entry(BAS::MDTypedJournalEntry const& tme);

class AccountPostingTemplate {
public:
	AccountPostingTemplate(Amount gross_amount,BAS::anonymous::AccountPosting const& at)
		:  m_at{at}
			,m_percent{static_cast<int>(round(at.amount*100 / gross_amount))}  {}
	BAS::anonymous::AccountPosting operator()(Amount amount) const {
		// BAS::anonymous::AccountPosting result{.account_no = m_account_no,.transtext="",.amount=amount*m_factor};
		BAS::anonymous::AccountPosting result{
			 .account_no = m_at.account_no
			,.transtext = m_at.transtext
			,.amount=static_cast<Amount>(round(amount*m_percent)/100.0)};
		return result;
	}
	int percent() const {return m_percent;}
private:
	BAS::anonymous::AccountPosting m_at;
	int m_percent;
	friend std::ostream& operator<<(std::ostream& os,AccountPostingTemplate const& att);
};
using AccountPostingTemplates = std::vector<AccountPostingTemplate>;

class JournalEntryTemplate {
public:

	JournalEntryTemplate(BAS::Series series,BAS::MDJournalEntry const& mdje) : m_series{series} {
		if (auto optional_gross_amount = to_gross_transaction_amount(mdje.defacto)) {
			auto gross_amount = *optional_gross_amount;
			if (gross_amount >= 0.01) {
				std::transform(mdje.defacto.account_transactions.begin(),mdje.defacto.account_transactions.end(),std::back_inserter(templates),[gross_amount](BAS::anonymous::AccountPosting const& at){
					AccountPostingTemplate result{gross_amount,at};
					return result;
				});
				std::sort(this->templates.begin(),this->templates.end(),[](auto const& e1,auto const& e2){
					return (abs(e1.percent()) > abs(e2.percent())); // greater to lesser
				});
			}
			else {
				std::cout << "DESIGN INSUFFICIENY - JournalEntryTemplate constructor failed to determine gross amount";
			}
		}
	}

	BAS::Series series() const {return m_series;}

	BAS::anonymous::AccountPostings operator()(Amount amount) const {
		BAS::anonymous::AccountPostings result{};
		std::transform(templates.begin(),templates.end(),std::back_inserter(result),[amount](AccountPostingTemplate const& att){
			return att(amount);
		});
		return result;
	}
	friend std::ostream& operator<<(std::ostream&, JournalEntryTemplate const&);
private:
	BAS::Series m_series;
	AccountPostingTemplates templates{};
}; // class JournalEntryTemplate

using JournalEntryTemplateList = std::vector<JournalEntryTemplate>;
using OptionalJournalEntryTemplate = std::optional<JournalEntryTemplate>;

OptionalJournalEntryTemplate to_template(BAS::MDJournalEntry const& mdje);
// OptionalJournalEntryTemplate to_template(BAS::MDJournalEntry const& mdje) {
// 	OptionalJournalEntryTemplate result({mdje.meta.series,mdje});
// 	return result;
// }

OptionalJournalEntryTemplate to_template(BAS::MDTypedJournalEntry const& tme);
// OptionalJournalEntryTemplate to_template(BAS::MDTypedJournalEntry const& tme) {
// 	return to_template(to_md_entry(tme));
// }

BAS::MDJournalEntry to_md_journal_entry(HeadingAmountDateTransEntry const& had,JournalEntryTemplate const& jet);
// BAS::MDJournalEntry to_md_journal_entry(HeadingAmountDateTransEntry const& had,JournalEntryTemplate const& jet) {
// 	BAS::MDJournalEntry result{};
// 	result.meta = {
// 		.series = jet.series()
// 	};
// 	result.defacto.caption = had.heading;
// 	result.defacto.date = had.date;
// 	result.defacto.account_transactions = jet(abs(had.amount)); // Ignore sign to have template apply its sign
// 	return result;
// }

std::ostream& operator<<(std::ostream& os,AccountPostingTemplate const& att);
// std::ostream& operator<<(std::ostream& os,AccountPostingTemplate const& att) {
// 	os << "\n\t" << att.m_at.account_no << " " << att.m_percent;
// 	return os;
// }

std::ostream& operator<<(std::ostream& os,JournalEntryTemplate const& entry);
// std::ostream& operator<<(std::ostream& os,JournalEntryTemplate const& entry) {
// 	os << "template: series " << entry.series();
// 	std::for_each(entry.templates.begin(),entry.templates.end(),[&os](AccountPostingTemplate const& att){
// 		os << "\n\t" << att;
// 	});
// 	return os;
// }

bool had_matches_trans(HeadingAmountDateTransEntry const& had,BAS::anonymous::JournalEntry const& aje);

using AccountsTopologyMap = std::map<std::size_t,std::map<BAS::kind::BASAccountTopology,BAS::TypedMetaEntries>>;
AccountsTopologyMap to_accounts_topology_map(BAS::TypedMetaEntries const& tmes);

using Kind2MDTypedJournalEntriesMap = std::map<BAS::kind::AccountPostingTypeTopology,std::vector<BAS::MDTypedJournalEntry>>; // AccountPostingTypeTopology -> TypedMetaEntry
using Kind2MDTypedJournalEntriesCAS = std::map<std::size_t,Kind2MDTypedJournalEntriesMap>; // hash -> TypeMetaEntry
// TODO: Consider to make Kind2MDTypedJournalEntriesCAS an unordered_map (as it is already a map from hash -> TypedMetaEntry)
//       All we should have to do is to define std::hash for this type to make std::unordered_map find it?

Kind2MDTypedJournalEntriesCAS to_meta_entry_topology_map(SIEArchive const& sie_archive);

using HADMatchesJEPredicate = std::function<bool(BAS::anonymous::JournalEntry)>;

BAS::TypedMetaEntries all_years_template_candidates(
   SIEArchive const& sie_archive
  ,HADMatchesJEPredicate const& matches);

OptionalJournalEntryTemplate template_of(OptionalHeadingAmountDateTransEntry const& had,SIEDocument const& sie_doc);

// TYPED ENTRY operator<<

std::ostream& operator<<(std::ostream& os,BAS::kind::BASAccountTopology const& accounts);
std::ostream& operator<<(std::ostream& os,BAS::kind::AccountPostingTypeTopology const& props);
std::ostream& operator<<(std::ostream& os,BAS::anonymous::TypedAccountPosting const& tat);

template <typename T>
struct IndentedOnNewLine{
	IndentedOnNewLine(T const& val,int count) : val{val},count{count} {}
	T const& val;
	int count;
};

std::ostream& operator<<(std::ostream& os,IndentedOnNewLine<BAS::anonymous::TypedAccountPostings> const& indented);
std::ostream& operator<<(std::ostream& os,BAS::anonymous::TypedJournalEntry const& tje);
std::ostream& operator<<(std::ostream& os,BAS::MDTypedJournalEntry const& tme);

// A typed sub-meta-entry is a subset of transactions of provided typed meta entry
// that are all of the same "type" and that all sums to zero (do balance)
std::vector<BAS::MDTypedJournalEntry> to_typed_sub_meta_entries(BAS::MDTypedJournalEntry const& tme);
BAS::anonymous::TypedAccountPostings to_alternative_tats(SIEArchive const& sie_archive,BAS::anonymous::TypedAccountPosting const& tat);
bool operator==(BAS::MDTypedJournalEntry const& tme1,BAS::MDTypedJournalEntry const& tme2);
BAS::MDTypedJournalEntry to_tats_swapped_tme(BAS::MDTypedJournalEntry const& tme,BAS::anonymous::TypedAccountPosting const& target_tat,BAS::anonymous::TypedAccountPosting const& new_tat);
BAS::OptionalMDJournalEntry to_meta_entry_candidate(BAS::MDTypedJournalEntry const& tme,Amount const& gross_amount);

struct TestResult {
	std::ostringstream prompt{"null"};
	bool failed{true};
};

std::ostream& operator<<(std::ostream& os,TestResult const& tr);
TestResult test_typed_meta_entry(SIEArchive const& sie_archive,BAS::MDTypedJournalEntry const& tme);

