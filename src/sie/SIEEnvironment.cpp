#include "SIEEnvironment.hpp"
#include "logger/log.hpp"

#include <numeric> // std::accumulate,...

// Log helper
namespace logger {
  auto opt_to_string = [](auto const& maybe_v,std::string_view const& null_name) {
    return maybe_v
      .transform([](auto v) { return std::format("{}", v); })
      .value_or(std::format("?{}?",null_name));  
  };
}


// public:

SIEEnvironment::SIEEnvironment(FiscalYear const& fiscal_year)
	: m_fiscal_year{fiscal_year} {}

// Entry API
void SIEEnvironment::post(BAS::MetaEntry const& me) {

  logger::scope_logger log_raii{
     logger::development_trace
    ,std::format("SIEEnvironment::post(BAS::MetaEntry '{}{}')"
      ,me.meta.series
      ,logger::opt_to_string(me.meta.verno,"ver_no"))};

  if (me.meta.verno) {
    m_journals[me.meta.series][*me.meta.verno] = me.defacto;
    verno_of_last_posted_to[me.meta.series] = *me.meta.verno;

    // LOG
    logger::development_trace("verno_of_last_posted_to[{}] = {} "
      ,me.meta.series
      ,verno_of_last_posted_to[me.meta.series]);

  }
  else {
    logger::cout_proxy << "\nSIEEnvironment::post failed - can't post an entry with null verno";
  }
}

SIEEnvironment::StageEntryResult SIEEnvironment::stage(BAS::MetaEntry const& me) {
  StageEntryResult result{me,SIEEnvironment::StageEntryResult::Status::Undefined};

  // scope Log
  logger::scope_logger log_raii{
     logger::development_trace
    ,std::format("SIEEnvironment::stage(BAS::MetaEntry '{}{}')"
      ,me.meta.series
      ,logger::opt_to_string(me.meta.verno,"ver_no"))};


  // if (!(this->financial_year_date_range() and  this->financial_year_date_range()->contains(me.defacto.date))) {
  if (this->fiscal_year().contains(me.defacto.date)) {
    if (does_balance(me.defacto)) {
      if (not this->already_in_posted(me)) {
        // Not yet posted (in sie-file from external tool)
        // So add it to make our internal sie-environment complete
        result.set_status(this->add(me).has_value()?StageEntryResult::Status::NowPosted:StageEntryResult::Status::Undefined);
      }
      else {
        // Is 'posted' to external tool
        // But update it in case we have edited it
        result.set_status(this->update(me).has_value()?StageEntryResult::Status::NowPosted:StageEntryResult::Status::Undefined);
      }
    }
    else {
      logger::cout_proxy << "\nSorry, Failed to stage. Entry Does not Balance";
    }
  }
  else {
    // Block adding an entry with a date outside an SIE envrionment financial date range

    // Log
    if (true) {
      logger::cout_proxy << "\nDate:" << me.defacto.date << " is not in financial year:";
      if (this->financial_year_date_range()) logger::cout_proxy << this->financial_year_date_range().value();
      else logger::cout_proxy << "*anonymous*";
    }
  }
  return result;
} // stage


BAS::OptionalMetaEntry SIEEnvironment::add(BAS::MetaEntry me) {
  logger::scope_logger log_raii{logger::development_trace,"SIEEnvironment::add(BAS::MetaEntry)"};

  BAS::OptionalMetaEntry result{};

  auto candidate = me;

  // Ensure a valid series
  if ((candidate.meta.series < 'A') or ('M' < candidate.meta.series)) {
    candidate.meta.series = 'A';
    logger::cout_proxy << "\nadd(me): assigned series 'A' to entry with no series assigned";
  }

  auto actual_verno = 
    candidate.meta.verno
    .value_or(largest_verno(me.meta.series) + 1);    

  // LOG
  if (!me.meta.verno) {
    logger::development_trace("add(me): assigned verno:{} to entry with no verno assigned",actual_verno);
  }

  candidate.meta.verno = actual_verno;

  if (!m_journals[candidate.meta.series].contains(candidate.meta.verno.value())) {
    m_journals[candidate.meta.series][candidate.meta.verno.value()] = candidate.defacto;
    result = candidate;
  }
  else {
    logger::cout_proxy << "\nDESIGN INSUFFICIENCY: Ignored adding new voucher with already existing ID " << me.meta.series << actual_verno;
  }
  return result;
} // add

