#pragma once

#include "sie.hpp"
#include "logger/log.hpp"

struct DatedJournalEntryMeta {
  Date m_date;
  BAS::WeakJournalEntryMeta m_jem;
};

class SIEEnvironmentChangeResult {
public:
  enum class Status {
     Unknown
    ,NowPosted
    ,StagedOk
    ,SameValueAssigned
    ,Undefined
  };
  SIEEnvironmentChangeResult() = delete;
  SIEEnvironmentChangeResult(BAS::MDJournalEntry const& mdje,Status status = Status{});
  SIEEnvironmentChangeResult with_status(Status status) const;
  bool now_posted() const;
  operator bool() const;
  BAS::MDJournalEntry const& md_entry() const;
private:
  Status m_status{};
  BAS::MDJournalEntry m_md_entry; // Note: SIEEnvironment does not store actual BAS::MetaEntry.
                                  //       So we store a ref-safe clone to return as result
                                  //       This may e.g., allow for returning a mutated entry.
}; // SIEEnvironmentChangeResult

using SIEEnvironmentChangeResults = std::vector<SIEEnvironmentChangeResult>;

using DatedJournalEntryMetas = std::vector<DatedJournalEntryMeta>;

class SIEEnvironment {

  // MetaEntry Is a meta-defacto representation of a 'journal entry' in a DAG

  // WeakJournalEntryMeta: Record {Series:series, OptionalVerNo:verno}
  // MetaEntry: pair {WeakJournalEntryMeta:meta , JournalEntry:defacto}

  // SIEEnvironment store 'defacto' entries in a DAG <series> -> <verno> -> JournalEntry

  // AccountTransaction: Record {AccountNo:account_no, optional::string:transtext{}, Amount:amount}
  // AccountTransactions: vector AccountTransaction
  // JournalEntry: Record {string:caption, Date:date, AccountTransactions:account_transactions}
  // BASJournal: Map BAS::VerNo -> BAS::anonymous::JournalEntry (E.g., 7 -> JournalEntry) 
  // BASJournals: Map BASJournalId -> BASJournal E.g., 'A' -> BASJournal

public:

  SIEEnvironment(FiscalYear const& fiscal_year);
  SIEEnvironment() = delete;
  SIEEnvironment& operator=(SIEEnvironment const& other) = default;

  MaybeBASJournalRef at(BAS::Series series);
  BAS::MaybeJournalEntryRef at(DatedJournalEntryMeta key);
	BAS::MDJournalEntries unposted() const;

	SIEEnvironmentChangeResult post_(BAS::MDJournalEntry const& mdje);
	SIEEnvironmentChangeResult stage_entry_(BAS::MDJournalEntry const& mdje);
	SIEEnvironmentChangeResults stage_sie_(SIEEnvironment const& staged_sie_environment);
	SIEEnvironmentChangeResult add_(BAS::MDJournalEntry mdje);
	SIEEnvironmentChangeResult update_(BAS::MDJournalEntry const& mdje);

	BAS::VerNo largest_verno(BAS::Series series);
	bool already_in_posted(BAS::MDJournalEntry const& mdje);
	bool is_unposted(BAS::Series series, BAS::VerNo verno) const;

  DatedJournalEntryMetas to_dated_journal_entry_metas() const;
	std::filesystem::path staged_sie_file_path() const;
	SIE::OrgNr organisation_no{};
	SIE::FNamn organisation_name{};
	SIE::Adress organisation_address{};
	BASJournals& journals();
	BASJournals const& journals() const;
	SKV::SRU::OptionalAccountNo sru_code(BAS::AccountNo const& bas_account_no);
	BAS::OptionalAccountNos to_bas_accounts(SKV::SRU::AccountNo const& sru_code) const;
  std::size_t journals_entry_count() const;
	BAS::AccountMetas const&  account_metas() const;

	void set_account_name(BAS::AccountNo bas_account_no ,std::string const& name);
	void set_account_SRU(BAS::AccountNo bas_account_no, SKV::SRU::AccountNo sru_code);
	void set_opening_balance(BAS::AccountNo bas_account_no,Amount opening_balance);

	BalancesMap balances_at(Date date);
	FiscalYear fiscal_year() const;
	zeroth::OptionalDateRange financial_year_date_range() const;
  OptionalAmount opening_balance_of(BAS::AccountNo bas_account_no) const;
  std::map<BAS::AccountNo,Amount> const& opening_balances() const;
	
private:
	BASJournals m_journals{};
	zeroth::OptionalDateRange m_year_date_range{};
	FiscalYear m_fiscal_year;
	std::map<char,BAS::VerNo> verno_of_last_posted_to{};
	std::map<BAS::AccountNo,Amount> opening_balance{};
  friend class SIEEnvironmentsMap;
}; // class SIEEnvironment

using OptionalSIEEnvironment = std::optional<SIEEnvironment>;

std::ostream& operator<<(std::ostream& os,DatedJournalEntryMeta const& djem);

