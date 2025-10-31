#pragma once

#include "fiscal/BAS_SKV_Crossdependencies.hpp" // SKV types consumed by BAS and us
#include "fiscal/BASFramework.hpp" // BAS::MetaAccountTransactions,
#include <optional>
#include <string>
#include <map>
#include <vector>
#include <set>

namespace SKV { 
	namespace XML {
		namespace VATReturns {
            using BoxNo = unsigned int;
            using FormBoxMap = std::map<BoxNo,BAS::MDAccountTransactions>;
            using BoxNos = std::vector<BoxNo>;

			BoxNos const EU_VAT_BOX_NOS{30,31,32};
			BoxNos const EU_PURCHASE_BOX_NOS{20,21};
			BoxNos const VAT_BOX_NOS{10,11,12,30,31,32,60,61,62,48,49};

			Amount to_box_49_amount(FormBoxMap const& box_map);

			BAS::AccountNos to_accounts(BoxNo box_no);
			std::set<BAS::AccountNo> to_accounts(BoxNos const& box_nos);

		} // namespace VATReturns 
	} // namespace XML 
} // namespace SKV 

std::set<BAS::AccountNo> to_vat_returns_form_bas_accounts(SKV::XML::VATReturns::BoxNos const& box_nos);
std::set<BAS::AccountNo> const& to_vat_accounts();

std::optional<unsigned int> to_four_digit_positive_int(std::string const& s);

namespace SKV {
	namespace SRU {

        // SRU AccountNo, etc. now in BAS_SKV_Crossdependencies.hpp to break circular dependency
        //     SKVFramework can consume BASFramework while BASFramework consumes BAS_SKV_Crossdependencies.hpp

		inline OptionalAccountNo to_account_no(std::string const& s) {
			return to_four_digit_positive_int(s);
		}
	} // namespace SRU
} // namespace SKV