BAS::OptionalMetaEntry SIEEnvironment::update(BAS::MetaEntry const& me) {
  logger::scope_logger log_raii{logger::development_trace,"SIEEnvironment::update(BAS::MetaEntry)"};
  logger::cout_proxy << "\nupdate(" << me << ")" << std::flush; 

  BAS::OptionalMetaEntry result{};

  if (me.meta.verno and *me.meta.verno > 0) {
    auto journal_iter = m_journals.find(me.meta.series);
    if (journal_iter != m_journals.end()) {
      if (me.meta.verno) {
        auto entry_iter = journal_iter->second.find(*me.meta.verno);
        if (entry_iter != journal_iter->second.end()) {
          entry_iter->second = me.defacto; // update
          result = BAS::MetaEntry{me.meta,entry_iter->second};
          // logger::cout_proxy << "\nupdated :" << entry_iter->second;
          // logger::cout_proxy << "\n    --> :" << me;
        }
        else {
          logger::design_insufficiency("Can't update no-nexistent entry {}{}",me.meta.series,me.meta.verno.value());
        }
      }
      else {
        logger::design_insufficiency("Can't update entry with null verno");
      }
    }
    else {
      logger::design_insufficiency("Can't update entry in non recorded series {}",me.meta.series);
    }
  }
  return result;
} // update


BAS::VerNo SIEEnvironment::largest_verno(BAS::Series series) {
	auto const& journal = m_journals[series];
  // return 0 (empty) or 1...n for found largest
	return std::accumulate(journal.begin(),journal.end(),unsigned{0},[](auto acc,auto const& entry){
		return (acc<entry.first) ? entry.first : acc;
	});
}

bool SIEEnvironment::already_in_posted(BAS::MetaEntry const& me) {

  bool result{false};

  if (!me.meta.verno) {
    logger::design_insufficiency(
      "already_in_posted failed - provided me has no verno");
  }
  else if (me.meta.verno.value() > 0) {
    result = !this->is_unposted(me.meta.series,me.meta.verno.value());
  }
  else {
    logger::design_insufficiency("already_in_posted(me): Failed - called with me having invalid verno:0");
  }

  return result;
}

// Entries API
BAS::MetaEntries SIEEnvironment::stage(SIEEnvironment const& staged_sie_environment) {
  logger::scope_logger log_raii{
      logger::development_trace
    ,std::format("stage(SIEEnvironment:{})",staged_sie_environment.journals_entry_count())};
  BAS::MetaEntries result{};
  for (auto const& [series,journal] : staged_sie_environment.journals()) {
    for (auto const& [verno,aje] : journal) {
      auto je = this->stage({{.series=series,.verno=verno},aje});
      if (!je) {
        result.push_back({{.series=series,.verno=verno},aje}); // no longer staged
      }
    }
  }
  return result;
}

std::filesystem::path SIEEnvironment::staged_sie_file_path() const {
  return std::format(
      "cratchit_{}_{}.se"
    ,this->m_fiscal_year.start()
    ,this->m_fiscal_year.last());
};

std::filesystem::path SIEEnvironment::source_sie_file_path() const {
  return this->sie_file_path;
}

void SIEEnvironment::set_source_sie_file_path(std::filesystem::path const& source_sie_file_path) {
  this->sie_file_path = source_sie_file_path;
}


// Journals API
BASJournals& SIEEnvironment::journals() {return m_journals;}
BASJournals const& SIEEnvironment::journals() const {return m_journals;}

// The rest of the API

bool SIEEnvironment::is_unposted(BAS::Series series, BAS::VerNo verno) const {

  bool result{true}; // deafult unposted

  if (verno_of_last_posted_to.contains(series)) {
    result = (verno > this->verno_of_last_posted_to.at(series));

    logger::development_trace("is_unposted: verno_of_last_posted_to[{}] = {}"
      ,series
      ,this->verno_of_last_posted_to.at(series));
  }
  else {
    // No record = unposted
    logger::development_trace("No 'is posted' record for {}{}",series,verno);
  }

  if (true) {
    if (result)
      logger::development_trace("NOT yet posted:{}{}",series,verno);
    else
      logger::development_trace("IS posted:{}{}",series,verno);
  }

  return result;
}

SKV::SRU::OptionalAccountNo SIEEnvironment::sru_code(BAS::AccountNo const& bas_account_no) {
  SKV::SRU::OptionalAccountNo result{};
  try {
    auto iter = std::find_if(account_metas().begin(),account_metas().end(),[&bas_account_no](auto const& entry){
      return (entry.first == bas_account_no);
    });
    if (iter != account_metas().end()) {
      result = iter->second.sru_code;
    }
  }
  catch (std::exception const& e) {} // Ignore/silence
  return result;
}

