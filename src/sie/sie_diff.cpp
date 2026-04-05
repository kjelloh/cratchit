#include "sie_diff.hpp"
#include "hash_combine.hpp"
#include <format>

std::size_t hash_of_id_and_all_content(BAS::MDJournalEntry const& mdje) {
  std::size_t result{};
  auto const& [meta,defacto] = mdje;
  // meta: WeakJournalEntryMeta
  /*
	struct WeakJournalEntryMeta {
    // 'Weak' because optional verno
		Series series;
		OptionalVerNo verno;
		std::optional<bool> unposted_flag{};
		bool operator==(WeakJournalEntryMeta const& other) const = default;
	};

  */

  // meta
  hash_combine(result,meta.series);
  if (meta.verno) hash_combine(result,*meta.verno);
  hash_combine(result,meta.unposted_flag);

  //defacto : BAS::anonymous::JournalEntry
  /*
		struct JournalEntry_t {
			std::string caption{};
			Date date{};
			T account_transactions{};

  */
  
  //defacto
  hash_combine(result,defacto.caption);
  hash_combine(result,std::format("{}",defacto.date)); // Works in absence of std::hash<std::chrono::year_month_day>
  for (auto const& at : defacto.account_transactions) {
		// struct AccountPosting {
		// 	BAS::AccountNo account_no;
		// 	std::optional<std::string> transtext{};
		// 	Amount amount;
    hash_combine(result,at.account_no);
    hash_combine(result,at.transtext);
    hash_combine(result,to_cents_amount(at.amount));
  }
  return result;
}




