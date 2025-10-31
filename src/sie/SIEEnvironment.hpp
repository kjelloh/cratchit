#pragma once

#include "sie.hpp"
#include "logger/log.hpp"


class SIEEnvironment {

  // MetaEntry Is a meta-defacto representation of a 'journal entry' in a DAG

  // JournalEntryMeta: Record {Series:series, OptionalVerNo:verno}
  // MetaEntry: pair {JournalEntryMeta:meta , JournalEntry:defacto}

  // SIEEnvironment store 'defacto' entries in a DAG <series> -> <verno> -> JournalEntry

  // AccountTransaction: Record {AccountNo:account_no, optional::string:transtext{}, Amount:amount}
  // AccountTransactions: vetcor AccountTransaction
  // JournalEntry: Record {string:caption, Date:date, AccountTransactions:account_transactions}
  // BASJournal: Map BAS::VerNo -> BAS::anonymous::JournalEntry (E.g., 7 -> JournalEntry) 
  // BASJournals: Map BASJournalId -> BASJournal E.g., 'A' -> BASJournal

public:

  SIEEnvironment(FiscalYear const& fiscal_year);
  SIEEnvironment() = delete;

  // Entry API
	void post(BAS::MDJournalEntry const& mdje);

  class StageEntryResult {
  public:
    enum class Status {
       Unknown
      ,NowPosted
      ,StagedOk
      ,Undefined
    };
    StageEntryResult() = delete;
    StageEntryResult(BAS::MDJournalEntry const& mdje,Status status = Status{})
      : m_md_entry{mdje},m_status{status} {}
    bool now_posted() const {return m_status == Status::NowPosted;}
    operator bool() const {return m_status == Status::StagedOk;}
    BAS::MDJournalEntry const& md_entry() const {return m_md_entry;}
    StageEntryResult& set_status(Status status) {m_status = status; return *this;}
  private:
    Status m_status{};
    BAS::MDJournalEntry m_md_entry; // Note: SIEEnvironment does not store actual BAS::MetaEntry.
                            //       So we store a ref-safe clone to return as result
                            //       This may e.g., allow for returning a mutated entry.
  };
	StageEntryResult stage(BAS::MDJournalEntry const& mdje);
	BAS::OptionalMDJournalEntry add(BAS::MDJournalEntry mdje);
	BAS::OptionalMDJournalEntry update(BAS::MDJournalEntry const& mdje);
	BAS::VerNo largest_verno(BAS::Series series);
	bool already_in_posted(BAS::MDJournalEntry const& mdje);

  // Entries API
	BAS::MDJournalEntries stage(SIEEnvironment const& staged_sie_environment);

  // file API
	std::filesystem::path staged_sie_file_path() const;
  std::filesystem::path source_sie_file_path() const;
  void set_source_sie_file_path(std::filesystem::path const& source_sie_file_path);

  // Meta data API
	SIE::OrgNr organisation_no{};
	SIE::FNamn organisation_name{};
	SIE::Adress organisation_address{};

  // Journals API
	BASJournals& journals();
	BASJournals const& journals() const;

  // The rest of the API
	bool is_unposted(BAS::Series series, BAS::VerNo verno) const;

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
	BAS::MDJournalEntries unposted() const;
	
private:
	BASJournals m_journals{};
	zeroth::OptionalDateRange m_year_date_range{};
	FiscalYear m_fiscal_year;
	std::map<char,BAS::VerNo> verno_of_last_posted_to{};
	std::map<BAS::AccountNo,Amount> opening_balance{};
  friend class SIEEnvironmentsMap;
  // Path to the file from which this environment originated (external tool SIE export)
	std::filesystem::path sie_file_path{};

}; // class SIEEnvironment

using OptionalSIEEnvironment = std::optional<SIEEnvironment>;

