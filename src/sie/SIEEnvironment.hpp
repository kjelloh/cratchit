#pragma once

#include "sie.hpp"


class SIEEnvironment {
public:
  // Path to the file from which this environment originated (external tool SIE export)
	std::filesystem::path sie_file_path{};

	std::filesystem::path staged_sie_file_path() const {
    if (this->year_date_range) {
      return std::format("cratchit_{}_{}.se",this->year_date_range->begin(),this->year_date_range->end());
    }
    else {
      return "cratchit.se"; // Hard coded and asuming only used for "current"
    }
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
	BAS::OptionalAccountNos to_bas_accounts(SKV::SRU::AccountNo const& sru_code) {
		BAS::OptionalAccountNos result{};
		try {
			BAS::AccountNos bas_account_nos{};
			std::for_each(account_metas().begin(),account_metas().end(),[&sru_code,&bas_account_nos](auto const& entry){
				if (entry.second.sru_code == sru_code) bas_account_nos.push_back(entry.first);
			});
			if (bas_account_nos.size() > 0) result = bas_account_nos;
		}
		catch (std::exception const& e) {
			std::cout << "\nto_bas_accounts failed. Exception=" << std::quoted(e.what());
		}
		return result;
	}

	void post(BAS::MetaEntry const& me) {
    // std::cout << "\npost(" << me << ")"; 
		if (me.meta.verno) {
			m_journals[me.meta.series][*me.meta.verno] = me.defacto;
			verno_of_last_posted_to[me.meta.series] = *me.meta.verno;
		}
		else {
			std::cout << "\nSIEEnvironment::post failed - can't post an entry with null verno";
		}
	}

  // Try to stage all provided entries for posting
  // Returns actually staged entries
	BAS::MetaEntries stage(SIEEnvironment const& staged_sie_environment) {
    // std::cout << "\nstage(staged_sie_environment)"  << std::flush; 
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
	BAS::MetaEntries unposted() const {
		// std::cout << "\nunposted()";
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

	void set_year_date_range(zeroth::DateRange const& dr) {
		this->year_date_range = dr;
		// std::cout << "\nset_year_date_range <== " << *this->year_date_range;
	}

	void set_account_name(BAS::AccountNo bas_account_no ,std::string const& name) {
		if (BAS::detail::global_account_metas.contains(bas_account_no)) {
			if (BAS::detail::global_account_metas[bas_account_no].name != name) {
				std::cout << "\nWARNING: BAS Account " << bas_account_no << " name " << std::quoted(BAS::detail::global_account_metas[bas_account_no].name) << " changed to " << std::quoted(name);
			}
		}
		BAS::detail::global_account_metas[bas_account_no].name = name; // Mutate global instance
	}
	void set_account_SRU(BAS::AccountNo bas_account_no, SKV::SRU::AccountNo sru_code) {
		if (BAS::detail::global_account_metas.contains(bas_account_no)) {
			if (BAS::detail::global_account_metas[bas_account_no].sru_code) {
				if (*BAS::detail::global_account_metas[bas_account_no].sru_code != sru_code) {
					std::cout << "\nWARNING: BAS Account " << bas_account_no << " SRU Code " << *BAS::detail::global_account_metas[bas_account_no].sru_code << " changed to " << sru_code;
				}
			}
		}
		BAS::detail::global_account_metas[bas_account_no].sru_code = sru_code; // Mutate global instance
	}

	void set_opening_balance(BAS::AccountNo bas_account_no,Amount opening_balance) {
		if (this->opening_balance.contains(bas_account_no) == false) this->opening_balance[bas_account_no] = opening_balance;
		else {
			std::cout << "\nDESIGN INSUFFICIENCY - set_opening_balance failed. Balance for bas_account_no:" << bas_account_no;
			std::cout << " is already registered as " << this->opening_balance[bas_account_no] << ".";
			std::cout << " Provided opening_balance:" << opening_balance << " IGNORED";
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

	zeroth::OptionalDateRange financial_year_date_range() const {
		return this->year_date_range;
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
	zeroth::OptionalDateRange year_date_range{};
	std::map<char,BAS::VerNo> verno_of_last_posted_to{};
	std::map<BAS::AccountNo,Amount> opening_balance{};
  friend class SIEEnvironmentsMap;

  // Try to stage provided me.
  // A succesfull stage either adds it ot uopdate an existing entry.
  // This is to allow cratchit to edit an entry imported from an external tool.
	std::optional<BAS::MetaEntry> stage(BAS::MetaEntry const& me) {
    // std::cout << "\nstage(" << me << ")"  << std::flush; 
		std::optional<BAS::MetaEntry> result{};

    if (!(this->financial_year_date_range() and  this->financial_year_date_range()->contains(me.defacto.date))) {
      // Block adding an entry with a date outside an SIE envrionment financial date range
      std::cout << "\nDate:" << me.defacto.date << " is not in financial year:";
      if (this->financial_year_date_range()) std::cout << this->financial_year_date_range().value();
      else std::cout << "*anonymous*";
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
			std::cout << "\nSorry, Failed to stage. Entry Does not Balance";
		}
		return result;
	}

	BAS::MetaEntry add(BAS::MetaEntry me) {
    std::cout << "\nadd(" << me << ")"  << std::flush; 
		BAS::MetaEntry result{me};
		// Ensure a valid series
		if (me.meta.series < 'A' or 'M' < me.meta.series) {
			me.meta.series = 'A';
			std::cout << "\nadd(me) assigned series 'A' to entry with no series assigned";
		}
		// Assign "actual" sequence number
		auto verno = largest_verno(me.meta.series) + 1;
    // std::cout << "\n\tSetting actual ver no:" << verno;
		result.meta.verno = verno;
    if (m_journals[me.meta.series].contains(verno) == false) {
		  m_journals[me.meta.series][verno] = me.defacto;
    }
    else {
      std::cout << "\nDESIGN INSUFFICIENCY: Ignored adding new voucher with already existing ID " << me.meta.series << verno;
    }
		return result;
	}

	BAS::MetaEntry update(BAS::MetaEntry const& me) {
    std::cout << "\nupdate(" << me << ")" << std::flush; 
		BAS::MetaEntry result{me};
		if (me.meta.verno and *me.meta.verno > 0) {
			auto journal_iter = m_journals.find(me.meta.series);
			if (journal_iter != m_journals.end()) {
				if (me.meta.verno) {
					auto entry_iter = journal_iter->second.find(*me.meta.verno);
					if (entry_iter != journal_iter->second.end()) {
						entry_iter->second = me.defacto; // update
            // std::cout << "\nupdated :" << entry_iter->second;
            // std::cout << "\n    --> :" << me;
					}
				}
			}
		}
		return result;
	}

	BAS::VerNo largest_verno(BAS::Series series);

	bool already_in_posted(BAS::MetaEntry const& me) {
		bool result{false};
		if (me.meta.verno and *me.meta.verno > 0) {
			auto journal_iter = m_journals.find(me.meta.series);
			if (journal_iter != m_journals.end()) {
				if (me.meta.verno) {
					auto entry_iter = journal_iter->second.find(*me.meta.verno);
					result = (entry_iter != journal_iter->second.end());
				}
			}
		}
    if (true) {
      std::cout << "\nalready_in_posted(me=" << me << ") = ";
      if (result) std::cout << " TRUE";
      else std::cout << " false";
    }
		return result;
	}
}; // class SIEEnvironment
using OptionalSIEEnvironment = std::optional<SIEEnvironment>;

