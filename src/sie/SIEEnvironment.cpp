#include "SIEEnvironment.hpp"
#include <numeric> // std::accumulate,...

	BAS::VerNo SIEEnvironment::largest_verno(BAS::Series series) {
		auto const& journal = m_journals[series];
		return std::accumulate(journal.begin(),journal.end(),unsigned{},[](auto acc,auto const& entry){
			return (acc<entry.first)?entry.first:acc;
		});
	}
