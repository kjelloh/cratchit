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

// private:

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
  if (me.meta.series < 'A' or 'M' < me.meta.series) {
    me.meta.series = 'A';
    logger::cout_proxy << "\nadd(me) assigned series 'A' to entry with no series assigned";
  }
  // Assign "actual" sequence number
  auto verno = largest_verno(me.meta.series) + 1;
  // logger::cout_proxy << "\n\tSetting actual ver no:" << verno;
  result.meta.verno = verno;
  if (m_journals[me.meta.series].contains(verno) == false) {
    m_journals[me.meta.series][verno] = me.defacto;
  }
  else {
    logger::cout_proxy << "\nDESIGN INSUFFICIENCY: Ignored adding new voucher with already existing ID " << me.meta.series << verno;
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
	return std::accumulate(journal.begin(),journal.end(),unsigned{},[](auto acc,auto const& entry){
		return (acc<entry.first)?entry.first:acc;
	});
}

bool SIEEnvironment::already_in_posted(BAS::MetaEntry const& me) {
  bool result{false};
  if (me.meta.verno and *me.meta.verno > 0) {
    auto journal_iter = m_journals.find(me.meta.series);
    if (journal_iter != m_journals.end()) {
      if (me.meta.verno) {
        auto entry_iter = journal_iter->second.find(*me.meta.verno);
        result = (entry_iter != journal_iter->second.end());

        // Log
        if (true) {
          if (result and !(me.defacto == entry_iter->second)) {
            // sie-id in posted but with another value
            logger::design_insufficiency(
              "already_in_posted but with another value!");
          } 
        }
      }
    }
  }

  if (true) {
    logger::development_trace("already_in_posted:{} for {}{}"
      ,result
      ,me.meta.series
      ,logger::opt_to_string(me.meta.verno,"verno"));
  }
  return result;
}

