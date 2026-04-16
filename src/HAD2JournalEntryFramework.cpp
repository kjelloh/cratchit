#include "HAD2JournalEntryFramework.hpp"
#include "text/functional.hpp" // strings_share_tokens,...
#include <numeric> // std::accumulate,...

namespace BAS {
  namespace kind {
    AccountPostingKind to_posting_kind(AccountPostingKindTag const& kind_tag) {
			AccountPostingKind result{AccountPostingKind::undefined};
			static const std::map<std::string,AccountPostingKind> KIND_TAG_2_POSTING_KIND{
				 {"",AccountPostingKind::undefined}
				,{"transfer",AccountPostingKind::transfer}
				,{"eu_purchase",AccountPostingKind::eu_purchase}
				,{"gross",AccountPostingKind::gross}
				,{"net",AccountPostingKind::net}
				,{"eu_vat",AccountPostingKind::eu_vat}
				,{"vat",AccountPostingKind::vat}
				,{"cents",AccountPostingKind::cents}
			};
			if (KIND_TAG_2_POSTING_KIND.contains(kind_tag)) {
				result = KIND_TAG_2_POSTING_KIND.at(kind_tag);
			}
			else {
				result = AccountPostingKind::unknown;
			}
			return result;
		}

    std::size_t to_posting_kind_tags_rank(BAS::kind::AccountPostingKindTags const& kind_tags) {
			std::size_t result{};
			std::vector<AccountPostingKind> posting_kinds{};
			for (auto const& kind_tag : kind_tags) posting_kinds.push_back(to_posting_kind(kind_tag));
			std::sort(
         posting_kinds.begin()
        ,posting_kinds.end()
        ,[](AccountPostingKind lhs,AccountPostingKind rhs){
				  return (lhs<rhs);
			});
			// Assemble a "number" of "digits" each having value 0..15 (i.e, 
      // in effect a hexadecimal number with each digit indicating an posting_kind enum value)
			for (auto posting_kind : posting_kinds) result = result*0x10 + static_cast<std::size_t>(posting_kind);
			return result;
		}

		 std::vector<std::string> sorted(AccountPostingKindTags const& kind_tags) {
			std::vector<std::string> result{kind_tags.begin(),kind_tags.end()};
			std::sort(result.begin(),result.end(),[](auto const& s1,auto const& s2){
				return (to_posting_kind(s1) < to_posting_kind(s2));
			});
			return result;
		}

		BASAccountsTopology to_accounts_topology(MDJournalEntry const& mdje) {
			BASAccountsTopology result{};
			auto f = [&result](BAS::anonymous::AccountPosting const& ap) {
				result.insert(ap.account_no);
			};
			for_each_anonymous_account_transaction(mdje.defacto,f);
			return result;
		}

		BASAccountsTopology to_accounts_topology(MDTaggedPostingsJournalEntry const& md_tpje) {
			BASAccountsTopology result{};
			auto f = [&result](BAS::anonymous::AccountPostingTagsPair const& ap_tags_pair) {
				auto const& [ap,kind_tags] = ap_tags_pair;
				result.insert(ap.account_no);
			};
			for_each_posting_entry(md_tpje,f);
			return result;
		}

		AccountPostingKindTags to_posting_kind_tags(MDTaggedPostingsJournalEntry const& md_tpje) {
			AccountPostingKindTags result{};
			auto f = [&result](BAS::anonymous::AccountPostingTagsPair const& ap_tags_pair) {
				auto const& [ap,kind_tags] = ap_tags_pair;
				for (auto const& kind_tag : kind_tags) result.insert(kind_tag);
			};
			for_each_posting_entry(md_tpje,f);
			return result;
		}

		std::size_t to_signature(BASAccountsTopology const& bat) {
			return detail::hash<BASAccountsTopology>{}(bat);
		}

		std::size_t to_signature(AccountPostingKindTags const& kind_tags) {
			return detail::hash<AccountPostingKindTags>{}(kind_tags);
		}

  } // kind
} // BAS