BAS::OptionalAccountNos SIEEnvironment::to_bas_accounts(SKV::SRU::AccountNo const& sru_code) const {
  BAS::OptionalAccountNos result{};
  try {
    BAS::AccountNos bas_account_nos{};
    std::for_each(account_metas().begin(),account_metas().end(),[&sru_code,&bas_account_nos](auto const& entry){
      if (entry.second.sru_code == sru_code) bas_account_nos.push_back(entry.first);
    });
    if (bas_account_nos.size() > 0) result = bas_account_nos;
  }
  catch (std::exception const& e) {
    logger::cout_proxy << "\nto_bas_accounts failed. Exception=" << std::quoted(e.what());
  }
  return result;
}

std::size_t SIEEnvironment::journals_entry_count() const {
  std::size_t result{};
  for (auto const& [series,journal] : this->m_journals) {
    result += journal.size();
  }
  return result;
}

BAS::MetaEntries SIEEnvironment::unposted() const {

  logger::scope_logger log_raii{
     logger::development_trace
    ,std::format("SIEEnvironment::unposted:")};

  BAS::MetaEntries result{};
  for (auto const& [series,journal] : this->m_journals) {
    for (auto const& [verno,je] : journal) {
      if (this->is_unposted(series,verno)) {
        BAS::MetaEntry bjer{
          .meta = {
            .series = series
            ,.verno = verno
          }
          ,.defacto = je
        };
        result.push_back(bjer);
      }        
    }
  }
  return result;
}

BAS::AccountMetas const&  SIEEnvironment::account_metas() const {
  return BAS::detail::global_account_metas;
}

void SIEEnvironment::set_account_name(BAS::AccountNo bas_account_no ,std::string const& name) {
  if (BAS::detail::global_account_metas.contains(bas_account_no)) {
    if (BAS::detail::global_account_metas[bas_account_no].name != name) {
      logger::cout_proxy << "\nWARNING: BAS Account " << bas_account_no << " name " << std::quoted(BAS::detail::global_account_metas[bas_account_no].name) << " changed to " << std::quoted(name);
    }
  }
  BAS::detail::global_account_metas[bas_account_no].name = name; // Mutate global instance
}

void SIEEnvironment::set_account_SRU(BAS::AccountNo bas_account_no, SKV::SRU::AccountNo sru_code) {
  if (BAS::detail::global_account_metas.contains(bas_account_no)) {
    if (BAS::detail::global_account_metas[bas_account_no].sru_code) {
      if (*BAS::detail::global_account_metas[bas_account_no].sru_code != sru_code) {
        logger::cout_proxy << "\nWARNING: BAS Account " << bas_account_no << " SRU Code " << *BAS::detail::global_account_metas[bas_account_no].sru_code << " changed to " << sru_code;
      }
    }
  }
  BAS::detail::global_account_metas[bas_account_no].sru_code = sru_code; // Mutate global instance
}

void SIEEnvironment::set_opening_balance(BAS::AccountNo bas_account_no,Amount opening_balance) {
  if (this->opening_balance.contains(bas_account_no) == false) this->opening_balance[bas_account_no] = opening_balance;
  else {
    logger::cout_proxy << "\nDESIGN INSUFFICIENCY - set_opening_balance failed. Balance for bas_account_no:" << bas_account_no;
    logger::cout_proxy << " is already registered as " << this->opening_balance[bas_account_no] << ".";
    logger::cout_proxy << " Provided opening_balance:" << opening_balance << " IGNORED";
  }
}

BalancesMap SIEEnvironment::balances_at(Date date) {
  BalancesMap result{};
  for (auto const& ob : this->opening_balance) {
    result[date].push_back(Balance{
      .account_no = ob.first
      ,.opening_balance = ob.second
      ,.change = -1
      ,.end_balance = -1
    });
  }
  return result;
}

FiscalYear SIEEnvironment::fiscal_year() const { return m_fiscal_year;}

// TODO: Remove optional / replace with fiscal_year() call  / 20251029
zeroth::OptionalDateRange SIEEnvironment::financial_year_date_range() const {
  return this->fiscal_year().period();
}

OptionalAmount SIEEnvironment::opening_balance_of(BAS::AccountNo bas_account_no) const {
  OptionalAmount result{};
  if (this->opening_balance.contains(bas_account_no)) {
    result = this->opening_balance.at(bas_account_no);
  }
  return result;
}

std::map<BAS::AccountNo,Amount> const& SIEEnvironment::opening_balances() const {
  return this->opening_balance;
}

// private:

