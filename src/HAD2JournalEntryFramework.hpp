#pragma once
#include "fiscal/BASFramework.hpp" // BAS::anonymous::JournalEntry,...
#include "fiscal/amount/HADFramework.hpp" // HeadingAmountDateTransEntry,...
#include "sie/SIEDocumentFramework.hpp" // SIEArchive,...

namespace BAS {
	namespace anonymous {
		using AccountPostingTags = std::set<std::string>;
		using AccountPostingsTags = std::map<BAS::anonymous::AccountPosting,AccountPostingTags>;
		using AccountPostingsTagsEntry = AccountPostingsTags::value_type;
		using PostingTagsJournalEntry = BAS::anonymous::JournalEntry_t<AccountPostingsTags>;
	} // anonymous

	using MDPostingTagsJournalEntry = MetaDefacto<BAS::WeakJournalEntryMeta,anonymous::PostingTagsJournalEntry>;
	using TypedMetaEntries = std::vector<MDPostingTagsJournalEntry>;

  void for_each_posting_entry(BAS::MDPostingTagsJournalEntry const& mdtje,auto& f) {
    for (auto const& apte : mdtje.defacto.account_postings) {
      f(apte);
    }
	}

	namespace kind {

		using BASAccountsTopology = std::set<BAS::AccountNo>;
    using AccountPostingKindTag = std::string;
		using AccountPostingKindTags = std::set<AccountPostingKindTag>;

		enum class AccountPostingKind {
			// NOTE: Restrict to 16 values (Or to_posting_kind_tags_rank stops being reliable)
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

    AccountPostingKind to_posting_kind(AccountPostingKindTag const& kind_tag);
    std::size_t to_posting_kind_tags_rank(BAS::kind::AccountPostingKindTags const& kind_tags);
		std::vector<std::string> sorted(AccountPostingKindTags const& kind_tags);

		namespace detail {
			template <typename T>
			struct hash {};

			template <>
			struct hash<BASAccountsTopology> {
				std::size_t operator()(BASAccountsTopology const& bat) {
					std::size_t result{};
					for (auto const& account_no : bat) {
						auto h = std::hash<BAS::AccountNo>{}(account_no);
						result = result ^ (h << 1);
					}
					return result;
				}
			};

			template <>
			struct hash<AccountPostingKindTags> {
				std::size_t operator()(AccountPostingKindTags const& kind_tags) {
					std::size_t result{};
					for (auto const& kind_tag : kind_tags) {
						auto h = std::hash<std::string>{}(kind_tag);
						result = result ^ static_cast<std::size_t>((h << 1));
					}
					return result;
				}
			};
		} // namespace detail

