#pragma once

#include "sie.hpp"
#include "logger/log.hpp"

class SIEEnvironment {
public:

  SIEEnvironment(FiscalYear const& fiscal_year);

  SIEEnvironment() = delete;

  // Path to the file from which this environment originated (external tool SIE export)
	std::filesystem::path sie_file_path{};

	std::filesystem::path staged_sie_file_path() const {
		return std::format(
			 "cratchit_{}_{}.se"
			,this->m_fiscal_year.start()
			,this->m_fiscal_year.last());
  };

	SIE::OrgNr organisation_no{};
	SIE::FNamn organisation_name{};
	SIE::Adress organisation_address{};

	BASJournals& journals() {return m_journals;}
	BASJournals const& journals() const {return m_journals;}
	bool is_unposted(BAS::Series series, BAS::VerNo verno) const {
		bool result{true}; // deafult unposted
		if (verno_of_last_posted_to.contains(series)) result = (verno > this->verno_of_last_posted_to.at(series));
		return result;
	}
	SKV::SRU::OptionalAccountNo sru_code(BAS::AccountNo const& bas_account_no) {
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
	BAS::OptionalAccountNos to_bas_accounts(SKV::SRU::AccountNo const& sru_code) const {
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

	void post(BAS::MetaEntry const& me) {
    // logger::cout_proxy << "\npost(" << me << ")"; 
		if (me.meta.verno) {
			m_journals[me.meta.series][*me.meta.verno] = me.defacto;
			verno_of_last_posted_to[me.meta.series] = *me.meta.verno;
		}
		else {
			logger::cout_proxy << "\nSIEEnvironment::post failed - can't post an entry with null verno";
		}
	}

  std::size_t journals_entry_count() const {
    std::size_t result{};
    for (auto const& [series,journal] : this->m_journals) {
      result += journal.size();
    }
    return result;
  }

  // Try to stage all provided entries for posting
  // Returns actually staged entries
	BAS::MetaEntries stage(SIEEnvironment const& staged_sie_environment);

	BAS::MetaEntries unposted() const {
		// logger::cout_proxy << "\nunposted()";
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

	BAS::AccountMetas const&  account_metas() const {return BAS::detail::global_account_metas;} // const ref global instance

	// void set_year_date_range(zeroth::DateRange const& dr) {
	// 	this->m_year_date_range = dr;
	// 	// logger::cout_proxy << "\nset_year_date_range <== " << *this->year_date_range;
	// }

	void set_account_name(BAS::AccountNo bas_account_no ,std::string const& name) {
		if (BAS::detail::global_account_metas.contains(bas_account_no)) {
			if (BAS::detail::global_account_metas[bas_account_no].name != name) {
				logger::cout_proxy << "\nWARNING: BAS Account " << bas_account_no << " name " << std::quoted(BAS::detail::global_account_metas[bas_account_no].name) << " changed to " << std::quoted(name);
			}
		}
		BAS::detail::global_account_metas[bas_account_no].name = name; // Mutate global instance
	}
	void set_account_SRU(BAS::AccountNo bas_account_no, SKV::SRU::AccountNo sru_code) {
		if (BAS::detail::global_account_metas.contains(bas_account_no)) {
			if (BAS::detail::global_account_metas[bas_account_no].sru_code) {
				if (*BAS::detail::global_account_metas[bas_account_no].sru_code != sru_code) {
					logger::cout_proxy << "\nWARNING: BAS Account " << bas_account_no << " SRU Code " << *BAS::detail::global_account_metas[bas_account_no].sru_code << " changed to " << sru_code;
				}
			}
		}
		BAS::detail::global_account_metas[bas_account_no].sru_code = sru_code; // Mutate global instance
	}

	void set_opening_balance(BAS::AccountNo bas_account_no,Amount opening_balance) {
		if (this->opening_balance.contains(bas_account_no) == false) this->opening_balance[bas_account_no] = opening_balance;
		else {
			logger::cout_proxy << "\nDESIGN INSUFFICIENCY - set_opening_balance failed. Balance for bas_account_no:" << bas_account_no;
			logger::cout_proxy << " is already registered as " << this->opening_balance[bas_account_no] << ".";
			logger::cout_proxy << " Provided opening_balance:" << opening_balance << " IGNORED";
		}
	}

	BalancesMap balances_at(Date date) {
		BalancesMap result{};
		for (auto const& ob : this->opening_balance) {
			// struct Balance {
			// 	BAS::AccountNo account_no;
			// 	Amount opening_balance;
			// 	Amount change;
			// 	Date date;
			// 	Amount end_balance;
			// };				
			result[date].push_back(Balance{
				.account_no = ob.first
				,.opening_balance = ob.second
				,.change = -1
				,.end_balance = -1
			});
		}
		return result;
	}

	FiscalYear fiscal_year() const { return m_fiscal_year;}

	// TODO: Remove optional / 20251029
	zeroth::OptionalDateRange financial_year_date_range() const {
		return this->fiscal_year().period();
	}

  OptionalAmount opening_balance_of(BAS::AccountNo bas_account_no) {
    OptionalAmount result{};
    if (this->opening_balance.contains(bas_account_no)) {
      result = this->opening_balance.at(bas_account_no);
    }
    return result;
  }

  std::map<BAS::AccountNo,Amount> const& opening_balances() const {
    return this->opening_balance;
  }
	
private:
	BASJournals m_journals{};
	zeroth::OptionalDateRange m_year_date_range{};
	FiscalYear m_fiscal_year;
	std::map<char,BAS::VerNo> verno_of_last_posted_to{};
	std::map<BAS::AccountNo,Amount> opening_balance{};
  friend class SIEEnvironmentsMap;

  // Try to stage provided me.
  // A succesfull stage either adds it ot uopdate an existing entry.
  // This is to allow cratchit to edit an entry imported from an external tool.
	std::optional<BAS::MetaEntry> stage(BAS::MetaEntry const& me);

	BAS::MetaEntry add(BAS::MetaEntry me);

	BAS::MetaEntry update(BAS::MetaEntry const& me);

	BAS::VerNo largest_verno(BAS::Series series);

	bool already_in_posted(BAS::MetaEntry const& me);

}; // class SIEEnvironment
using OptionalSIEEnvironment = std::optional<SIEEnvironment>;