BAS::MDJournalEntry to_md_entry(BAS::MDTaggedPostingsJournalEntry const& md_tpje) {
	BAS::MDJournalEntry result {
		.meta = md_tpje.meta
		,.defacto = {
			.caption = md_tpje.defacto.caption
			,.date = md_tpje.defacto.date
		}
	};
	for (auto const& [ap,kind_tags] : md_tpje.defacto.account_postings) {
		result.defacto.account_postings.push_back(ap);
	}
	return result;
}

// ----------------------------------------
// MDJournalEntry -> TYPED MDJournalEntry

BAS::MDTaggedPostingsJournalEntry to_template_candidate_entry(BAS::MDJournalEntry const& mdje) {
	// std::cout << "\nto_typed_meta_entry: " << me;
	BAS::anonymous::AccountPostingsTags posting_tags{};

  /*
  use the following tagging

  "gross" for the transaction with the whole transaction amount (balances debit and credit to one account)
  "vat" for a vat part amount
  "net" for ex VAT amount
  "transfer" For amount just "passing though" and account (a debit and credit in the same transaction = ending up being half the gross amount on each balancing side)
  "counter" (for non VAT amount that counters (balances) the gross amount)
  */

	if (auto optional_gross_amount = to_gross_transaction_amount(mdje.defacto)) {
		auto gross_amount = *optional_gross_amount;
		// Direct type detection based on gross_amount and account meta data
		for (auto const& ap : mdje.defacto.account_postings) {
			if (round(abs(ap.amount)) == round(gross_amount)) posting_tags[ap].insert("gross");
			if (is_vat_account_at(ap)) posting_tags[ap].insert("vat");
			if (abs(ap.amount) < 1) posting_tags[ap].insert("cents");
			if (round(abs(ap.amount)) == round(gross_amount / 2)) posting_tags[ap].insert("transfer"); // 20240519 I no longer understand this? A transfer if half the gross? Strange?
		}

		// Ex vat amount Detection
		Amount ex_vat_amount{},vat_amount{};
		for (auto const& ap : mdje.defacto.account_postings) {
			if (!posting_tags.contains(ap)) {
				// Not gross, Not VAT (above) => candidate for ex VAT
				ex_vat_amount += ap.amount;
			}
			else if (posting_tags.at(ap).contains("vat")) {
				vat_amount += ap.amount;
			}
		}
    std::string net_or_counter_tag = (vat_amount != 0)?std::string{"net"}:std::string{"counter"};
		if (abs(round(abs(ex_vat_amount)) + round(abs(vat_amount)) - gross_amount) <= 1) {
			// ex_vat + vat within cents of gross
			// tag non typed ats as ex-vat
			for (auto const& ap : mdje.defacto.account_postings) {
				if (!posting_tags.contains(ap)) {
					posting_tags[ap].insert(net_or_counter_tag);
				}
			}
		}
	}
	else {
		std::cout << "\nDESIGN INSUFFICIENCY - to_typed_meta_entry failed to determine gross amount";
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
	for (auto const& ap : mdje.defacto.account_postings) {
		// Identify transactions to EU VAT and EU Purchase tagged accounts
		if (is_vat_returns_form_at(SKV::XML::VATReturns::EU_VAT_BOX_NOS,ap)) {
			posting_tags[ap].insert("eu_vat");
			eu_vat_amount = ap.amount;
		}
		if (is_vat_returns_form_at(SKV::XML::VATReturns::EU_PURCHASE_BOX_NOS,ap)) {
			posting_tags[ap].insert("eu_purchase");
			eu_purchase_amount = ap.amount;
		}
	}
	for (auto const& ap : mdje.defacto.account_postings) {
		// Identify counter transactions to EU VAT and EU Purchase tagged accounts
		if (ap.amount == -eu_vat_amount) posting_tags[ap].insert("eu_vat"); // The counter trans for EU VAT
		if ((first_digit(ap.account_no) == 4 or first_digit(ap.account_no) == 9) and (ap.amount == -eu_purchase_amount)) posting_tags[ap].insert("eu_purchase"); // The counter trans for EU Purchase
	}
	// Mark gross accounts for EU VAT transaction journal entry
	for (auto const& ap : mdje.defacto.account_postings) {
		// We expect two accounts left unmarked and they are the gross accounts
		if (!posting_tags.contains(ap) and (abs(ap.amount) == abs(eu_purchase_amount))) {
			posting_tags[ap].insert("gross");
		}
	}

	// Finally add any still untyped at with empty property set
	for (auto const& ap : mdje.defacto.account_postings) {
		if (!posting_tags.contains(ap)) posting_tags.insert({ap,{}});
	}

	BAS::MDTaggedPostingsJournalEntry result{
		.meta = mdje.meta
		,.defacto = {
			.caption = mdje.defacto.caption
			,.date = mdje.defacto.date
			,.account_postings = posting_tags
		}
	};
	return result;
};

// MDJournalEntry -> TYPED MDJournalEntry
// ----------------------------------------

std::ostream& operator<<(std::ostream& os,JournalEntryVATType const& vat_type) {
	switch (vat_type) {
		case JournalEntryVATType::Undefined: {os << "Undefined";} break;
		case JournalEntryVATType::NoVAT: {os << "No VAT";} break;
		case JournalEntryVATType::SwedishVAT: {os << "Swedish VAT";} break;
		case JournalEntryVATType::EUVAT: {os << "EU VAT";} break;
		case JournalEntryVATType::VATReturns: {os << "VAT Returns";} break;
    case JournalEntryVATType::VATClearing: {os << "VAT Returns Clearing";} break; // VAT Returns Cleared by Swedish Skatteverket (SKV)
    case JournalEntryVATType::SKVInterest: {os << "SKV Interest";} break; // SKV applied interest to holding on the SKV account
    case JournalEntryVATType::SKVFee: {os << "SKV Fee";} break; // SKV applied a fee to be paied (e.g., for missing to leave a report before deadline)
		case JournalEntryVATType::VATTransfer: {os << "VAT Transfer";} break;

		case JournalEntryVATType::Unknown: {os << "Unknown";} break;

		default: os << "operator<< failed for vat_type with integer value " << static_cast<int>(vat_type);
	}
	return os;
}

JournalEntryVATType to_vat_type(BAS::MDTaggedPostingsJournalEntry const& md_tpje) {
	JournalEntryVATType result{JournalEntryVATType::Undefined};
	static bool const log{true};
	// Count each type of property (NOTE: Can be less than transaction count as they may overlap, e.g., two or more gross account postings)
	std::map<std::string,unsigned int> props_counter{};
	for (auto const& [ap,kind_tags] : md_tpje.defacto.account_postings) {
		for (auto const& kind_tag : kind_tags) props_counter[kind_tag]++;
	}
	// LOG
	if (log) {
		for (auto const& [kind_tag,count] : props_counter) {
			std::cout << "\n" << std::quoted(kind_tag) << " count:" << count;
		}
	}
	// Calculate total number of properties (NOTE: Can be more that the transactions as e.g., vat and eu_vat overlaps)
	auto props_sum = std::accumulate(props_counter.begin(),props_counter.end(),unsigned{0},[](auto acc,auto const& entry){
		acc += entry.second;
		return acc;
	});
	// Identify what type of VAT the candidate defines
	if ((props_counter.size() == 1) and props_counter.contains("gross")) {
		result = JournalEntryVATType::NoVAT; // NO VAT (All gross i.e., a Debit and credit that is not a VAT and with the same amount)
		if (log) std::cout << "\nTemplate is an NO VAT, all gross amount transaction :)"; // gross,gross
	}
  else if ((props_counter.size() == 2) and props_counter.contains("gross") and props_counter.contains("counter")) {
		result = JournalEntryVATType::NoVAT; // NO VAT and only one gross and more than one counter gross
		if (log) std::cout << "\nTemplate is an NO VAT, gross + counter gross transaction :)"; // gross,counter...
  }
	else if ((props_counter.size() == 3) and props_counter.contains("gross") and props_counter.contains("net") and props_counter.contains("vat") and !props_counter.contains("eu_vat")) {
		if (props_sum == 3) {
			if (log) std::cout << "\nTemplate is a SWEDISH PURCHASE/sale"; // (gross,net,vat);
			result = JournalEntryVATType::SwedishVAT; // Swedish VAT
		}
	}
	else if ((props_counter.size() == 4) and props_counter.contains("gross") and props_counter.contains("net") and props_counter.contains("vat") and props_counter.contains("cents") and !props_counter.contains("eu_vat")) {
		if (props_sum == 4) {
			if (log) std::cout << "\nTemplate is a SWEDISH PURCHASE/sale"; // (gross,net,vat);
			result = JournalEntryVATType::SwedishVAT; // Swedish VAT
		}
	}
	else if (
		(     (props_counter.contains("gross"))
			and (props_counter.contains("eu_purchase"))
			and (props_counter.contains("eu_vat")))) {
		result = JournalEntryVATType::EUVAT; // EU VAT
		if (log) std::cout << "\nTemplate is an EU PURCHASE :)"; // gross,gross,eu_vat,eu_vat,eu_purchase,eu_purchase
	}
	else if (std::all_of(props_counter.begin(),props_counter.end(),[](std::map<std::string,unsigned int>::value_type const& entry){ return (entry.first == "vat") or (entry.first == "eu_vat") or  (entry.first == "cents");})) {
		result = JournalEntryVATType::VATReturns; // All VATS (probably a VAT report)
	}
  else if (md_tpje.defacto.account_postings.size() == 2 and std::all_of(md_tpje.defacto.account_postings.begin(),md_tpje.defacto.account_postings.end(),[](auto const& ap_tags_pair){
      auto const& [ap,kind_tags] = ap_tags_pair;
      return (ap.account_no == 1630 or ap.account_no == 2650 or ap.account_no == 1650); // SKV account updated with VAT, i.e., cleared
	  // 1630 = SKV tax account, 1650 = SKV tax receivable, 2650 = SKV tax payable
    })) {
		result = JournalEntryVATType::VATClearing; // SKV account cleared against 2650
  }
	else if (std::all_of(props_counter.begin(),props_counter.end(),[](std::map<std::string,unsigned int>::value_type const& entry){ return (entry.first == "transfer") or (entry.first == "vat");})) {
		result = JournalEntryVATType::VATTransfer; // All transfer of vat (probably a VAT settlement with Swedish Tax Agency)
	}
  else if (md_tpje.defacto.account_postings.size() == 2 and std::all_of(md_tpje.defacto.account_postings.begin(),md_tpje.defacto.account_postings.end(),[](auto const& ap_tags_pair){
      auto const& [ap,kind_tags] = ap_tags_pair;
      return (ap.account_no == 8314 or ap.account_no == 1630);
    })) {
    // One account 1630 (SKV tax account) and one account 8314 (tax free interest gain)
		result = JournalEntryVATType::SKVInterest; // SKV gained interest
	}
	else if (false) { // TODO 231105: Implement a criteria to identify an SKV Fee event
    // bokförs på konto 6992 övriga ej avdragsgilla kostnader
		result = JournalEntryVATType::SKVFee;
	}
  else if (false) { // TODO 20240516: Implement criteria and type to idendify tax free SKV interest loss
    // (at.account_no == 8423 or at.account_no == 1630);
  }
	else {
		if (log) std::cout << "\nFailed to recognise the VAT type";
	}
	return result;
}

void for_each_template_candidate_entry(SIEArchive const& sie_archive,TypedMDJournalEntryVisitorF const& f) {
	auto f_caller = [&f](BAS::MDJournalEntry const& mdje) {
		auto md_tpje = to_template_candidate_entry(mdje);
		f(md_tpje);
	};
	for_each_md_journal_entry(sie_archive,f_caller);
}

// TYPED -> TEMPLATE

OptionalJournalEntryTemplate to_template(BAS::MDJournalEntry const& mdje) {
	OptionalJournalEntryTemplate result({mdje.meta.series,mdje});
	return result;
}

OptionalJournalEntryTemplate to_template(BAS::MDTaggedPostingsJournalEntry const& md_tpje) {
	return to_template(to_md_entry(md_tpje));
}

BAS::MDJournalEntry to_md_journal_entry(HeadingAmountDateTransEntry const& had,JournalEntryTemplate const& jet) {
	BAS::MDJournalEntry result{};
	result.meta = {
		.series = jet.series()
	};
	result.defacto.caption = had.heading;
	result.defacto.date = had.date;
	result.defacto.account_postings = jet(abs(had.amount)); // Ignore sign to have template apply its sign
	return result;
}

std::ostream& operator<<(std::ostream& os,AccountPostingTemplate const& att) {
	os << "\n\t" << att.m_ap.account_no << " " << att.m_percent;
	return os;
}

std::ostream& operator<<(std::ostream& os,JournalEntryTemplate const& entry) {
	os << "template: series " << entry.series();
	std::for_each(entry.templates.begin(),entry.templates.end(),[&os](AccountPostingTemplate const& att){
		os << "\n\t" << att;
	});
	return os;
}

bool had_matches_trans(HeadingAmountDateTransEntry const& had,BAS::anonymous::JournalEntry const& aje) {
	return text::functional::strings_share_tokens(had.heading,aje.caption);
}

AccountsTopologyMap to_accounts_topology_map(BAS::TaggedPostingsMDJournalEntries const& tp_md_jes) {
	AccountsTopologyMap result{};
	auto g = [&result](BAS::MDTaggedPostingsJournalEntry const& md_tpje) {
		auto accounts_topology = BAS::kind::to_accounts_topology(md_tpje);
		auto signature = BAS::kind::to_signature(accounts_topology);
		result[signature][accounts_topology].push_back(md_tpje);
	};
	std::for_each(tp_md_jes.begin(),tp_md_jes.end(),g);
	return result;
}

Kind2MDTypedJournalEntriesCAS to_meta_entry_topology_map(SIEArchive const& sie_archive) {
	Kind2MDTypedJournalEntriesCAS result{};
	// Group on Type Topology
	Kind2MDTypedJournalEntriesCAS meta_entry_topology_map{};
	auto h = [&result](BAS::MDTaggedPostingsJournalEntry const& md_tpje){
		auto types_topology = BAS::kind::to_posting_kind_tags(md_tpje);
		auto signature = BAS::kind::to_signature(types_topology);
		result[signature][types_topology].push_back(md_tpje);
	};
	for_each_template_candidate_entry(sie_archive,h);
	return result;
}

BAS::TaggedPostingsMDJournalEntries all_years_template_candidates(
   SIEArchive const& sie_archive
  ,HADMatchesJEPredicate const& matches) {
  BAS::TaggedPostingsMDJournalEntries result{};
  auto meta_entry_topology_map = to_meta_entry_topology_map(sie_archive);
  for (auto const& [signature,tme_map] : meta_entry_topology_map) {
    for (auto const& [kind_tags,tp_md_jes] : tme_map) {
      auto accounts_topology_map = to_accounts_topology_map(tp_md_jes);
      for (auto const& [signature,bat_map] : accounts_topology_map) {
        for (auto const& [kind_tags,tp_md_jes] : bat_map) {
          for (auto const& md_tpje : tp_md_jes) {
            auto mdje = to_md_entry(md_tpje);
            if (matches(mdje.defacto)) result.push_back(md_tpje);
          }
        }
      }
    }
  }
  return result;
}

OptionalJournalEntryTemplate template_of(OptionalHeadingAmountDateTransEntry const& had,SIEDocument const& sie_doc) {
	OptionalJournalEntryTemplate result{};
	if (had) {
		BAS::MDJournalEntries candidates{};
		for (auto const& je : sie_doc.journals()) {
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

// TYPED ENTRY operator<<

std::ostream& operator<<(std::ostream& os,BAS::kind::BASAccountsTopology const& accounts) {
	if (accounts.size()==0) os << " ?";
	else for (auto const& account : accounts) {
		os << " " << account;
	}
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::kind::AccountPostingKindTags const& kind_tags) {
	auto sorted_props = BAS::kind::sorted(kind_tags); // kind_tags is a std::set, sorted_props is a vector
	if (sorted_props.size()==0) os << " ?";
	else for (auto const& kind_tag : sorted_props) {
		os << " " << kind_tag;
	}
	os << " = sort_code: 0x" << std::hex << BAS::kind::to_posting_kind_tags_rank(kind_tags) << std::dec;
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::anonymous::AccountPostingTagsPair const& ap_tags_pair) {
	auto const& [ap,kind_tags] = ap_tags_pair;
	os << kind_tags << " : " << ap;
	return os;
}

std::ostream& operator<<(std::ostream& os,IndentedOnNewLine<BAS::anonymous::AccountPostingsTags> const& indented) {
	for (auto const& ap : indented.val) {
		os << "\n";
		for (int x = 0; x < indented.count; ++x) os << ' ';
		os << ap;
	}
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::anonymous::TaggedPostingsJournalEntry const& tpje) {
	os << std::quoted(tpje.caption) << " " << tpje.date;
	for (auto const& ap_tags_pair : tpje.account_postings) {
		os << "\n\t" << ap_tags_pair;
	}
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::MDTaggedPostingsJournalEntry const& md_tpje) {
	os << md_tpje.meta << " " << md_tpje.defacto;
	return os;
}

// A typed sub-meta-entry is a subset of transactions of provided typed meta entry
// that are all of the same "type" and that all sums to zero (do balance)
std::vector<BAS::MDTaggedPostingsJournalEntry> to_typed_sub_meta_entries(BAS::MDTaggedPostingsJournalEntry const& md_tpje) {
	std::vector<BAS::MDTaggedPostingsJournalEntry> result{};
	// TODO: When needed, identify sub-entries of typed account postings that balance (sums to zero)
	result.push_back(md_tpje); // For now, return input as the single sub-entry
	return result;
}

BAS::anonymous::AccountPostingsTags to_alternative_posting_tags(SIEArchive const& sie_archive,BAS::anonymous::AccountPostingTagsPair const& ap_tags_pair) {
	BAS::anonymous::AccountPostingsTags result{};
	result.insert(ap_tags_pair); // For now, return ourself as the only alternative
	return result;
}

 bool operator==(BAS::MDTaggedPostingsJournalEntry const& lhs,BAS::MDTaggedPostingsJournalEntry const& rhs) {
	return (BAS::kind::to_posting_kind_tags(lhs) == BAS::kind::to_posting_kind_tags(rhs));
}

BAS::MDTaggedPostingsJournalEntry to_swapped_ap_tags_pair_md_tpje(BAS::MDTaggedPostingsJournalEntry const& md_tpje,BAS::anonymous::AccountPostingTagsPair const& target_ap_tags_pair,BAS::anonymous::AccountPostingTagsPair const& new_ap_tags_pair) {
	BAS::MDTaggedPostingsJournalEntry result{md_tpje};
	// TODO: Implement actual swap of posting tags pairs
	return result;
}

BAS::OptionalMDJournalEntry to_meta_entry_candidate(BAS::MDTaggedPostingsJournalEntry const& md_tpje,Amount const& gross_amount) {
	BAS::OptionalMDJournalEntry result{};
	// TODO: Implement actual generation of a candidate using the provided typed meta entry and the gross amount
	auto order_code = BAS::kind::to_posting_kind_tags_rank(BAS::kind::to_posting_kind_tags(md_tpje));
	BAS::MDJournalEntry mdje_candidate{
		.meta = md_tpje.meta
		,.defacto = {
			.caption = md_tpje.defacto.caption
			,.date = md_tpje.defacto.date
			,.account_postings = {}
		}
	};
	switch (order_code) {
		// <DETECTED TOPOLOGIES>
		// 	 eu_vat vat cents = sort_code: 0x567

		// 	 gross net vat = sort_code: 0x346
		case 0x346:
		// 	 gross net vat cents = sort_code: 0x3467
		case 0x3467: {
			// With Swedish VAT (with rounding)
			if (md_tpje.defacto.account_postings.size() == 3 or md_tpje.defacto.account_postings.size() == 4) {
				// One gross account + single counter {net,vat} and single rounding trans
				for (auto const& ap_tags_pair : md_tpje.defacto.account_postings) {
					switch (BAS::kind::to_posting_kind_tags_rank(ap_tags_pair.second)) {
						case 0x3: {
							// gross
							mdje_candidate.defacto.account_postings.push_back({
								.account_no = ap_tags_pair.first.account_no
								,.transtext = ap_tags_pair.first.transtext
								,.amount = gross_amount
							});
						}; break;
						case 0x4: {
							// net
							mdje_candidate.defacto.account_postings.push_back({
								.account_no = ap_tags_pair.first.account_no
								,.transtext = ap_tags_pair.first.transtext
								,.amount = static_cast<Amount>(gross_amount*0.8) // NOTE: Hard coded 25% VAT
							});
						}; break;
						case 0x6: {
							// VAT
							mdje_candidate.defacto.account_postings.push_back({
								.account_no = ap_tags_pair.first.account_no
								,.transtext = ap_tags_pair.first.transtext
								,.amount = static_cast<Amount>(gross_amount*0.2) // NOTE: Hard coded 25% VAT
							});
						}; break;
						case 0x7: {
							// Cents
							mdje_candidate.defacto.account_postings.push_back({
								.account_no = ap_tags_pair.first.account_no
								,.transtext = ap_tags_pair.first.transtext
								,.amount = static_cast<Amount>(0.0) // NOTE: No rounding here
								// NOTE: Applying a rounding scheme has to "guess" what to aim for.
								//       It seems some sellers aim at making the gross amount without cents.
								//       But I have seen Telia invoices with rounding although both gross and net amounts
								//       are with cents (go figure how that come?)
							});
						}; break;
					}
				}
				result = mdje_candidate;
			}
			else {
				// Multipple counter transaction aggretates not yet supported
			}
		} break;

		// 	 transfer gross cents = sort_code: 0x137
		// 	 gross vat cents = sort_code: 0x367
		// 	 vat cents = sort_code: 0x67

		// 	 eu_purchase gross eu_vat vat = sort_code: 0x2356
		case 0x2356: {
			// With EU VAT
			if (md_tpje.defacto.account_postings.size() == 6) {
				// One gross + one counter gross trasnaction (EU Transactions between countries happens without charging the buyer with VAT)
				// But to populate the VAT Returns form we need four more "fake" transactions
				// One "fake" EU VAT + a counter "fake" VAT transactions (zero VAT to pay for the buyer)
				// One "fake" EU Purchase + a counter EU Purchase (to not duble book the purchase in the buyers journal)
				for (auto const& ap_tags_pair : md_tpje.defacto.account_postings) {
					switch (BAS::kind::to_posting_kind_tags_rank(ap_tags_pair.second)) {
						case 0x2: {
							// eu_purchase +/-
							mdje_candidate.defacto.account_postings.push_back({
								.account_no = ap_tags_pair.first.account_no
								,.transtext = ap_tags_pair.first.transtext
								,.amount = (ap_tags_pair.first.amount<0)?-abs(gross_amount):abs(gross_amount)
							});
						} break;
						case 0x3: {
							// gross +/-
							mdje_candidate.defacto.account_postings.push_back({
								.account_no = ap_tags_pair.first.account_no
								,.transtext = ap_tags_pair.first.transtext
								,.amount = (ap_tags_pair.first.amount<0)?-abs(gross_amount):abs(gross_amount)
							});
						} break;
						case 0x5: {
							// eu_vat +/-
							auto vat_amount = static_cast<Amount>(((ap_tags_pair.first.amount<0)?-1.0:1.0) * 0.2 * abs(gross_amount));
							mdje_candidate.defacto.account_postings.push_back({
								.account_no = ap_tags_pair.first.account_no
								,.transtext = ap_tags_pair.first.transtext
								,.amount = vat_amount
							});
						} break;
						// NOTE: case 0x6: vat will hit the same transaction as the eu_vat tagged account trasnactiopn is also tagged vat ;)
					} // switch
				} // for ats
				result = mdje_candidate;
			}

		} break;

		// 	 gross = sort_code: 0x3
		case 0x3: {
			// With gross, counter gross
			if (md_tpje.defacto.account_postings.size() == 2) {
				for (auto const& ap_tags_pair : md_tpje.defacto.account_postings) {
					switch (BAS::kind::to_posting_kind_tags_rank(ap_tags_pair.second)) {
						case 0x3: {
							// gross +/-
							mdje_candidate.defacto.account_postings.push_back({
								.account_no = ap_tags_pair.first.account_no
								,.transtext = ap_tags_pair.first.transtext
								,.amount = (ap_tags_pair.first.amount<0)?-abs(gross_amount):abs(gross_amount)
							});
						}; break;
					} // switch
				} // for ats
				result = mdje_candidate;
			}
			else {
				// Multipple gounter gross accounts not yet supportered
			}
		}

		// 	 gross net = sort_code: 0x34

		// 	 gross vat = sort_code: 0x36
		// 	 transfer = sort_code: 0x1
		// 	 transfer vat = sort_code: 0x16
	} // switch
	// result = to_meta_entry(md_tpje); // Return with unmodified amounts!
	return result;
}

std::ostream& operator<<(std::ostream& os,TestResult const& tr) {
	os << tr.prompt.str();
	return os;
}

TestResult test_typed_meta_entry(SIEArchive const& sie_archive,BAS::MDTaggedPostingsJournalEntry const& md_tpje) {
	TestResult result{};
	result.prompt << "test_typed_meta_entry=";
	auto sub_tmes = to_typed_sub_meta_entries(md_tpje);
	for (auto const& sub_tme : sub_tmes) {
		for (auto const& ap_tags_pair : sub_tme.defacto.account_postings) {
			auto alt_tats = to_alternative_posting_tags(sie_archive,ap_tags_pair);
			for (auto const& alt_tat : alt_tats) {
				auto alt_tme = to_swapped_ap_tags_pair_md_tpje(md_tpje,ap_tags_pair,alt_tat);
				result.prompt << "\n\t\t" <<  "Swapped " << ap_tags_pair << " with " << alt_tat;
				// Test that we can do a roundtrip and get the alt_tme back
				auto gross_amount = std::accumulate(alt_tme.defacto.account_postings.begin(),alt_tme.defacto.account_postings.end(),Amount{0},[](auto acc, auto const& ap_tags_pair){
					if (ap_tags_pair.first.amount > 0) acc += ap_tags_pair.first.amount;
					return acc;
				});
				auto raw_alt_candidate = to_md_entry(alt_tme); // Raw conversion
				auto alt_candidate = to_meta_entry_candidate(alt_tme,gross_amount); // Generate from gross amount
				if (alt_candidate and are_same_and_less_than_100_cents_apart(*alt_candidate,raw_alt_candidate)) {
					result.prompt << "\n\t\t" << "Success, are less that 100 cents apart :)!";
					result.prompt << "\n\t\t      raw: " << raw_alt_candidate;
					result.prompt << "\n\t\tgenerated: " << *alt_candidate;
					result.failed = false;
				}
				else {
					result.prompt << "\n\t\t" << "FAILED, ARE NOT SAME OR SAME BUT MORE THAN 100 CENTS APART";
					result.prompt << "\n\t\t      raw: " << raw_alt_candidate;
					if (alt_candidate) result.prompt << "\n\t\tgenerated: " << *alt_candidate;
					else result.prompt << "\n\t\tgenerated: " << " null";
				}
			}
		}
	}
	result.prompt << "\n\t\t" << " *DONE*";
	return result;
}