		BASAccountsTopology to_accounts_topology(MDJournalEntry const& mdje);
		BASAccountsTopology to_accounts_topology(MDPostingTagsJournalEntry const& tme);
		AccountPostingKindTags to_posting_kind_tags(MDPostingTagsJournalEntry const& tme);
		std::size_t to_signature(BASAccountsTopology const& bat);
		std::size_t to_signature(AccountPostingKindTags const& met);

	} // namespace kind

} // BAS

BAS::MDPostingTagsJournalEntry to_template_candidate_entry(BAS::MDJournalEntry const& mdje);

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
JournalEntryVATType to_vat_type(BAS::MDPostingTagsJournalEntry const& tme);

using TypedMDJournalEntryVisitorF = std::function<void(BAS::MDPostingTagsJournalEntry const&)>;
void for_each_template_candidate_entry(SIEArchive const& sie_archive,TypedMDJournalEntryVisitorF const& f);

// ==================================================================================
// Had -> journal_entry -> Template

BAS::MDJournalEntry to_md_entry(BAS::MDPostingTagsJournalEntry const& tme);

class AccountPostingTemplate {
public:
	AccountPostingTemplate(Amount gross_amount,BAS::anonymous::AccountPosting const& ap)
		:  m_ap{ap}
			,m_percent{static_cast<int>(round(ap.amount*100 / gross_amount))}  {}
	BAS::anonymous::AccountPosting operator()(Amount amount) const {
		BAS::anonymous::AccountPosting result{
			 .account_no = m_ap.account_no
			,.transtext = m_ap.transtext
			,.amount=static_cast<Amount>(round(amount*m_percent)/100.0)};
		return result;
	}
	int percent() const {return m_percent;}
private:
	BAS::anonymous::AccountPosting m_ap;
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
				std::transform(
           mdje.defacto.account_postings.begin()
          ,mdje.defacto.account_postings.end()
          ,std::back_inserter(templates)
          ,[gross_amount](BAS::anonymous::AccountPosting const& ap){
            AccountPostingTemplate result{gross_amount,ap};
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

OptionalJournalEntryTemplate to_template(BAS::MDPostingTagsJournalEntry const& tme);
// OptionalJournalEntryTemplate to_template(BAS::MDPostingTagsJournalEntry const& tme) {
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
// 	result.defacto.account_postings = jet(abs(had.amount)); // Ignore sign to have template apply its sign
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

using AccountsTopologyMap = std::map<std::size_t,std::map<BAS::kind::BASAccountsTopology,BAS::TypedMetaEntries>>;
AccountsTopologyMap to_accounts_topology_map(BAS::TypedMetaEntries const& tmes);

using Kind2MDTypedJournalEntriesMap = std::map<BAS::kind::AccountPostingKindTags,std::vector<BAS::MDPostingTagsJournalEntry>>; // AccountPostingKindTags -> TypedMetaEntry
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

std::ostream& operator<<(std::ostream& os,BAS::kind::BASAccountsTopology const& accounts);
std::ostream& operator<<(std::ostream& os,BAS::kind::AccountPostingKindTags const& kind_tags);
std::ostream& operator<<(std::ostream& os,BAS::anonymous::AccountPostingsTagsEntry const& apte);

template <typename T>
struct IndentedOnNewLine{
	IndentedOnNewLine(T const& val,int count) : val{val},count{count} {}
	T const& val;
	int count;
};

std::ostream& operator<<(std::ostream& os,IndentedOnNewLine<BAS::anonymous::AccountPostingsTags> const& indented);
std::ostream& operator<<(std::ostream& os,BAS::anonymous::PostingTagsJournalEntry const& tje);
std::ostream& operator<<(std::ostream& os,BAS::MDPostingTagsJournalEntry const& tme);

// A typed sub-meta-entry is a subset of transactions of provided typed meta entry
// that are all of the same "type" and that all sums to zero (do balance)
std::vector<BAS::MDPostingTagsJournalEntry> to_typed_sub_meta_entries(BAS::MDPostingTagsJournalEntry const& tme);
BAS::anonymous::AccountPostingsTags to_alternative_posting_tags(SIEArchive const& sie_archive,BAS::anonymous::AccountPostingsTagsEntry const& apte);
bool operator==(BAS::MDPostingTagsJournalEntry const& tme1,BAS::MDPostingTagsJournalEntry const& tme2);
BAS::MDPostingTagsJournalEntry to_tats_swapped_tme(BAS::MDPostingTagsJournalEntry const& tme,BAS::anonymous::AccountPostingsTagsEntry const& target_apte,BAS::anonymous::AccountPostingsTagsEntry const& new_apte);
BAS::OptionalMDJournalEntry to_meta_entry_candidate(BAS::MDPostingTagsJournalEntry const& tme,Amount const& gross_amount);

struct TestResult {
	std::ostringstream prompt{"null"};
	bool failed{true};
};

std::ostream& operator<<(std::ostream& os,TestResult const& tr);
TestResult test_typed_meta_entry(SIEArchive const& sie_archive,BAS::MDPostingTagsJournalEntry const& tme);

