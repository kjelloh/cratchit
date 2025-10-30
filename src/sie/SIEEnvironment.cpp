#include "SIEEnvironment.hpp"
#include "logger/log.hpp"

#include <numeric> // std::accumulate,...

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

BAS::VerNo SIEEnvironment::largest_verno(BAS::Series series) {
	auto const& journal = m_journals[series];
	return std::accumulate(journal.begin(),journal.end(),unsigned{},[](auto acc,auto const& entry){
		return (acc<entry.first)?entry.first:acc;
	});
}

std::optional<BAS::MetaEntry> SIEEnvironment::stage(BAS::MetaEntry const& me) {

  // Log
  auto opt_to_string = [](auto const& maybe_v,std::string_view const& null_name) {
    return maybe_v
      .transform([](auto v) { return std::format("{}", v); })
      .value_or(std::format("?{}?",null_name));  
  };
  logger::scope_logger log_raii{
     logger::development_trace
    ,std::format("SIEEnvironment::stage(BAS::MetaEntry '{}{}')",me.meta.series,opt_to_string(me.meta.verno,"ver_no"))};

  std::optional<BAS::MetaEntry> result{};

  if (!(this->financial_year_date_range() and  this->financial_year_date_range()->contains(me.defacto.date))) {
    // Block adding an entry with a date outside an SIE envrionment financial date range
    
    logger::cout_proxy << "\nDate:" << me.defacto.date << " is not in financial year:";
    if (this->financial_year_date_range()) logger::cout_proxy << this->financial_year_date_range().value();
    else logger::cout_proxy << "*anonymous*";
    return std::nullopt;
  }
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
  return result;
}

