#include "SKVFramework.hpp"

#include <numeric> // std::accumulate,
#include <algorithm> // std::copy,
#include <iterator> // std::inserter,
#include <sstream> // std::ostringstream,
#include <exception> // std::exception,
#include <iostream> // std::cout,
#include <iomanip> // std::setw,


namespace SKV::XML::VATReturns {
	extern char const* ACCOUNT_VAT_CSV; // See bottom of this source file
}

namespace SKV {
    namespace XML {
        namespace VATReturns {
			Amount to_box_49_amount(FormBoxMap const& box_map) {
				BoxNos vat_box_nos{10,11,12,30,31,32,60,61,62,48};
				auto box_49_amount = std::accumulate(vat_box_nos.begin(),vat_box_nos.end(),Amount{},[&box_map](Amount acc,BoxNo box_no){
					if (box_map.contains(box_no)) {
						auto mats_sum = BAS::to_mdats_sum(box_map.at(box_no));
						acc += mats_sum;
						std::cout << "\n\tto_box_49_amount += [" << box_no << "]:" << mats_sum << " = " << acc;
					}
					return acc;
				});
				return box_49_amount;
			}


			Key::Paths account_vat_form_mapping() {
				Key::Paths result{};
				std::istringstream is{ACCOUNT_VAT_CSV};
				std::string row{};
				while (std::getline(is,row)) {
					result.push_back(Key::Path{row,';'});
				}
				return result;
			}

			BAS::AccountNos to_accounts(BoxNo box_no) {
				static auto const ps = account_vat_form_mapping();
				BAS::AccountNos result{};
				return std::accumulate(ps.begin(),ps.end(),BAS::AccountNos{},[&box_no](auto acc,Key::Path const& p){
					try {
						std::ostringstream os{};
						os << std::setfill('0') << std::setw(2) << box_no;
						if (p[2].find(os.str()) != std::string::npos) acc.push_back(std::stoi(p[0]));

					}
					catch (std::exception const& e) { std::cout << "\nDESIGN INSUFFICIENCY: to_accounts::lambda failed. Exception=" << std::quoted(e.what());}					
					return acc;
				});
				return result;
			}

			std::set<BAS::AccountNo> to_accounts(BoxNos const& box_nos) {
				std::set<BAS::AccountNo> result{};
				for (auto const& box_no : box_nos) {
					auto vat_account_nos = to_accounts(box_no);
					std::copy(vat_account_nos.begin(),vat_account_nos.end(),std::inserter(result,result.end()));
				}
				return result;
			}

			std::set<BAS::AccountNo> to_vat_accounts() {
				return to_accounts({10,11,12,30,31,32,60,61,62,48,49});
			}			

        } // namespace VATReturns
    } // namespace XML
} // namespace SKV

std::set<BAS::AccountNo> to_vat_returns_form_bas_accounts(SKV::XML::VATReturns::BoxNos const& box_nos) {
	return SKV::XML::VATReturns::to_accounts(box_nos);
}

std::set<BAS::AccountNo> const& to_vat_accounts() {
	static auto const vat_accounts = SKV::XML::VATReturns::to_vat_accounts(); // cache
	// Define in terms of how SKV VAT returns form defines linking to BAS Accounts for correct content
	return vat_accounts;
}

bool is_vat_returns_form_at(std::vector<SKV::XML::VATReturns::BoxNo> const& box_nos,BAS::anonymous::AccountTransaction const& at) {
	auto const& bas_account_nos = to_vat_returns_form_bas_accounts(box_nos);
	return bas_account_nos.contains(at.account_no);
}

bool is_vat_account(BAS::AccountNo account_no) {
	auto const& vat_accounts = to_vat_accounts();
	return vat_accounts.contains(account_no);
}

bool is_vat_account_at(BAS::anonymous::AccountTransaction const& at) {
	return is_vat_account(at.account_no);
};


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

