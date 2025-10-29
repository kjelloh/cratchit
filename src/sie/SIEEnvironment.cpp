#include "SIEEnvironment.hpp"
#include <numeric> // std::accumulate,...

SIEEnvironment::SIEEnvironment(FiscalYear const& fiscal_year)
	: m_fiscal_year{fiscal_year} {}

BAS::VerNo SIEEnvironment::largest_verno(BAS::Series series) {
	auto const& journal = m_journals[series];
	return std::accumulate(journal.begin(),journal.end(),unsigned{},[](auto acc,auto const& entry){
		return (acc<entry.first)?entry.first:acc;
	});
}
