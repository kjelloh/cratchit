#include "SKVFramework.hpp"

namespace SKV {
    namespace XML {
        namespace VATReturns {
			Amount to_box_49_amount(FormBoxMap const& box_map) {
				BoxNos vat_box_nos{10,11,12,30,31,32,60,61,62,48};
				auto box_49_amount = std::accumulate(vat_box_nos.begin(),vat_box_nos.end(),Amount{},[&box_map](Amount acc,BoxNo box_no){
					if (box_map.contains(box_no)) {
						auto mats_sum = BAS::mats_sum(box_map.at(box_no));
						acc += mats_sum;
						std::cout << "\n\tto_box_49_amount += [" << box_no << "]:" << mats_sum << " = " << acc;
					}
					return acc;
				});
				return box_49_amount;
			}
        } // namespace VATReturns
    } // namespace XML
} // namespace SKV

std::set<BAS::AccountNo> to_vat_returns_form_bas_accounts(BAS::SKV::BoxNos const& box_nos) {
	return SKV::XML::VATReturns::to_accounts(box_nos);
}

std::set<BAS::AccountNo> const& to_vat_accounts() {
	static auto const vat_accounts = SKV::XML::VATReturns::to_vat_accounts(); // cache
	// Define in terms of how SKV VAT returns form defines linking to BAS Accounts for correct content
	return vat_accounts;
}

std::optional<unsigned int> to_four_digit_positive_int(std::string const& s) {
	std::optional<unsigned int> result{};
	try {
		if (s.size()==4) {
			if (std::all_of(s.begin(),s.end(),::isdigit)) {
				auto account_no = std::stoi(s);
				if (account_no >= 1000) result = account_no;
			}
		}
	}
	catch (std::exception const& e) { std::cout << "\nDESIGN INSUFFICIENCY: to_four_digit_positive_int(" << s << ") failed. Exception=" << std::quoted(e.what());}
	return result;
}

namespace SKV {
	namespace SRU {
    } // namespace SRU
} // namespace SKV