namespace SKV { // SKV
	namespace XML { // SKV::XML
		namespace VATReturns { // SKV::XML::VATReturns
// Mapping between BAS Account numbers and VAT Returns form designation codes (SRU Codes as "bonus")
char const* ACCOUNT_VAT_CSV = R"(KONTO;BENÄMNING;MOMSKOD (MOMSRUTA);SRU
3305;Försäljning tjänster till land utanför EU;ÖTEU (40);7410
3521;Fakturerade frakter, EU-land;VTEU (35);7410
3108;Försäljning varor till annat EU-land, momsfri;VTEU (35);7410
2634;Utgående moms omvänd skattskyldighet, 6 %;UTFU3 (32);7369
2624;Utgående moms omvänd skattskyldighet, 12 %;UTFU2 (31);7369
2614;Utgående moms omvänd skattskyldighet, 25 %;UOS1 (30);7369
2635;Utgående moms import av varor, 6 %;UI6 (62);7369
2615;Utgående moms import av varor, 25 %;UI25 (60);7369
2625;Utgående moms import av varor, 12 %;UI12 (61);7369
2636;Utgående moms VMB 6 %;U3 (12);7369
2631;Utgående moms på försäljning inom Sverige, 6 %;U3 (12);7369
2630;Utgående moms, 6 %;U3 (12);7369
2633;Utgående moms för uthyrning, 6 %;U3 (12);7369
2632;Utgående moms på egna uttag, 6 %;U3 (12);7369
2626;Utgående moms VMB 12 %;U2 (11);7369
2622;Utgående moms på egna uttag, 12 %;U2 (11);7369
2621;Utgående moms på försäljning inom Sverige, 12 %;U2 (11);7369
2620;Utgående moms, 12 %;U2 (11);7369
2623;Utgående moms för uthyrning, 12 %;U2 (11);7369
2612;Utgående moms på egna uttag, 25 %;U1 (10);7369
2610;Utgående moms, 25 %;U1 (10);7369
2611;Utgående moms på försäljning inom Sverige, 25 %;U1 (10);7369
2616;Utgående moms VMB 25 %;U1 (10);7369
2613;Utgående moms för uthyrning, 25 %;U1 (10);7369
2650;Redovisningskonto för moms;R2 (49);7369
1650;Momsfordran;R1 (49);7261
3231;Försäljning inom byggsektorn, omvänd skattskyldighet moms;OTTU (41);7410
3003;Försäljning inom Sverige, 6 % moms;MP3 (05);7410
3403;Egna uttag momspliktiga, 6 %;MP3 (05);7410
3402;Egna uttag momspliktiga, 12 %;MP2 (05);7410
3002;Försäljning inom Sverige, 12 % moms;MP2 (05);7410
3401;Egna uttag momspliktiga, 25 %;MP1 (05);7410
3510;Fakturerat emballage;MP1 (05);7410
3600;Rörelsens sidointäkter (gruppkonto);MP1 (05);7410
3530;Fakturerade tull- och speditionskostnader m.m.;MP1 (05);7410
3520;Fakturerade frakter;MP1 (05);7410
3001;Försäljning inom Sverige, 25 % moms;MP1 (05);7410
3540;Faktureringsavgifter;MP1 (05);7410
3106;Försäljning varor till annat EU-land, momspliktig;MP1 (05);7410
3990;Övriga ersättningar och intäkter;MF (42);7413
3404;Egna uttag, momsfria;MF (42);7410
3004;Försäljning inom Sverige, momsfri;MF (42);7410
3980;Erhållna offentliga stöd m.m.;MF (42);7413
4516;Inköp av varor från annat EU-land, 12 %;IVEU (20);7512
4515;Inköp av varor från annat EU-land, 25 %;IVEU (20);7512
9021;Varuvärde Inlöp annat EG-land (Momsrapport ruta 20);IVEU (20);
4517;Inköp av varor från annat EU-land, 6 %;IVEU (20);7512
4415;Inköpta varor i Sverige, omvänd skattskyldighet, 25 % moms;IV (23);7512
4531;Inköp av tjänster från ett land utanför EU, 25 % moms;ITGLOB (22);7512
4532;Inköp av tjänster från ett land utanför EU, 12 % moms;ITGLOB (22);7512
4533;Inköp av tjänster från ett land utanför EU, 6 % moms;ITGLOB (22);7512
4537;Inköp av tjänster från annat EU-land, 6 %;ITEU (21);7512
4536;Inköp av tjänster från annat EU-land, 12 %;ITEU (21);7512
4535;Inköp av tjänster från annat EU-land, 25 %;ITEU (21);7512
4427;Inköpta tjänster i Sverige, omvänd skattskyldighet, 6 %;IT (24);7512
4426;Inköpta tjänster i Sverige, omvänd skattskyldighet, 12 %;IT (24);7512
2642;Debiterad ingående moms i anslutning till frivillig skattskyldighet;I (48);7369
2640;Ingående moms;I (48);7369
2647;Ingående moms omvänd skattskyldighet varor och tjänster i Sverige;I (48);7369
2641;Debiterad ingående moms;I (48);7369
2649;Ingående moms, blandad verksamhet;I (48);7369
2646;Ingående moms på uthyrning;I (48);7369
2645;Beräknad ingående moms på förvärv från utlandet;I (48);7369
3913;Frivilligt momspliktiga hyresintäkter;HFS (08);7413
3541;Faktureringsavgifter, EU-land;FTEU (39);7410
3308;Försäljning tjänster till annat EU-land;FTEU (39);7410
3542;Faktureringsavgifter, export;E (36);7410
3522;Fakturerade frakter, export;E (36);7410
3105;Försäljning varor till land utanför EU;E (36);7410
3211;Försäljning positiv VMB 25 %;BVMB (07);7410
3212;Försäljning negativ VMB 25 %;BVMB (07);7410
9022;Beskattningsunderlag vid import (Momsrapport Ruta 50);BI (50);
4545;Import av varor, 25 % moms;BI (50);7512
4546;Import av varor, 12 % moms;BI (50);7512
4547;Import av varor, 6 % moms;BI (50);7512
3740;Öres- och kronutjämning;A;7410)";
		} // namespace VATReturns
	} // namespace XML
} // namespace SKV 