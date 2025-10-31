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

std::optional<BAS::MetaEntry> SIEEnvironment::stage(BAS::MetaEntry const& me) {
  std::optional<BAS::MetaEntry> result{};

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
        result = this->add(me);
      }
      else {
        // Is 'posted' to external tool
        // But update it in case we have edited it
        result = this->update(me);
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

BAS::MetaEntry SIEEnvironment::add(BAS::MetaEntry me) {
  logger::scope_logger log_raii{logger::development_trace,"SIEEnvironment::add(BAS::MetaEntry)"};

  BAS::MetaEntry result{me};

  // Ensure a valid series
  if ((result.meta.series < 'A') or ('M' < result.meta.series)) {
    result.meta.series = 'A';
    logger::cout_proxy << "\nadd(me): assigned series 'A' to entry with no series assigned";
  }

  auto actual_verno = 
    result.meta.verno
    .value_or(largest_verno(me.meta.series) + 1);    

  // LOG
  if (!me.meta.verno) {
    logger::development_trace("add(me): assigned verno:{} to entry with no verno assigned",actual_verno);
  }

  result.meta.verno = actual_verno;

  if (!m_journals[me.meta.series].contains(result.meta.verno.value())) {
    m_journals[me.meta.series][result.meta.verno.value()] = me.defacto;
  }
  else {
    logger::cout_proxy << "\nDESIGN INSUFFICIENCY: Ignored adding new voucher with already existing ID " << me.meta.series << actual_verno;
  }
  return result;
} // add

BAS::MetaEntry SIEEnvironment::update(BAS::MetaEntry const& me) {
  logger::scope_logger log_raii{logger::development_trace,"SIEEnvironment::update(BAS::MetaEntry)"};

  logger::cout_proxy << "\nupdate(" << me << ")" << std::flush; 
  BAS::MetaEntry result{me};
  if (me.meta.verno and *me.meta.verno > 0) {
    auto journal_iter = m_journals.find(me.meta.series);
    if (journal_iter != m_journals.end()) {
      if (me.meta.verno) {
        auto entry_iter = journal_iter->second.find(*me.meta.verno);
        if (entry_iter != journal_iter->second.end()) {
          entry_iter->second = me.defacto; // update
          // logger::cout_proxy << "\nupdated :" << entry_iter->second;
          // logger::cout_proxy << "\n    --> :" << me;
        }
      }
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

// private:

