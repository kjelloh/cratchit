#include "BASFramework.hpp"
#include "text/format.hpp" // operator<<(std::optional<std::string>),...
#include "tokenize.hpp"
#include <numeric> // std::accumulate,


// -----------------------
// BEGIN Balance

std::ostream& operator<<(std::ostream& os,Balance const& balance) {
	os << balance.account_no << " ib:" << balance.opening_balance << " period:" << balance.change << " " << balance.end_balance;
	return os;
}

std::ostream& operator<<(std::ostream& os,BalancesMap const& balances_map) {
	for (auto const& [date,balances] : balances_map) {
		os << date <<  " balances";
		for (auto const& balance : balances) {
			os << "\n\t" << balance;
		}
	}
	return os;
}

// END Balance
// -----------------------

namespace BAS {

	AccountTransactionMeta to_account_transaction_meta(BAS::MDJournalEntry const& mdje) {
		return AccountTransactionMeta{
			.date = mdje.defacto.date
		 ,.jem = mdje.meta
		 ,.caption = mdje.defacto.caption
		};
	}

	Amount to_mdats_sum(BAS::MDAccountTransactions const& mdats) {
		return std::accumulate(mdats.begin(),mdats.end(),Amount{},[](Amount acc,BAS::MDAccountTransaction const& mat){
			acc += mat.defacto.amount;
			return acc;
		});
	}
} // namespace BAS

namespace BAS {

    // Creates pairs of iterators for ranges of unbroken lines
    template <typename I>
    std::vector<std::pair<I,I>> to_ranges(std::vector<I> line_nos) {
        std::vector<std::pair<I,I>> result{};
        if (line_nos.size()>0) {
            I begin{line_nos[0]}; 
            I previous{begin};
            for (auto line_ix : line_nos) {
                if (line_ix > previous+1) {
                    // Broken sequence - push previous one
                    result.push_back({begin,previous});
                    begin = line_ix;
                }
                previous = line_ix;
            }
            if (previous > begin) result.push_back({begin,previous});
        }
        return result;
    }

    template <typename I>
    std::ostream& operator<<(std::ostream& os,std::vector<std::pair<I,I>> const& rr) {
        for (auto const& r : rr) {
            if (r.first == r.second) os << " " << r.first;
            else os << " [" << r.first << ".." << r.second << "]";
        }
        return os;
    }

	void parse_bas_account_plan_csv(std::istream& in,std::ostream& prompt) {
		std::string entry{};
		int line_ix{0};
		std::map<int,int> count_distribution{};
		using field_ix = int;
		using line_no = int;
		std::map<field_ix,std::map<std::string,std::vector<line_no>>> fields_map{};
		while (std::getline(in,entry)) {
			prompt << "\n" << line_ix << entry;
			auto fields = tokenize::splits(entry,';',tokenize::eAllowEmptyTokens::YES);
			int field_ix{0};
			prompt << "\n\tcount:" << fields.size();
			++count_distribution[fields.size()];
			for (auto const& field : fields) {
				prompt << "\n\t  " << field_ix << ":" << std::quoted(field);
				fields_map[field_ix][field].push_back(line_ix);
				++field_ix;
			}
			++line_ix;
		}
		prompt << "\nField Count distribution";
		for (auto const& entry : count_distribution) prompt << "\nfield count:" << entry.first << " entry count:" << entry.second;
		prompt << "\nField Distribution";
		for (auto const& [field_ix,field_map] : fields_map) {
			prompt << "\n\tindex:" << field_ix;
			for (auto const& [field,line_nos] : field_map) {
				prompt << "\n\t  field:" << std::quoted(field) << " line:";
				auto rr = to_ranges(line_nos);
				prompt << rr;
				if (line_nos.size()>0) {
					line_no begin{line_nos[0]}; 
					line_no previous{begin};
					for (auto line_ix : line_nos) {
						if (line_ix > previous+1) {
							// Broken sequence - log previous one
							if (begin == previous) prompt << " " << begin;
							else prompt << " [" << begin << ".." << previous << "]";
							begin = line_ix;
						}
						// prompt << "<" << line_ix << ">";
						previous = line_ix;
					}
					// Log last range
					if (begin == previous) prompt << " " << previous;
					else prompt << " [" << begin << ".." << previous << "]";
				}
			}
		}
	}
} // namespace BAS

// -----------------------------
// BEGIN Accounting

Amount to_positive_gross_transaction_amount(BAS::anonymous::JournalEntry const& aje) {
	Amount result = std::accumulate(aje.account_transactions.begin(),aje.account_transactions.end(),Amount{},[](Amount acc,BAS::anonymous::AccountTransaction const& account_transaction){
		acc += (account_transaction.amount>0)?account_transaction.amount:0;
		return acc;
	});
	return result;
}

Amount to_negative_gross_transaction_amount(BAS::anonymous::JournalEntry const& aje) {
	Amount result = std::accumulate(aje.account_transactions.begin(),aje.account_transactions.end(),Amount{},[](Amount acc,BAS::anonymous::AccountTransaction const& account_transaction){
		acc += (account_transaction.amount<0)?account_transaction.amount:0;
		return acc;
	});
	return result;
}

bool does_balance(BAS::anonymous::JournalEntry const& aje) {
	auto positive_gross_transaction_amount = to_positive_gross_transaction_amount(aje);
	auto negative_gross_amount = to_negative_gross_transaction_amount(aje);
	// std::cout << "\ndoes_balance: positive_gross_transaction_amount=" << positive_gross_transaction_amount << "  negative_gross_amount=" << negative_gross_amount;
	// std::cout << "\n\tsum=" << positive_gross_transaction_amount + negative_gross_amount;
	// TODO: FIX Ronding errors somewhere that makes the positive and negative sum to be some infinitesimal value NOT zero ...
	return (BAS::to_cents_amount(positive_gross_transaction_amount + negative_gross_amount) == 0); // Fix for amounts not correct to the cents...
}

OptionalAmount to_gross_transaction_amount(BAS::anonymous::JournalEntry const& aje) {
	// std::cout << "\nto_gross_transaction_amount: " << aje;
	OptionalAmount result{};
	if (does_balance(aje)) {
		result = to_positive_gross_transaction_amount(aje); // Pick the positive alternative
	}
	else if (aje.account_transactions.size() == 1) {
		result = abs(aje.account_transactions.front().amount);
	}
	else {
		// Does NOT balance, and more than one account transaction.
		// Define the gross amount as the largest account absolute transaction amount
		auto max_at_iter = std::max_element(aje.account_transactions.begin(),aje.account_transactions.end(),[](auto const& at1,auto const& at2) {
			return abs(at1.amount) < abs(at2.amount);
		});
		if (max_at_iter != aje.account_transactions.end()) result = abs(max_at_iter->amount);
	}
	// if (result) std::cout << "\n\t==> " << *result;
	return result;
}

BAS::anonymous::OptionalAccountTransaction gross_account_transaction(BAS::anonymous::JournalEntry const& aje) {
	BAS::anonymous::OptionalAccountTransaction result{};
	auto trans_amount = to_positive_gross_transaction_amount(aje);
	auto iter = std::find_if(aje.account_transactions.begin(),aje.account_transactions.end(),[&trans_amount](auto const& at){
		return abs(at.amount) == trans_amount;
	});
	if (iter != aje.account_transactions.end()) result = *iter;
	return result;
}

Amount to_account_transactions_sum(BAS::anonymous::AccountTransactions const& ats) {
	Amount result = std::accumulate(ats.begin(),ats.end(),Amount{},[](Amount acc,BAS::anonymous::AccountTransaction const& at){
		acc += at.amount;
		return acc;
	});
	return result;
}

// END Accounting
// -----------------------------

// -----------------------------
// BGIN Accounting IO

std::ostream& operator<<(std::ostream& os,BAS::anonymous::AccountTransaction const& at) {
	if (BAS::global_account_metas().contains(at.account_no)) os << std::quoted(BAS::global_account_metas().at(at.account_no).name) << ":";
	os << at.account_no;
	os << " " << at.transtext;
	os << " " << to_string(at.amount); // When amount is double there will be no formatting of the amount to ensure two decimal cents digits
	return os;
};

std::string to_string(BAS::anonymous::AccountTransaction const& at) {
	std::ostringstream os{};
	os << at;
	return os.str();
};

std::ostream& operator<<(std::ostream& os,BAS::anonymous::AccountTransactions const& ats) {
	for (auto const& at : ats) {
		// os << "\n\t" << at; 
		os << "\n    " << at; 
	}
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::anonymous::JournalEntry const& aje) {
	os << std::quoted(aje.caption) << " " << aje.date;
	os << aje.account_transactions;
	return os;
};

std::ostream& operator<<(std::ostream& os,BAS::OptionalVerNo const& verno) {
	if (verno and *verno!=0) os << *verno;
	else os << " _ ";
	return os;
}

std::ostream& operator<<(std::ostream& os,std::optional<bool> flag) {
	auto ch = (flag)?((*flag)?'*':' '):' '; // '*' if known and set, else ' '
	os << ch;
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::WeakJournalEntryMeta const& jem) {
	os << jem.unposted_flag << jem.series << jem.verno;
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::AccountTransactionMeta const& atm) {
	os << atm.date << " " << atm.jem /* << atm.caption */;
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::MDAccountTransaction const& mdat) {
	os << mdat.meta << " " << mdat.defacto;
	return os;
};

std::ostream& operator<<(std::ostream& os,BAS::MDJournalEntry const& mdje) {
	os << mdje.meta << " " << mdje.defacto;
	return os;
}

std::ostream& operator<<(std::ostream& os,BAS::MDJournalEntries const& mdjes) {
	for (auto const& mdje : mdjes) {
		os << "\n" << mdje;
	}
	return os;
};

std::string to_string(BAS::anonymous::JournalEntry const& aje) {
	std::ostringstream os{};
	os << aje;
	return os.str();
};

std::string to_string(BAS::MDJournalEntry const& mdje) {
	std::ostringstream os{};
	os << mdje;
	return os.str();
};

// END Accounting IO
// -----------------------------

// extern text string literals
namespace BAS {
	// The following string literal is the "raw" output of:
	// 1) In macOS Numbers opening excel file downloaded from https://www.bas.se/kontoplaner/
	// 2) Exported as csv-file
	// See project resource ./resources/Kontoplan-2022.csv
	char const* bas_2022_account_plan_csv{R"(;Kontoplan – BAS 2022;;;;;;;;
;;;;;;;;;
;;;;|;;= Ändring eller tillägg jämfört med föregående år. Mer information finns på BAS webbplats (bas.se).;;;
;;;;■;;= Kontot ingår i det urval av konton som för de flesta företag är tillräckligt för en grundläggande bokföring.;;;
;;;;[Ej K2];;= Kontot används inte av de företag som valt att tillämpa K2-regler.;;;
;;;;;;;;;
;;;Huvudkonton;;;Underkonton;;;
;;;;;;;;;
;;1;Tillgångar;;;;;;
;;10;Immateriella anläggningstillgångar;;;;;;
;[Ej K2];101;Utvecklingsutgifter;[Ej K2];1010;Utvecklingsutgifter;;;
;;;;[Ej K2];1011;Balanserade utgifter för forskning och utveckling;;;
;;;;[Ej K2];1012;Balanserade utgifter för programvaror;;;
;;;;[Ej K2];1018;Ackumulerade nedskrivningar på balanserade utgifter;;;
;;;;[Ej K2];1019;Ackumulerade avskrivningar på balanserade utgifter;;;
;;102;Koncessioner m.m.;;1020;Koncessioner m.m.;;;
;;;;;1028;Ackumulerade nedskrivningar på koncessioner m.m.;;;
;;;;;1029;Ackumulerade avskrivningar på koncessioner m.m.;;;
;■;103;Patent;■;1030;Patent;;;
;;;;;1038;Ackumulerade nedskrivningar på patent;;;
;;;;■;1039;Ackumulerade avskrivningar på patent;;;
;;104;Licenser;;1040;Licenser;;;
;;;;;1048;Ackumulerade nedskrivningar på licenser;;;
;;;;;1049;Ackumulerade avskrivningar på licenser;;;
;;105;Varumärken;;1050;Varumärken;;;
;;;;;1058;Ackumulerade nedskrivningar på varumärken;;;
;;;;;1059;Ackumulerade avskrivningar på varumärken;;;
;■;106;Hyresrätter, tomträtter och liknande;■;1060;Hyresrätter, tomträtter och liknande;;;
;;;;;1068;Ackumulerade nedskrivningar på hyresrätter, tomträtter och liknande;;;
;;;;■;1069;Ackumulerade avskrivningar på hyresrätter, tomträtter och liknande;;;
;;107;Goodwill;;1070;Goodwill;;;
;;;;;1078;Ackumulerade nedskrivningar på goodwill;;;
;;;;;1079;Ackumulerade avskrivningar på goodwill;;;
;;108;Förskott för immateriella anläggningstillgångar;;1080;Förskott för immateriella anläggningstillgångar;;;
;;;;[Ej K2];1081;Pågående projekt för immateriella anläggningstillgångar;;;
;;;;;1088;Förskott för immateriella anläggningstillgångar;;;
;;11;Byggnader och mark;;;;;;
;■;111;Byggnader;■;1110;Byggnader;;;
;;;;;1111;Byggnader på egen mark;;;
;;;;;1112;Byggnader på annans mark;;;
;;;;;1118;Ackumulerade nedskrivningar på byggnader;;;
;;;;■;1119;Ackumulerade avskrivningar på byggnader;;;
;;112;Förbättringsutgifter på annans fastighet;;1120;Förbättringsutgifter på annans fastighet;;;
;;;;;1129;Ackumulerade avskrivningar på förbättringsutgifter på annans fastighet;;;
;■;113;Mark;■;1130;Mark;;;
;;114;Tomter och obebyggda markområden;;1140;Tomter och obebyggda markområden;;;
;■;115;Markanläggningar;■;1150;Markanläggningar;;;
;;;;;1158;Ackumulerade nedskrivningar på markanläggningar;;;
;;;;■;1159;Ackumulerade avskrivningar på markanläggningar;;;
;;118;Pågående nyanläggningar och förskott för byggnader och mark;;1180;Pågående nyanläggningar och förskott för byggnader och mark;;;
;;;;;1181;Pågående ny-, till- och ombyggnad;;;
;;;;;1188;Förskott för byggnader och mark;;;
;;12;Maskiner och inventarier;;;;;;
;■;121;Maskiner och andra tekniska anläggningar;■;1210;Maskiner och andra tekniska anläggningar;;;
;;;;;1211;Maskiner;;;
;;;;;1213;Andra tekniska anläggningar;;;
;;;;;1218;Ackumulerade nedskrivningar på maskiner och andra tekniska anläggningar;;;
;;;;■;1219;Ackumulerade avskrivningar på maskiner och andra tekniska anläggningar;;;
;■;122;Inventarier och verktyg;■;1220;Inventarier och verktyg;;;
;;;;;1221;Inventarier;;;
;;;;;1222;Byggnadsinventarier;;;
;;;;;1223;Markinventarier;;;
;;;;;1225;Verktyg;;;
;;;;;1228;Ackumulerade nedskrivningar på inventarier och verktyg;;;
;;;;■;1229;Ackumulerade avskrivningar på inventarier och verktyg;;;
;;123;Installationer;;1230;Installationer;;;
;;;;;1231;Installationer på egen fastighet;;;
;;;;;1232;Installationer på annans fastig het;;;
;;;;;1238;Ackumulerade nedskrivningar på installationer;;;
;;;;;1239;Ackumulerade avskrivningar på installationer;;;
;■;124;Bilar och andra transportmedel;■;1240;Bilar och andra transportmedel;;;
;;;;;1241;Personbilar;;;
;;;;;1242;Lastbilar;;;
;;;;;1243;Truckar;;;
;;;;;1244;Arbetsmaskiner;;;
;;;;;1245;Traktorer;;;
;;;;;1246;Motorcyklar, mopeder och skotrar;;;
;;;;;1247;Båtar, flygplan och helikoptrar;;;
;;;;;1248;Ackumulerade nedskrivningar på bilar och andra transportmedel;;;
;;;;■;1249;Ackumulerade avskrivningar på bilar och andra transportmedel;;;
;■;125;Datorer;■;1250;Datorer;;;
;;;;;1251;Datorer, företaget;;;
;;;;;1257;Datorer, personal;;;
;;;;;1258;Ackumulerade nedskrivningar på datorer;;;
;;;;■;1259;Ackumulerade avskrivningar på datorer;;;
;[Ej K2];126;Leasade tillgångar;[Ej K2];1260;Leasade tillgångar;;;
;;;;[Ej K2];1269;Ackumulerade avskrivningar på leasade tillgångar;;;
;;128;Pågående nyanläggningar och förskott för maskiner och inventarier;;1280;Pågående nyanläggningar och förskott för maskiner och inventarier;;;
;;;;;1281;Pågående nyanläggningar, maskiner och inventarier;;;
;;;;;1288;Förskott för maskiner och inventarier;;;
;■;129;Övriga materiella anläggningstillgångar;■;1290;Övriga materiella anläggningstillgångar;;;
;;;;■;1291;Konst och liknande tillgångar;;;
;;;;;1292;Djur som klassificeras som anläggningstillgång;;;
;;;;;1298;Ackumulerade nedskrivningar på övriga materiella anläggningstillgångar;;;
;;;;■;1299;Ackumulerade avskrivningar på övriga materiella anläggningstillgångar;;;
;;13;Finansiella anläggningstillgångar;;;;;;
;;131;Andelar i koncernföretag;;1310;Andelar i koncernföretag;;;
;;;;;1311;Aktier i noterade svenska koncernföretag;;;
;;;;;1312;Aktier i onoterade svenska koncernföretag;;;
;;;;;1313;Aktier i noterade utländska koncernföretag;;;
;;;;;1314;Aktier i onoterade utländska koncernföretag;;;
;;;;;1316;Andra andelar i svenska koncernföretag;;;
;;;;;1317;Andra andelar i utländska koncernförertag;;;
;;;;;1318;Ackumulerade nedskrivningar av andelar i koncernföretag;;;
;;132;Långfristiga fordringar hos koncernföretag;;1320;Långfristiga fordringar hos koncernföretag;;;
;;;;;1321;Långfristiga fordringar hos moderföretag;;;
;;;;;1322;Långfristiga fordringar hos dotterföretag;;;
;;;;;1323;Långfristiga fordringar hos andra koncernföretag;;;
;;;;;1328;Ackumulerade nedskrivningar av långfristiga fordringar hos koncernföretag;;;
;;133;Andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;1330;Andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;1331;Andelar i intresseföretag;;;
;;;;;1332;Ackumulerade nedskrivningar av andelar i intresseföretag;;;
;;;;;1333;Andelar i gemensamt styrda företag;;;
;;;;;1334;Ackumulerade nedskrivningar av andelar i gemensamt styrda företag;;;
;;;;;1336;Andelar i övriga företag som det finns ett ägarintresse i;;;
;;;;;1337;Ackumulerade nedskrivningar av andelar i övriga företag som det finns ett ägarintresse i;;;
;;;;;1338;Ackumulerade nedskrivningar av andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;134;Långfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;1340;Långfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;1341;Långfristiga fordringar hos intresseföretag;;;
;;;;;1342;Ackumulerade nedskrivningar av långfristiga fordringar hos intresseföretag;;;
;;;;;1343;Långfristiga fordringar hos gemensamt styrda företag;;;
;;;;;1344;Ackumulerade nedskrivningar av långfristiga fordringar hos gemensamt styrda företag;;;
;;;;;1346;Långfristiga fordringar hos övriga företag som det finns ett ägarintresse i;;;
;;;;;1347;Ackumulerade nedskrivningar av långfristiga fordringar hos övriga företag som det finns ett ägarintresse i;;;
;;;;;1348;Ackumulerade nedskrivningar av långfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;■;135;Andelar och värdepapper i andra företag;■;1350;Andelar och värdepapper i andra företag;;;
;;;;;1351;Andelar i noterade företag;;;
;;;;;1352;Andra andelar;;;
;;;;;1353;Andelar i bostadsrättsföreningar;;;
;;;;;1354;Obligationer;;;
;;;;;1356;Andelar i ekonomiska föreningar, övriga företag;;;
;;;;;1357;Andelar i handelsbolag, andra företag;;;
;;;;;1358;Ackumulerade nedskrivningar av andra andelar och värdepapper;;;
;;136;Lån till delägare eller närstående enligt ABL, långfristig del;;1360;Lån till delägare eller närstående enligt ABL, långfristig del;;;
;;;;;1369;Ackumulerade nedskrivningar av lån till delägare eller närstående enligt ABL, långfristig del;;;
;[Ej K2];137;Uppskjuten skattefordran;[Ej K2];1370;Uppskjuten skattefordran;;;
;■;138;Andra långfristiga fordringar;■;1380;Andra långfristiga fordringar;;;
;;;;;1381;Långfristiga reversfordringar;;;
;;;;;1382;Långfristiga fordringar hos anställda;;;
;;;;;1383;Lämnade depositioner, långfristiga;;;
;;;;;1384;Derivat;;;
;;;;;1385;Kapitalförsäkring;|;;
;;;;;1387;Långfristiga kontraktsfordringar;;;
;;;;;1388;Långfristiga kundfordringar;;;
;;;;;1389;Ackumulerade nedskrivningar av andra långfristiga fordringar;;;
;;14;Lager, produkter i arbete och pågående arbeten;;;;;;
;■;141;Lager av råvaror;■;1410;Lager av råvaror;;;
;;;;■;1419;Förändring av lager av råvaror;;;
;;142;Lager av tillsatsmaterial och förnödenheter;;1420;Lager av tillsatsmaterial och förnödenheter;;;
;;;;;1429;Förändring av lager av tillsatsmaterial och förnödenheter;;;
;■;144;Produkter i arbete;■;1440;Produkter i arbete;;;
;;;;■;1449;Förändring av produkter i arbete;;;
;■;145;Lager av färdiga varor;■;1450;Lager av färdiga varor;;;
;;;;■;1459;Förändring av lager av färdiga varor;;;
;■;146;Lager av handelsvaror;■;1460;Lager av handelsvaror;;;
;;;;;1465;Lager av varor VMB;;;
;;;;;1466;Nedskrivning av varor VMB;;;
;;;;;1467;Lager av varor VMB förenklad;;;
;;;;■;1469;Förändring av lager av handelsvaror;;;
;■;147;Pågående arbeten;■;1470;Pågående arbeten;;;
;;;;;1471;Pågående arbeten, nedlagda kostnader;;;
;;;;;1478;Pågående arbeten, fakturering;;;
;;;;■;1479;Förändring av pågående arbeten;;;
;■;148;Förskott för varor och tjänster;■;1480;Förskott för varor och tjänster;;;
;;;;;1481;Remburser;;;
;;;;;1489;Övriga förskott till leverantörer;;;
;■;149;Övriga lagertillgångar;■;1490;Övriga lagertillgångar;;;
;;;;;1491;Lager av värdepapper;;;
;;;;;1492;Lager av fastigheter;;;
;;;;;1493;Djur som klassificeras som omsättningstillgång;;;
;;15;Kundfordringar;;;;;;
;■;151;Kundfordringar;■;1510;Kundfordringar;;;
;;;;;1511;Kundfordringar;;;
;;;;;1512;Belånade kundfordringar (factoring);;;
;;;;■;1513;Kundfordringar – delad faktura;;;
;;;;;1516;Tvistiga kundfordringar;;;
;;;;;1518;Ej reskontraförda kundfordringar;;;
;;;;■;1519;Nedskrivning av kundfordringar;;;
;;152;Växelfordringar;;1520;Växelfordringar;;;
;;;;;1525;Osäkra växelfordringar;;;
;;;;;1529;Nedskrivning av växelfordringar;;;
;;153;Kontraktsfordringar;;1530;Kontraktsfordringar;;;
;;;;;1531;Kontraktsfordringar;;;
;;;;;1532;Belånade kontraktsfordringar;;;
;;;;;1536;Tvistiga kontraktsfordringar;;;
;;;;;1539;Nedskrivning av kontraktsfordringar;;;
;;155;Konsignationsfordringar;;1550;Konsignationsfordringar;;;
;;156;Kundfordringar hos koncernföretag;;1560;Kundfordringar hos koncernföretag;;;
;;;;;1561;Kundfordringar hos moderföretag;;;
;;;;;1562;Kundfordringar hos dotterföretag;;;
;;;;;1563;Kundfordringar hos andra koncernföretag;;;
;;;;;1568;Ej reskontraförda kundfordringar hos koncernföretag;;;
;;;;;1569;Nedskrivning av kundfordringar hos koncernföretag;;;
;;157;Kundfordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;1570;Kundfordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;1571;Kundfordringar hos intresseföretag;;;
;;;;;1572;Kundfordringar hos gemensamt styrda företag;;;
;;;;;1573;Kundfordringar hos övriga företag som det finns ett ägarintresse i;;;
;■;158;Fordringar för kontokort och kuponger;■;1580;Fordringar för kontokort och kuponger;;;
;;16;Övriga kortfristiga fordringar;;;;;;
;■;161;Kortfristiga fordringar hos anställda;■;1610;Kortfristiga fordringar hos anställda;;;
;;;;;1611;Reseförskott;;;
;;;;;1612;Kassaförskott;;;
;;;;;1613;Övriga förskott;;;
;;;;;1614;Tillfälliga lån till anställda;;;
;;;;;1619;Övriga fordringar hos anställda;;;
;;162;Upparbetad men ej fakturerad intäkt;;1620;Upparbetad men ej fakturerad intäkt;;;
;■;163;Avräkning för skatter och avgifter (skattekonto);■;1630;Avräkning för skatter och avgifter (skattekonto);;;
;■;164;Skattefordringar;■;1640;Skattefordringar;;;
;■;165;Momsfordran;■;1650;Momsfordran;;;
;;166;Kortfristiga fordringar hos koncernföretag;;1660;Kortfristiga fordringar hos koncernföretag;;;
;;;;;1661;Kortfristiga fordringar hos moderföretag;;;
;;;;;1662;Kortfristiga fordringar hos dotterföretag;;;
;;;;;1663;Kortfristiga fordringar hos andra koncernföretag;;;
;;167;Kortfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;1670;Kortfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;1671;Kortfristiga fordringar hos intresseföretag;;;
;;;;;1672;Kortfristiga fordringar hos gemensamt styrda företag;;;
;;;;;1673;Kortfristiga fordringar hos övriga företag som det finns ett ägarintresse i;;;
;■;168;Andra kortfristiga fordringar;■;1680;Andra kortfristiga fordringar;;;
;;;;;1681;Utlägg för kunder;;;
;;;;;1682;Kortfristiga lånefordringar;;;
;;;;;1683;Derivat;;;
;;;;;1684;Kortfristiga fordringar hos leverantörer;;;
;;;;;1685;Kortfristiga fordringar hos delägare eller närstående;;;
;;;;;1687;Kortfristig del av långfristiga fordringar;;;
;;;;;1688;Fordran arbetsmarknadsförsäkringar;;;
;;;;;1689;Övriga kortfristiga fordringar;;;
;;169;Fordringar för tecknat men ej inbetalt aktiekapital;;1690;Fordringar för tecknat men ej inbetalt aktiekapital;;;
;;17;Förutbetalda kostnader och upplupna intäkter;;;;;;
;■;171;Förutbetalda hyreskostnader;■;1710;Förutbetalda hyreskostnader;;;
;■;172;Förutbetalda leasingavgifter;■;1720;Förutbetalda leasingavgifter;;;
;■;173;Förutbetalda försäkringspremier;■;1730;Förutbetalda försäkringspremier;;;
;■;174;Förutbetalda räntekostnader;■;1740;Förutbetalda räntekostnader;;;
;■;175;Upplupna hyresintäkter;■;1750;Upplupna hyresintäkter;;;
;■;176;Upplupna ränteintäkter;■;1760;Upplupna ränteintäkter;;;
;;177;Tillgångar av kostnadsnatur;;1770;Tillgångar av kostnadsnatur;;;
;;178;Upplupna avtalsintäkter;;1780;Upplupna avtalsintäkter;;;
;■;179;Övriga förutbetalda kostnader och upplupna intäkter;■;1790;Övriga förutbetalda kostnader och upplupna intäkter;;;
;;18;Kortfristiga placeringar;;;;;;
;■;181;Andelar i börsnoterade företag;■;1810;Andelar i börsnoterade företag;;;
;;182;Obligationer;;1820;Obligationer;;;
;;183;Konvertibla skuldebrev;;1830;Konvertibla skuldebrev;;;
;;186;Andelar i koncernföretag, kortfristigt;;1860;Andelar i koncernföretag, kortfristigt;;;
;■;188;Andra kortfristiga placeringar;■;1880;Andra kortfristiga placeringar;;;
;;;;;1886;Derivat;;;
;;;;;1889;Andelar i övriga företag;;;
;■;189;Nedskrivning av kortfristiga placeringar;■;1890;Nedskrivning av kortfristiga placeringar;;;
;;19;Kassa och bank;;;;;;
;■;191;Kassa;■;1910;Kassa;;;
;;;;;1911;Huvudkassa;;;
;;;;;1912;Kassa 2;;;
;;;;;1913;Kassa 3;;;
;■;192;PlusGiro;■;1920;PlusGiro;;;
;■;193;Företagskonto/checkkonto/affärskonto;■;1930;Företagskonto/checkkonto/affärskonto;;;
;■;194;Övriga bankkonton;■;1940;Övriga bankkonton;;;
;;195;Bankcertifikat;;1950;Bankcertifikat;;;
;;196;Koncernkonto moderföretag;;1960;Koncernkonto moderföretag;;;
;;197;Särskilda bankkonton;;1970;Särskilda bankkonton;;;
;;;;;1972;Upphovsmannakonto;;;
;;;;;1973;Skogskonto;;;)"
R"(;;;;; 1974; Spärrade bankmedel;;;
;;;;;1979;Övriga särskilda bankkonton;;;
;;198;Valutakonton;;1980;Valutakonton;;;
;;199;Redovisningsmedel;;1990;Redovisningsmedel;;;
;;2;Eget kapital och skulder;;;;;;
;;20;Eget kapital;;;;;;
;;201;Eget kapital (enskild firma);■;2010;Eget kapital;;;
;;;;■;2011;Egna varuuttag;;;
;;;;■;2013;Övriga egna uttag;;;
;;;;■;2017;Årets kapitaltillskott;;;
;;;;■;2018;Övriga egna insättningar;;;
;;;;■;2019;Årets resultat;;;
;;201;Eget kapital, delägare 1;■;2010;Eget kapital;;;
;;;;■;2011;Egna varuuttag;;;
;;;;■;2013;Övriga egna uttag;;;
;;;;■;2017;Årets kapitaltillskott;;;
;;;;■;2018;Övriga egna insättningar;;;
;;;;■;2019;Årets resultat, delägare 1;;;
;;202;Eget kapital, delägare 2;■;2020;Eget kapital;;;
;;;;■;2021;Egna varuuttag;;;
;;;;■;2023;Övriga egna uttag;;;
;;;;■;2027;Årets kapitaltillskott;;;
;;;;■;2028;Övriga egna insättningar;;;
;;;;■;2029;Årets resultat, delägare 2;;;
;;203;Eget kapital, delägare 3;■;2030;Eget kapital;;;
;;;;■;2031;Egna varuuttag;;;
;;;;■;2033;Övriga egna uttag;;;
;;;;■;2037;Årets kapitaltillskott;;;
;;;;■;2038;Övriga egna insättningar;;;
;;;;■;2039;Årets resultat, delägare 3;;;
;;204;Eget kapital, delägare 4;■;2040;Eget kapital;;;
;;;;■;2041;Egna varuuttag;;;
;;;;■;2043;Övriga egna uttag;;;
;;;;■;2047;Årets kapitaltillskott;;;
;;;;■;2048;Övriga egna insättningar;;;
;;;;■;2049;Årets resultat, delägare 4;;;
;;205;Avsättning till expansionsfond;;2050;Avsättning till expansionsfond;;;
;■;206;Eget kapital i ideella föreningar, stiftelser och registrerade trossamfund;■;2060;Eget kapital i ideella föreningar, stiftelser och registrerade trossamfund;;;
;;;;;2061;Eget kapital/stiftelsekapital/grundkapital;;;
;;;;;2065;Förändring i fond för verkligt värde;;;
;;;;;2066;Värdesäkringsfond;;;
;;;;;2067;Balanserad vinst eller förlust/balanserat kapital;;;
;;;;;2068;Vinst eller förlust från föregående år;;;
;;;;;2069;Årets resultat;;;
;■;207;Ändamålsbestämda medel;■;2070;Ändamålsbestämda medel;;;
;;;;;2071;Ändamål 1;;;
;;;;;2072;Ändamål 2;;;
;;208;Bundet eget kapital;;2080;Bundet eget kapital;;;
;;;;■;2081;Aktiekapital;;;
;;;;;2082;Ej registrerat aktiekapital;;;
;;;;■;2083;Medlemsinsatser;;;
;;;;;2084;Förlagsinsatser;;;
;;;;;2085;Uppskrivningsfond;;;
;;;;■;2086;Reservfond;;;
;;;;;2087;Insatsemission;;;
;;;;;2087;Bunden överkursfond;;;
;;;;;2088;Fond för yttre underhåll;;;
;;;;[Ej K2];2089;Fond för utvecklingsutgifter;;;
;■;209;Fritt eget kapital;■;2090;Fritt eget kapital;;;
;;;;■;2091;Balanserad vinst eller förlust;;;
;;;;[Ej K2];2092;Mottagna/lämnade koncernbidrag;;;
;;;;;2093;Erhållna aktieägartillskott;;;
;;;;;2094;Egna aktier;;;
;;;;;2095;Fusionsresultat;;;
;;;;[Ej K2];2096;Fond för verkligt värde;;;
;;;;;2097;Fri överkursfond;;;
;;;;■;2098;Vinst eller förlust från föregående år;;;
;;;;■;2099;Årets resultat;;;
;;21;Obeskattade reserver;;;;;;
;;211;Periodiseringsfonder;;2110;Periodiseringsfonder;;;
;■;212;Periodiseringsfond 2020;■;2120;Periodiseringsfond 2020;;;
;;;;■;2121;Periodiseringsfond 2021;;;
;;;;■;2122;Periodiseringsfond 2022;;;
;;;;■;2123;Periodiseringsfond 2023;;;
;;;;■;2125;Periodiseringsfond 2015;;;
;;;;■;2126;Periodiseringsfond 2016;;;
;;;;■;2127;Periodiseringsfond 2017;;;
;;;;■;2128;Periodiseringsfond 2018;;;
;;;;■;2129;Periodiseringsfond 2019;;;
;;213;Periodiseringsfond 2020 – nr 2;;2130;Periodiseringsfond 2020 – nr 2;;;
;;;;;2131;Periodiseringsfond 2021 – nr 2;;;
;;;;;2132;Periodiseringsfond 2022 – nr 2;;;
;;;;;2133;Periodiseringsfond 2023 – nr 2;;;
;;;;;2134;Periodiseringsfond 2024 – nr 2;;;
;;;;;2135;Periodiseringsfond 2015 – nr 2;;;
;;;;;2136;Periodiseringsfond 2016 – nr 2;;;
;;;;;2137;Periodiseringsfond 2017 – nr 2;;;
;;;;;2138;Periodiseringsfond 2018 – nr 2;;;
;;;;;2139;Periodiseringsfond 2019 – nr 2;;;
;■;215;Ackumulerade överavskrivningar;■;2150;Ackumulerade överavskrivningar;;;
;;;;;2151;Ackumulerade överavskrivningar på immateriella anläggningstillgångar;;;
;;;;;2152;Ackumulerade överavskrivningar på byggnader och markanläggningar;;;
;;;;;2153;Ackumulerade överavskrivningar på maskiner och inventarier;;;
;;216;Ersättningsfond;;2160;Ersättningsfond;;;
;;;;;2161;Ersättningsfond maskiner och inventarier;;;
;;;;;2162;Ersättningsfond byggnader och markanläggningar;;;
;;;;;2164;Ersättningsfond för djurlager i jordbruk och renskötsel;;;
;;219;Övriga obeskattade reserver;;2190;Övriga obeskattade reserver;;;
;;;;;2196;Lagerreserv;;;
;;;;;2199;Övriga obeskattade reserver;;;
;;22;Avsättningar;;;;;;
;■;221;Avsättningar för pensioner enligt tryggandelagen;■;2210;Avsättningar för pensioner enligt tryggandelagen;;;
;■;222;Avsättningar för garantier;■;2220;Avsättningar för garantier;;;
;;223;Övriga avsättningar för pensioner och liknande förpliktelser;;2230;Övriga avsättningar för pensioner och liknande förpliktelser;;;
;[Ej K2];224;Avsättningar för uppskjutna skatter;[Ej K2];2240;Avsättningar för uppskjutna skatter;;;
;;225;Övriga avsättningar för skatter;;2250;Övriga avsättningar för skatter;;;
;;;;;2252;Avsättningar för tvistiga skatter;;;
;;;;;2253;Avsättningar särskild löneskatt, deklarationspost;;;
;■;229;Övriga avsättningar;■;2290;Övriga avsättningar;;;
;;23;Långfristiga skulder;;;;;;
;;231;Obligations- och förlagslån;;2310;Obligations- och förlagslån;;;
;;232;Konvertibla lån och liknande;;2320;Konvertibla lån och liknande;;;
;;;;;2321;Konvertibla lån;;;
;;;;;2322;Lån förenade med optionsrätt;;;
;;;;;2323;Vinstandelslån;;;
;;;;;2324;Kapitalandelslån;;;
;■;233;Checkräkningskredit;■;2330;Checkräkningskredit;;;
;;;;;2331;Checkräkningskredit 1;;;
;;;;;2332;Checkräkningskredit 2;;;
;;234;Byggnadskreditiv;;2340;Byggnadskreditiv;;;
;■;235;Andra långfristiga skulder till kreditinstitut;■;2350;Andra långfristiga skulder till kreditinstitut;;;
;;;;;2351;Fastighetslån, långfristig del;;;
;;;;;2355;Långfristiga lån i utländsk valuta från kreditinstitut;;;
;;;;;2359;Övriga långfristiga lån från kreditinstitut;;;
;;236;Långfristiga skulder till koncernföretag;;2360;Långfristiga skulder till koncernföretag;;;
;;;;;2361;Långfristiga skulder till moderföretag;;;
;;;;;2362;Långfristiga skulder till dotterföretag;;;
;;;;;2363;Långfristiga skulder till andra koncernföretag;;;
;;237;Långfristiga skulder till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;2370;Långfristiga skulder till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;2371;Långfristiga skulder till intresseföretag;;;
;;;;;2372;Långfristiga skulder till gemensamt styrda företag;;;
;;;;;2373;Långfristiga skulder till övriga företag som det finns ett ägarintresse i;;;
;■;239;Övriga långfristiga skulder;■;2390;Övriga långfristiga skulder;;;
;;;;;2391;Avbetalningskontrakt, långfristig del;;;
;;;;;2392;Villkorliga långfristiga skulder;;;
;;;;■;2393;Lån från närstående personer, långfristig del;;;
;;;;;2394;Långfristiga leverantörskrediter;;;
;;;;;2395;Andra långfristiga lån i utländsk valuta;;;
;;;;;2396;Derivat;;;
;;;;;2397;Mottagna depositioner, långfristiga;;;
;;;;;2399;Övriga långfristiga skulder;;;
;;24;Kortfristiga skulder till kreditinstitut, kunder och leverantörer;;;;;;
;■;241;Andra kortfristiga låneskulder till kreditinstitut;■;2410;Andra kortfristiga låneskulder till kreditinstitut;;;
;;;;;2411;Kortfristiga lån från kreditinstitut;;;
;;;;;2412;Byggnadskreditiv, kortfristig del;;;
;;;;;2417;Kortfristig del av långfristiga skulder till kreditinstitut;;;
;;;;;2419;Övriga kortfristiga skulder till kreditinstitut;;;
;■;242;Förskott från kunder;■;2420;Förskott från kunder;;;
;;;;;2421;Ej inlösta presentkort;;;
;;;;;2429;Övriga förskott från kunder;;;
;;243;Pågående arbeten;;2430;Pågående arbeten;;;
;;;;;2431;Pågående arbeten, fakturering;;;
;;;;;2438;Pågående arbeten, nedlagda kostnader;;;
;;;;;2439;Beräknad förändring av pågående arbeten;;;
;■;244;Leverantörsskulder;■;2440;Leverantörsskulder;;;
;;;;;2441;Leverantörsskulder;;;
;;;;;2443;Konsignationsskulder;;;
;;;;;2445;Tvistiga leverantörsskulder;;;
;;;;;2448;Ej reskontraförda leverantörsskulder;;;
;;245;Fakturerad men ej upparbetad intäkt;;2450;Fakturerad men ej upparbetad intäkt;;;
;;246;Leverantörsskulder till koncernföretag;;2460;Leverantörsskulder till koncernföretag;;;
;;;;;2461;Leverantörsskulder till moderföretag;;;
;;;;;2462;Leverantörsskulder till dotterföretag;;;
;;;;;2463;Leverantörsskulder till andra koncernföretag;;;
;;247;Leverantörsskulder till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;2470;Leverantörsskulder till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;2471;Leverantörsskulder till intresseföretag;;;
;;;;;2472;Leverantörsskulder till gemensamt styrda företag;;;
;;;;;2473;Leverantörsskulder till övriga företag som det finns ett ägarintresse i;;;
;■;248;Checkräkningskredit, kortfristig;■;2480;Checkräkningskredit, kortfristig;;;
;■;249;Övriga kortfristiga skulder till kreditinstitut, kunder och leverantörer;■;2490;Övriga kortfristiga skulder till kreditinstitut, kunder och leverantörer;;;
;;;;;2491;Avräkning spelarrangörer;;;
;;;;;2492;Växelskulder;;;
;;;;;2499;Andra övriga kortfristiga skulder;;;
;;25;Skatteskulder;;;;;;
;■;251;Skatteskulder;■;2510;Skatteskulder;;;
;;;;;2512;Beräknad inkomstskatt;;;
;;;;;2513;Beräknad fastighetsskatt/fastighetsavgift;;;
;;;;;2514;Beräknad särskild löneskatt på pensionskostnader;;;
;;;;;2515;Beräknad avkastningsskatt;;;
;;;;;2517;Beräknad utländsk skatt;;;
;;;;;2518;Betald F-skatt;;;
;;26;Moms och särskilda punktskatter;;;;;;
;■;261;Utgående moms, 25 %;■;2610;Utgående moms, 25 %;;;
;;;;■;2611;Utgående moms på försäljning inom Sverige, 25 %;;;
;;;;■;2612;Utgående moms på egna uttag, 25 %;;;
;;;;■;2613;Utgående moms för uthyrning, 25 %;;;
;;;;■;2614;Utgående moms omvänd skattskyldighet, 25 %;;;
;;;;■;2615;Utgående moms import av varor, 25 %;;;
;;;;■;2616;Utgående moms VMB 25 %;;;
;;;;;2618;Vilande utgående moms, 25 %;;;
;■;262;Utgående moms, 12 %;■;2620;Utgående moms, 12 %;;;
;;;;■;2621;Utgående moms på försäljning inom Sverige, 12 %;;;
;;;;■;2622;Utgående moms på egna uttag, 12 %;;;
;;;;■;2623;Utgående moms för uthyrning, 12 %;;;
;;;;■;2624;Utgående moms omvänd skattskyldighet, 12 %;;;
;;;;■;2625;Utgående moms import av varor, 12 %;;;
;;;;■;2626;Utgående moms VMB 12 %;;;
;;;;;2628;Vilande utgående moms, 12 %;;;
;■;263;Utgående moms, 6 %;■;2630;Utgående moms, 6 %;;;
;;;;■;2631;Utgående moms på försäljning inom Sverige, 6 %;;;
;;;;■;2632;Utgående moms på egna uttag, 6 %;;;
;;;;■;2633;Utgående moms för uthyrning, 6 %;;;
;;;;■;2634;Utgående moms omvänd skattskyldighet, 6 %;;;
;;;;■;2635;Utgående moms import av varor, 6 %;;;
;;;;■;2636;Utgående moms VMB 6 %;;;
;;;;;2638;Vilande utgående moms, 6 %;;;
;■;264;Ingående moms;■;2640;Ingående moms;;;
;;;;■;2641;Debiterad ingående moms;;;
;;;;■;2642;Debiterad ingående moms i anslutning till frivillig skattskyldighet;;;
;;;;■;2645;Beräknad ingående moms på förvärv från utlandet;;;
;;;;■;2646;Ingående moms på uthyrning;;;
;;;;■;2647;Ingående moms omvänd skattskyldighet varor och tjänster i Sverige;;;
;;;;■;2648;Vilande ingående moms;;;
;;;;■;2649;Ingående moms, blandad verksamhet;;;
;■;265;Redovisningskonto för moms;■;2650;Redovisningskonto för moms;;;
;;266;Särskilda punktskatter;;2660;Särskilda punktskatter;;;
;;;;;2661;Reklamskatt;;;
;;;;;2669;Övriga punktskatter;;;
;;27;Personalens skatter, avgifter och löneavdrag;;;;;;
;■;271;Personalskatt;■;2710;Personalskatt;;;
;■;273;Lagstadgade sociala avgifter och särskild löneskatt;■;2730;Lagstadgade sociala avgifter och särskild löneskatt;;;
;;;;;2731;Avräkning lagstadgade sociala avgifter;;;
;;;;;2732;Avräkning särskild löneskatt;;;
;■;274;Avtalade sociala avgifter;■;2740;Avtalade sociala avgifter;;;
;;275;Utmätning i lön m.m.;;2750;Utmätning i lön m.m.;;;
;;276;Semestermedel;;2760;Semestermedel;;;
;;;;;2761;Avräkning semesterlöner;;;
;;;;;2762;Semesterlönekassa;;;
;■;279;Övriga löneavdrag;■;2790;Övriga löneavdrag;;;
;;;;;2791;Personalens intressekonto;;;
;;;;;2792;Lönsparande;;;
;;;;;2793;Gruppförsäkringspremier;;;
;;;;;2794;Fackföreningsavgifter;;;
;;;;;2795;Mätnings- och granskningsarvoden;;;
;;;;;2799;Övriga löneavdrag;;;
;;28;Övriga kortfristiga skulder;;;;;;
;;281;Avräkning för factoring och belånade kontraktsfordringar;;2810;Avräkning för factoring och belånade kontraktsfordringar;;;
;;;;;2811;Avräkning för factoring;;;
;;;;;2812;Avräkning för belånade kontraktsfordringar;;;
;■;282;Kortfristiga skulder till anställda;■;2820;Kortfristiga skulder till anställda;;;
;;;;;2821;Löneskulder;;;
;;;;;2822;Reseräkningar;;;
;;;;;2823;Tantiem, gratifikationer;;;
;;;;;2829;Övriga kortfristiga skulder till anställda;;;
;;283;Avräkning för annans räkning;;2830;Avräkning för annans räkning;;;
;■;284;Kortfristiga låneskulder;■;2840;Kortfristiga låneskulder;;;
;;;;;2841;Kortfristig del av långfristiga skulder;;;
;;;;;2849;Övriga kortfristiga låneskulder;;;
;;285;Avräkning för skatter och avgifter (skattekonto);;2850;Avräkning för skatter och avgifter (skattekonto);;;
;;;;;2852;Anståndsbelopp för moms, arbetsgivaravgifter och personalskatt;;;
;;286;Kortfristiga skulder till koncernföretag;;2860;Kortfristiga skulder till koncernföretag;;;
;;;;;2861;Kortfristiga skulder till moderföretag;;;
;;;;;2862;Kortfristiga skulder till dotterföretag;;;
;;;;;2863;Kortfristiga skulder till andra koncernföretag;;;
;;287;Kortfristiga skulder till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;2870;Kortfristiga skulder till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;2871;Kortfristiga skulder till intresseföretag;;;
;;;;;2872;Kortfristiga skulder till gemensamt styrda företag;;;
;;;;;2873;Kortfristiga skulder till övriga företag som det finns ett ägarintresse i;;;
;;288;Skuld erhållna bidrag;;2880;Skuld erhållna bidrag;;;
;■;289;Övriga kortfristiga skulder;■;2890;Övriga kortfristiga skulder;;;
;;;;;2891;Skulder under indrivning;;;
;;;;;2892;Inre reparationsfond/underhållsfond;;;
;;;;;2893;Skulder till närstående personer, kortfristig del;;;
;;;;;2895;Derivat (kortfristiga skulder);;;
;;;;;2897;Mottagna depositioner, kortfristiga;;;
;;;;;2898;Outtagen vinstutdelning;;;
;;;;;2899;Övriga kortfristiga skulder;;;
;;29;Upplupna kostnader och förutbetalda intäkter;;;;;;
;■;291;Upplupna löner;■;2910;Upplupna löner;;;
;;;;;2911;Löneskulder;;;
;;;;;2912;Ackordsöverskott;;;
;;;;;2919;Övriga upplupna löner;;;
;■;292;Upplupna semesterlöner;■;2920;Upplupna semesterlöner;;;
;;293;Upplupna pensionskostnader;;2930;Upplupna pensionskostnader;;;
;;;;;2931;Upplupna pensionsutbetalningar;;;
;■;294;Upplupna lagstadgade sociala och andra avgifter;■;2940;Upplupna lagstadgade sociala och andra avgifter;;;
;;;;;2941;Beräknade upplupna lagstadgade sociala avgifter;;;
;;;;;2942;Beräknad upplupen särskild löneskatt;;;
;;;;;2943;Beräknad upplupen särskild löneskatt på pensionskostnader, deklarationspost;;;
;;;;;2944;Beräknad upplupen avkastningsskatt på pensionskostnader;;;
;■;295;Upplupna avtalade sociala avgifter;■;2950;Upplupna avtalade sociala avgifter;;;
;;;;;2951;Upplupna avtalade arbetsmarknadsförsäkringar;;;
;;;;;2959;Upplupna avtalade pensionsförsäkringsavgifter, deklarationspost;;;
;■;296;Upplupna räntekostnader;■;2960;Upplupna räntekostnader;;;
;■;297;Förutbetalda intäkter;■;2970;Förutbetalda intäkter;;;
;;;;;2971;Förutbetalda hyresintäkter;;;
;;;;;2972;Förutbetalda medlemsavgifter;;;
;;;;;2979;Övriga förutbetalda intäkter;;;
;;298;Upplupna avtalskostnader;;2980;Upplupna avtalskostnader;;;)"
R"(; ■; 299; Övriga upplupna kostnader och förutbetalda intäkter; ■; 2990; Övriga upplupna kostnader och förutbetalda intäkter;;;
;;;;;2991;Beräknat arvode för bokslut;;;
;;;;;2992;Beräknat arvode för revision;;;
;;;;;2993;Ospecificerad skuld till leverantörer;;;
;;;;;2998;Övriga upplupna kostnader och förutbetalda intäkter;;;
;;;;■;2999;OBS-konto;;;
;;3;Rörelsens inkomster/intäkter;;;;;;
;;30;Huvudintäkter;;;;;;
;■;300;Försäljning inom Sverige;■;3000;Försäljning inom Sverige;;;
;;;;■;3001;Försäljning inom Sverige, 25 % moms;;;
;;;;■;3002;Försäljning inom Sverige, 12 % moms;;;
;;;;■;3003;Försäljning inom Sverige, 6 % moms;;;
;;;;■;3004;Försäljning inom Sverige, momsfri;;;
;;31;Huvudintäkter;;;;;;
;■;310;Försäljning av varor utanför Sverige;■;3100;Försäljning av varor utanför Sverige;;;
;;;;■;3105;Försäljning varor till land utanför EU;;;
;;;;■;3106;Försäljning varor till annat EU-land, momspliktig;;;
;;;;■;3108;Försäljning varor till annat EU-land, momsfri;;;
;;32;Huvudintäkter;;;;;;
;■;320;Försäljning VMB och omvänd moms;■;3200;Försäljning VMB och omvänd moms;;;
;■;321;Försäljning positiv VMB 25 %;■;3211;Försäljning positiv VMB 25 %;;;
;;;;■;3212;Försäljning negativ VMB 25 %;;;
;■;323;Försäljning inom byggsektorn, omvänd skattskyldighet moms;■;3231;Försäljning inom byggsektorn, omvänd skattskyldighet moms;;;
;;33;Huvudintäkter;;;;;;
;■;330;Försäljning av tjänster utanför Sverige;■;3300;Försäljning av tjänster utanför Sverige;;;
;;;;■;3305;Försäljning tjänster till land utanför EU;;;
;;;;■;3308;Försäljning tjänster till annat EU-land;;;
;;34;Huvudintäkter;;;;;;
;■;340;Försäljning, egna uttag;■;3400;Försäljning, egna uttag;;;
;;;;■;3401;Egna uttag momspliktiga, 25 %;;;
;;;;■;3402;Egna uttag momspliktiga, 12 %;;;
;;;;■;3403;Egna uttag momspliktiga, 6 %;;;
;;;;■;3404;Egna uttag, momsfria;;;
;;35;Fakturerade kostnader;;;;;;
;■;350;Fakturerade kostnader (gruppkonto);■;3500;Fakturerade kostnader (gruppkonto);;;
;■;351;Fakturerat emballage;■;3510;Fakturerat emballage;;;
;;;;;3511;Fakturerat emballage;;;
;;;;;3518;Returnerat emballage;;;
;■;352;Fakturerade frakter;■;3520;Fakturerade frakter;;;
;;;;■;3521;Fakturerade frakter, EU-land;;;
;;;;■;3522;Fakturerade frakter, export;;;
;■;353;Fakturerade tull- och speditionskostnader m.m.;■;3530;Fakturerade tull- och speditionskostnader m.m.;;;
;■;354;Faktureringsavgifter;■;3540;Faktureringsavgifter;;;
;;;;■;3541;Faktureringsavgifter, EU-land;;;
;;;;■;3542;Faktureringsavgifter, export;;;
;;355;Fakturerade resekostnader;;3550;Fakturerade resekostnader;;;
;;356;Fakturerade kostnader till koncernföretag;;3560;Fakturerade kostnader till koncernföretag;;;
;;;;;3561;Fakturerade kostnader till moderföretag;;;
;;;;;3562;Fakturerade kostnader till dotterföretag;;;
;;;;;3563;Fakturerade kostnader till andra koncernföretag;;;
;;357;Fakturerade kostnader till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;3570;Fakturerade kostnader till intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;359;Övriga fakturerade kostnader;;3590;Övriga fakturerade kostnader;;;
;;36;Rörelsens sidointäkter;;;;;;
;■;360;Rörelsens sidointäkter (gruppkonto);■;3600;Rörelsens sidointäkter (gruppkonto);;;
;;361;Försäljning av material;;3610;Försäljning av material;;;
;;;;;3611;Försäljning av råmaterial;;;
;;;;;3612;Försäljning av skrot;;;
;;;;;3613;Försäljning av förbrukningsmaterial;;;
;;;;;3619;Försäljning av övrigt material;;;
;;362;Tillfällig uthyrning av personal;;3620;Tillfällig uthyrning av personal;;;
;;363;Tillfällig uthyrning av transportmedel;;3630;Tillfällig uthyrning av transportmedel;;;
;;367;Intäkter från värdepapper;;3670;Intäkter från värdepapper;;;
;;;;;3671;Försäljning av värdepapper;;;
;;;;;3672;Utdelning från värdepapper;;;
;;;;;3679;Övriga intäkter från värdepapper;;;
;;368;Management fees;;3680;Management fees;;;
;;369;Övriga sidointäkter;;3690;Övriga sidointäkter;;;
;;37;Intäktskorrigeringar;;;;;;
;;370;Intäktskorrigeringar (gruppkonto);;3700;Intäktskorrigeringar (gruppkonto);;;
;;371;Ofördelade intäktsreduktioner;;3710;Ofördelade intäktsreduktioner;;;
;■;373;Lämnade rabatter;■;3730;Lämnade rabatter;;;
;;;;;3731;Lämnade kassarabatter;;;
;;;;;3732;Lämnade mängdrabatter;;;
;■;374;Öres- och kronutjämning;■;3740;Öres- och kronutjämning;;;
;;375;Punktskatter;;3750;Punktskatter;;;
;;;;;3751;Intäktsförda punktskatter (kreditkonto);;;
;;;;;3752;Skuldförda punktskatter (debetkonto);;;
;;379;Övriga intäktskorrigeringar;;3790;Övriga intäktskorrigeringar;;;
;;38;Aktiverat arbete för egen räkning;;;;;;
;■;380;Aktiverat arbete för egen räkning (gruppkonto);■;3800;Aktiverat arbete för egen räkning (gruppkonto);;;
;;384;Aktiverat arbete (material);;3840;Aktiverat arbete (material);;;
;;385;Aktiverat arbete (omkostnader);;3850;Aktiverat arbete (omkostnader);;;
;;387;Aktiverat arbete (personal);;3870;Aktiverat arbete (personal);;;
;;39;Övriga rörelseintäkter;;;;;;
;■;390;Övriga rörelseintäkter (gruppkonto);■;3900;Övriga rörelseintäkter (gruppkonto);;;
;;391;Hyres- och arrendeintäkter;;3910;Hyres- och arrendeintäkter;;;
;;;;;3911;Hyresintäkter;;;
;;;;;3912;Arrendeintäkter;;;
;;;;■;3913;Frivilligt momspliktiga hyresintäkter;;;
;;;;;3914;Övriga momspliktiga hyresintäkter;;;
;;392;Provisionsintäkter, licensintäkter och royalties;;3920;Provisionsintäkter, licensintäkter och royalties;;;
;;;;;3921;Provisionsintäkter;;;
;;;;;3922;Licensintäkter och royalties;;;
;;;;;3925;Franchiseintäkter;;;
;[Ej K2];394;Orealiserade negativa/positiva värdeförändringar på säkringsinstrument;[Ej K2];3940;Orealiserade negativa/positiva värdeförändringar på säkringsinstrument;;;
;;395;Återvunna, tidigare avskrivna kundfordringar;;3950;Återvunna, tidigare avskrivna kundfordringar;;;
;■;396;Valutakursvinster på fordringar och skulder av rörelsekaraktär;■;3960;Valutakursvinster på fordringar och skulder av rörelsekaraktär;;;
;■;397;Vinst vid avyttring av immateriella och materiella anläggningstillgångar;■;3970;Vinst vid avyttring av immateriella och materiella anläggningstillgångar;;;
;;;;;3971;Vinst vid avyttring av immateriella anläggningstillgångar;;;
;;;;;3972;Vinst vid avyttring av byggnader och mark;;;
;;;;;3973;Vinst vid avyttring av maskiner och inventarier;;;
;■;398;Erhållna offentliga bidrag;■;3980;Erhållna offentliga bidrag;|;;
;;;;;3981;Erhållna EU-bidrag;;;
;;;;;3985;Erhållna statliga bidrag;|;;
;;;;;3987;Erhållna kommunala bidrag;;;
;;;;;3988;Erhållna offentliga bidrag för personal;|;;
;;;;;3989;Övriga erhållna offentliga bidrag;|;;
;;399;Övriga ersättningar, bidrag och intäkter;;3990;Övriga ersättningar, bidrag och intäkter;|;;
;;;;;3991;Konfliktersättning;;;
;;;;;3992;Erhållna skadestånd;;;
;;;;;3993;Erhållna donationer och gåvor;;;
;;;;;3994;Försäkringsersättningar;;;
;;;;;3995;Erhållet ackord på skulder av rörelsekaraktär;;;
;;;;;3996;Erhållna reklambidrag;;;
;;;;;3997;Sjuklöneersättning;|;;
;;;;;3998;Återbäring av överskott från försäkringsföretag;|;;
;;;;;3999;Övriga rörelseintäkter;;;
;;4;Utgifter/kostnader för varor, material och vissa köpta tjänster;;;;;;
;;40;Inköp av varor och material;;;;;;
;■;400;Inköp av varor från Sverige;■;4000;Inköp av varor från Sverige;;;
;;41;Inköp av varor och material;;;;;;
;;42;Inköp av varor och material;;;;;;
;■;420;Sålda varor VMB;■;4200;Sålda varor VMB;;;
;■;421;Sålda varor positiv VMB 25 %;■;4211;Sålda varor positiv VMB 25 %;;;
;;;;■;4212;Sålda varor negativ VMB 25 %;;;
;;43;Inköp av varor och material;;;;;;
;;44;Inköp av varor och material;;;;;;
;■;440;Momspliktiga inköp i Sverige;■;4400;Momspliktiga inköp i Sverige;;;
;■;441;Inköpta varor i Sverige, omvänd skattskyldighet, 25 % moms;■;4415;Inköpta varor i Sverige, omvänd skattskyldighet, 25 % moms;;;
;;;;;4416;Inköpta varor i Sverige, omvänd skattskyldighet, 12 % moms;;;
;;;;;4417;Inköpta varor i Sverige, omvänd skattskyldighet, 6 % moms;;;
;;442;Inköpta tjänster i Sverige, omvänd skattskyldighet, 25 % moms;;4425;Inköpta tjänster i Sverige, omvänd skattskyldighet, 25 % moms;;;
;;;;■;4426;Inköpta tjänster i Sverige, omvänd skattskyldighet, 12 %;;;
;;;;■;4427;Inköpta tjänster i Sverige, omvänd skattskyldighet, 6 %;;;
;;45;Inköp av varor och material;;;;;;
;■;450;Övriga momspliktiga inköp;■;4500;Övriga momspliktiga inköp;;;
;■;451;Inköp av varor från annat EU-land, 25 %;■;4515;Inköp av varor från annat EU-land, 25 %;;;
;;;;■;4516;Inköp av varor från annat EU-land, 12 %;;;
;;;;■;4517;Inköp av varor från annat EU-land, 6 %;;;
;;;;■;4518;Inköp av varor från annat EU-land, momsfri;;;
;■;453;Inköp av tjänster från ett land utanför EU, 25 % moms;■;4531;Inköp av tjänster från ett land utanför EU, 25 % moms;;;
;;;;■;4532;Inköp av tjänster från ett land utanför EU, 12 % moms;;;
;;;;■;4533;Inköp av tjänster från ett land utanför EU, 6 % moms;;;
;;;;■;4535;Inköp av tjänster från annat EU-land, 25 %;;;
;;;;■;4536;Inköp av tjänster från annat EU-land, 12 %;;;
;;;;■;4537;Inköp av tjänster från annat EU-land, 6 %;;;
;;;;■;4538;Inköp av tjänster från annat EU-land, momsfri;;;
;■;454;Import av varor, 25 % moms;■;4545;Import av varor, 25 % moms;;;
;;;;■;4546;Import av varor, 12 % moms;;;
;;;;■;4547;Import av varor, 6 % moms;;;
;;46;Legoarbeten, underentreprenader;;;;;;
;■;460;Legoarbeten och underentreprenader (gruppkonto);■;4600;Legoarbeten och underentreprenader (gruppkonto);;;
;;47;Reduktion av inköpspriser;;;;;;
;■;470;Reduktion av inköpspriser (gruppkonto);■;4700;Reduktion av inköpspriser (gruppkonto);;;
;;473;Erhållna rabatter;;4730;Erhållna rabatter;;;
;;;;;4731;Erhållna kassarabatter;;;
;;;;;4732;Erhållna mängdrabatter (inkl. bonus);;;
;;;;;4733;Erhållet aktivitetsstöd;;;
;;479;Övriga reduktioner av inköpspriser;;4790;Övriga reduktioner av inköpspriser;;;
;;48;(Fri kontogrupp);;;;;;
;;49;Förändring av lager, produkter i arbete och pågående arbeten;;;;;;
;■;490;Förändring av lager (gruppkonto);■;4900;Förändring av lager (gruppkonto);;;
;■;491;Förändring av lager av råvaror;■;4910;Förändring av lager av råvaror;;;
;■;492;Förändring av lager av tillsatsmaterial och förnödenheter;■;4920;Förändring av lager av tillsatsmaterial och förnödenheter;;;
;■;494;Förändring av produkter i arbete;■;4940;Förändring av produkter i arbete;;;
;;;;;4944;Förändring av produkter i arbete, material och utlägg;;;
;;;;;4945;Förändring av produkter i arbete, omkostnader;;;
;;;;;4947;Förändring av produkter i arbete, personalkostnader;;;
;■;495;Förändring av lager av färdiga varor;■;4950;Förändring av lager av färdiga varor;;;
;■;496;Förändring av lager av handelsvaror;■;4960;Förändring av lager av handelsvaror;;;
;■;497;Förändring av pågående arbeten, nedlagda kostnader;■;4970;Förändring av pågående arbeten, nedlagda kostnader;;;
;;;;;4974;Förändring av pågående arbeten, material och utlägg;;;
;;;;;4975;Förändring av pågående arbeten, omkostnader;;;
;;;;;4977;Förändring av pågående arbeten, personalkostnader;;;
;;498;Förändring av lager av värdepapper;;4980;Förändring av lager av värdepapper;;;
;;;;;4981;Sålda värdepappers anskaffningsvärde;;;
;;;;;4987;Nedskrivning av värdepapper;;;
;;;;;4988;Återföring av nedskrivning av värdepapper;;;
;;5;Övriga externa rörelseutgifter/ kostnader;;;;;;
;;50;Lokalkostnader;;;;;;
;;500;Lokalkostnader (gruppkonto);;5000;Lokalkostnader (gruppkonto);;;
;■;501;Lokalhyra;■;5010;Lokalhyra;;;
;;;;;5011;Hyra för kontorslokaler;;;
;;;;;5012;Hyra för garage;;;
;;;;;5013;Hyra för lagerlokaler;;;
;■;502;El för belysning;■;5020;El för belysning;;;
;■;503;Värme;■;5030;Värme;;;
;■;504;Vatten och avlopp;■;5040;Vatten och avlopp;;;
;;505;Lokaltillbehör;;5050;Lokaltillbehör;;;
;■;506;Städning och renhållning;■;5060;Städning och renhållning;;;
;;;;;5061;Städning;;;
;;;;;5062;Sophämtning;;;
;;;;;5063;Hyra för sopcontainer;;;
;;;;;5064;Snöröjning;;;
;;;;;5065;Trädgårdsskötsel;;;
;■;507;Reparation och underhåll av lokaler;■;5070;Reparation och underhåll av lokaler;;;
;;509;Övriga lokalkostnader;;5090;Övriga lokalkostnader;;;
;;;;;5098;Övriga lokalkostnader, avdragsgilla;;;
;;;;;5099;Övriga lokalkostnader, ej avdragsgilla;;;
;;51;Fastighetskostnader;;;;;;
;;510;Fastighetskostnader (gruppkonto);;5100;Fastighetskostnader (gruppkonto);;;
;;511;Tomträttsavgäld/arrende;;5110;Tomträttsavgäld/arrende;;;
;■;512;El för belysning;■;5120;El för belysning;;;
;■;513;Värme;■;5130;Värme;;;
;;;;;5131;Uppvärmning;;;
;;;;;5132;Sotning;;;
;■;514;Vatten och avlopp;■;5140;Vatten och avlopp;;;
;■;516;Städning och renhållning;■;5160;Städning och renhållning;;;
;;;;;5161;Städning;;;
;;;;;5162;Sophämtning;;;
;;;;;5163;Hyra för sopcontainer;;;
;;;;;5164;Snöröjning;;;
;;;;;5165;Trädgårdsskötsel;;;
;■;517;Reparation och underhåll av fastighet;■;5170;Reparation och underhåll av fastighet;;;
;;519;Övriga fastighetskostnader;;5190;Övriga fastighetskostnader;;;
;;;;;5191;Fastighetsskatt/fastighetsavgift;;;
;;;;;5192;Fastighetsförsäkringspremier;;;
;;;;;5193;Fastighetsskötsel och förvaltning;;;
;;;;;5198;Övriga fastighetskostnader, avdragsgilla;;;
;;;;;5199;Övriga fastighetskostnader, ej avdragsgilla;;;
;;52;Hyra av anläggningstillgångar;;;;;;
;■;520;Hyra av anläggningstillgångar (gruppkonto);■;5200;Hyra av anläggningstillgångar (gruppkonto);;;
;;521;Hyra av maskiner och andra tekniska anläggningar;;5210;Hyra av maskiner och andra tekniska anläggningar;;;
;;;;;5211;Korttidshyra av maskiner och andra tekniska anläggningar;;;
;;;;;5212;Leasing av maskiner och andra tekniska anläggningar;;;
;;522;Hyra av inventarier och verktyg;;5220;Hyra av inventarier och verktyg;;;
;;;;;5221;Korttidshyra av inventarier och verktyg;;;
;;;;;5222;Leasing av inventarier och verktyg;;;
;;525;Hyra av datorer;;5250;Hyra av datorer;;;
;;;;;5251;Korttidshyra av datorer;;;
;;;;;5252;Leasing av datorer;;;
;;529;Övriga hyreskostnader för anläggningstillgångar;;5290;Övriga hyreskostnader för anläggningstillgångar;;;
;;53;Energikostnader;;;;;;
;■;530;Energikostnader (gruppkonto);■;5300;Energikostnader (gruppkonto);;;
;;531;El för drift;;5310;El för drift;;;
;;532;Gas;;5320;Gas;;;
;;533;Eldningsolja;;5330;Eldningsolja;;;
;;534;Stenkol och koks;;5340;Stenkol och koks;;;
;;535;Torv, träkol, ved och annat träbränsle;;5350;Torv, träkol, ved och annat träbränsle;;;
;;536;Bensin, fotogen och motorbrännolja;;5360;Bensin, fotogen och motorbrännolja;;;
;;537;Fjärrvärme, kyla och ånga;;5370;Fjärrvärme, kyla och ånga;;;
;;538;Vatten;;5380;Vatten;;;
;;539;Övriga energikostnader;;5390;Övriga energikostnader;;;
;;54;Förbrukningsinventarier och förbrukningsmaterial;;;;;;
;;540;Förbrukningsinventarier och förbrukningsmaterial (gruppkonto);;5400;Förbrukningsinventarier och förbrukningsmaterial (gruppkonto);;;
;■;541;Förbrukningsinventarier;■;5410;Förbrukningsinventarier;;;
;;;;;5411;Förbrukningsinventarier med en livslängd på mer än ett år;;;
;;;;;5412;Förbrukningsinventarier med en livslängd på ett år eller mindre;;;
;■;542;Programvaror;■;5420;Programvaror;;;
;;543;Transportinventarier;;5430;Transportinventarier;;;
;;544;Förbrukningsemballage;;5440;Förbrukningsemballage;;;
;■;546;Förbrukningsmaterial;■;5460;Förbrukningsmaterial;;;
;;548;Arbetskläder och skyddsmaterial;;5480;Arbetskläder och skyddsmaterial;;;
;;549;Övriga förbrukningsinventarier och förbrukningsmaterial;;5490;Övriga förbrukningsinventarier och förbrukningsmaterial;;;
;;;;;5491;Övriga förbrukningsinventarier med en livslängd på mer än ett år;;;
;;;;;5492;Övriga förbrukningsinventarier med en livslängd på ett år eller mindre;;;
;;;;;5493;Övrigt förbrukningsmaterial;;;
;;55;Reparation och underhåll;;;;;;)"
R"(; ■; 550; Reparation och underhåll(gruppkonto); ■; 5500; Reparation och underhåll(gruppkonto);;;
;;551;Reparation och underhåll av maskiner och andra tekniska anläggningar;;5510;Reparation och underhåll av maskiner och andra tekniska anläggningar;;;
;;552;Reparation och underhåll av inventarier, verktyg och datorer m.m.;;5520;Reparation och underhåll av inventarier, verktyg och datorer m.m.;;;
;;553;Reparation och underhåll av installationer;;5530;Reparation och underhåll av installationer;;;
;;555;Reparation och underhåll av förbrukningsinventarier;;5550;Reparation och underhåll av förbrukningsinventarier;;;
;;558;Underhåll och tvätt av arbetskläder;;5580;Underhåll och tvätt av arbetskläder;;;
;;559;Övriga kostnader för reparation och underhåll;;5590;Övriga kostnader för reparation och underhåll;;;
;;56;Kostnader för transportmedel;;;;;;
;■;560;Kostnader för transportmedel (gruppkonto);■;5600;Kostnader för transportmedel (gruppkonto);;;
;;561;Personbilskostnader;;5610;Personbilskostnader;;;
;;;;■;5611;Drivmedel för personbilar;;;
;;;;■;5612;Försäkring och skatt för personbilar;;;
;;;;■;5613;Reparation och underhåll av personbilar;;;
;;;;■;5615;Leasing av personbilar;;;
;;;;;5616;Trängselskatt, avdragsgill;;;
;;;;;5619;Övriga personbilskostnader;;;
;;562;Lastbilskostnader;;5620;Lastbilskostnader;;;
;;563;Truckkostnader;;5630;Truckkostnader;;;
;;564;Kostnader för arbetsmaskiner;;5640;Kostnader för arbetsmaskiner;;;
;;565;Traktorkostnader;;5650;Traktorkostnader;;;
;;566;Motorcykel-, moped- och skoterkostnader;;5660;Motorcykel-, moped- och skoterkostnader;;;
;;567;Båt-, flygplans- och helikopterkostnader;;5670;Båt-, flygplans- och helikopterkostnader;;;
;;569;Övriga kostnader för transportmedel;;5690;Övriga kostnader för transportmedel;;;
;;57;Frakter och transporter;;;;;;
;■;570;Frakter och transporter (gruppkonto);■;5700;Frakter och transporter (gruppkonto);;;
;;571;Frakter, transporter och försäkringar vid varudistribution;;5710;Frakter, transporter och försäkringar vid varudistribution;;;
;;572;Tull- och speditionskostnader m.m.;;5720;Tull- och speditionskostnader m.m.;;;
;;573;Arbetstransporter;;5730;Arbetstransporter;;;
;;579;Övriga kostnader för frakter och transporter;;5790;Övriga kostnader för frakter och transporter;;;
;;58;Resekostnader;;;;;;
;■;580;Resekostnader (gruppkonto);■;5800;Resekostnader (gruppkonto);;;
;■;581;Biljetter;■;5810;Biljetter;;;
;■;582;Hyrbilskostnader;■;5820;Hyrbilskostnader;;;
;;583;Kost och logi;;5830;Kost och logi;;;
;;;;■;5831;Kost och logi i Sverige;;;
;;;;■;5832;Kost och logi i utlandet;;;
;;589;Övriga resekostnader;;5890;Övriga resekostnader;;;
;;59;Reklam och PR;;;;;;
;■;590;Reklam och PR (gruppkonto);■;5900;Reklam och PR (gruppkonto);;;
;;591;Annonsering;;5910;Annonsering;;;
;;592;Utomhus- och trafikreklam;;5920;Utomhus- och trafikreklam;;;
;;593;Reklamtrycksaker och direktreklam;;5930;Reklamtrycksaker och direktreklam;;;
;;594;Utställningar och mässor;;5940;Utställningar och mässor;;;
;;595;Butiksreklam och återförsäljarreklam;;5950;Butiksreklam och återförsäljarreklam;;;
;;596;Varuprover, reklamgåvor, presentreklam och tävlingar;;5960;Varuprover, reklamgåvor, presentreklam och tävlingar;;;
;;597;Film-, radio-, TV- och Internetreklam;;5970;Film-, radio-, TV- och Internetreklam;;;
;;598;PR, institutionell reklam och sponsring;;5980;PR, institutionell reklam och sponsring;;;
;;599;Övriga kostnader för reklam och PR;;5990;Övriga kostnader för reklam och PR;;;
;;6;Övriga externa rörelseutgifter/ kostnader;;;;;;
;;60;Övriga försäljningskostnader;;;;;;
;;600;Övriga försäljningskostnader (gruppkonto);;6000;Övriga försäljningskostnader (gruppkonto);;;
;;601;Kataloger, prislistor m.m.;;6010;Kataloger, prislistor m.m.;;;
;;602;Egna facktidskrifter;;6020;Egna facktidskrifter;;;
;;603;Speciella orderkostnader;;6030;Speciella orderkostnader;;;
;;604;Kontokortsavgifter;;6040;Kontokortsavgifter;;;
;;605;Försäljningsprovisioner;;6050;Försäljningsprovisioner;;;
;;;;;6055;Franchisekostnader o.dyl.;;;
;;606;Kreditförsäljningskostnader;;6060;Kreditförsäljningskostnader;;;
;;;;;6061;Kreditupplysning;;;
;;;;;6062;Inkasso och KFM-avgifter;;;
;;;;;6063;Kreditförsäkringspremier;;;
;;;;;6064;Factoringavgifter;;;
;;;;;6069;Övriga kreditförsäljningskostnader;;;
;;607;Representation;;6070;Representation;;;
;;;;■;6071;Representation, avdragsgill;;;
;;;;■;6072;Representation, ej avdragsgill;;;
;;608;Bankgarantier;;6080;Bankgarantier;;;
;■;609;Övriga försäljningskostnader;■;6090;Övriga försäljningskostnader;;;
;;61;Kontorsmateriel och trycksaker;;;;;;
;■;610;Kontorsmateriel och trycksaker (gruppkonto);■;6100;Kontorsmateriel och trycksaker (gruppkonto);;;
;;611;Kontorsmateriel;;6110;Kontorsmateriel;;;
;;615;Trycksaker;;6150;Trycksaker;;;
;;62;Tele och post;;;;;;
;;620;Tele och post (gruppkonto);;6200;Tele och post (gruppkonto);;;
;■;621;Telekommunikation;■;6210;Telekommunikation;;;
;;;;;6211;Fast telefoni;;;
;;;;;6212;Mobiltelefon;;;
;;;;;6213;Mobilsökning;;;
;;;;;6214;Fax;;;
;;;;;6215;Telex;;;
;;623;Datakommunikation;;6230;Datakommunikation;;;
;■;625;Postbefordran;■;6250;Postbefordran;;;
;;63;Företagsförsäkringar och övriga riskkostnader;;;;;;
;;630;Företagsförsäkringar och övriga riskkostnader (gruppkonto);;6300;Företagsförsäkringar och övriga riskkostnader (gruppkonto);;;
;■;631;Företagsförsäkringar;■;6310;Företagsförsäkringar;;;
;;632;Självrisker vid skada;;6320;Självrisker vid skada;;;
;;633;Förluster i pågående arbeten;;6330;Förluster i pågående arbeten;;;
;;634;Lämnade skadestånd;;6340;Lämnade skadestånd;;;
;;;;;6341;Lämnade skadestånd, avdragsgilla;;;
;;;;;6342;Lämnade skadestånd, ej avdragsgilla;;;
;■;635;Förluster på kundfordringar;■;6350;Förluster på kundfordringar;;;
;;;;;6351;Konstaterade förluster på kundfordringar;;;
;;;;;6352;Befarade förluster på kundfordringar;;;
;;636;Garantikostnader;;6360;Garantikostnader;;;
;;;;;6361;Förändring av garantiavsättning;;;
;;;;;6362;Faktiska garantikostnader;;;
;;637;Kostnader för bevakning och larm;;6370;Kostnader för bevakning och larm;;;
;;638;Förluster på övriga kortfristiga fordringar;;6380;Förluster på övriga kortfristiga fordringar;;;
;■;639;Övriga riskkostnader;■;6390;Övriga riskkostnader;;;
;;64;Förvaltningskostnader;;;;;;
;;640;Förvaltningskostnader (gruppkonto);;6400;Förvaltningskostnader (gruppkonto);;;
;■;641;Styrelsearvoden som inte är lön;■;6410;Styrelsearvoden som inte är lön;;;
;■;642;Ersättningar till revisor;■;6420;Ersättningar till revisor;;;
;;;;;6421;Revision;;;
;;;;;6422;Revisonsverksamhet utöver revision;;;
;;;;;6423;Skatterådgivning – revisor;;;
;;;;;6424;Övriga tjänster – revisor;;;
;;643;Management fees;;6430;Management fees;;;
;;644;Årsredovisning och delårsrapporter;;6440;Årsredovisning och delårsrapporter;;;
;;645;Bolagsstämma/års- eller föreningsstämma;;6450;Bolagsstämma/års- eller föreningsstämma;;;
;;649;Övriga förvaltningskostnader;;6490;Övriga förvaltningskostnader;;;
;;65;Övriga externa tjänster;;;;;;
;;650;Övriga externa tjänster (gruppkonto);;6500;Övriga externa tjänster (gruppkonto);;;
;;651;Mätningskostnader;;6510;Mätningskostnader;;;
;;652;Ritnings- och kopieringskostnader;;6520;Ritnings- och kopieringskostnader;;;
;■;653;Redovisningstjänster;■;6530;Redovisningstjänster;;;
;■;654;IT-tjänster;■;6540;IT-tjänster;;;
;■;655;Konsultarvoden;■;6550;Konsultarvoden;;;
;;;;;6551;Arkitekttjänster;|;;
;;;;;6552;Teknisk provning och analys;|;;
;;;;;6553;Tekniska konsulttjänster;|;;
;;;;;6554;Finansiell- och övrig ekonomisk rådgivning;|;;
;;;;;6555;Skatterådgivning inkl. insolvens- och konkursförvaltning;|;;
;;;;;6556;Köpta tjänster avseende forskning och utveckling;|;;
;;;;;6559;Övrig konsultverksamhet;|;;
;■;656;Serviceavgifter till branschorganisationer;■;6560;Serviceavgifter till branschorganisationer;;;
;■;657;Bankkostnader;■;6570;Bankkostnader;;;
;;658;Advokat- och rättegångskostnader;;6580;Advokat- och rättegångskostnader;;;
;■;659;Övriga externa tjänster;■;6590;Övriga externa tjänster;;;
;;66;(Fri kontogrupp);;;;;;
;;67;(Fri kontogrupp);;;;;;
;;68;Inhyrd personal;;;;;;
;■;680;Inhyrd personal (gruppkonto);■;6800;Inhyrd personal (gruppkonto);;;
;;681;Inhyrd produktionspersonal;;6810;Inhyrd produktionspersonal;;;
;;682;Inhyrd lagerpersonal;;6820;Inhyrd lagerpersonal;;;
;;683;Inhyrd transportpersonal;;6830;Inhyrd transportpersonal;;;
;;684;Inhyrd kontors- och ekonomipersonal;;6840;Inhyrd kontors- och ekonomipersonal;;;
;;685;Inhyrd IT-personal;;6850;Inhyrd IT-personal;;;
;;686;Inhyrd marknads- och försäljningspersonal;;6860;Inhyrd marknads- och försäljningspersonal;;;
;;687;Inhyrd restaurang- och butikspersonal;;6870;Inhyrd restaurang- och butikspersonal;;;
;;688;Inhyrda företagsledare;;6880;Inhyrda företagsledare;;;
;;689;Övrig inhyrd personal;;6890;Övrig inhyrd personal;;;
;;69;Övriga externa kostnader;;;;;;
;;690;Övriga externa kostnader (gruppkonto);;6900;Övriga externa kostnader (gruppkonto);;;
;;691;Licensavgifter och royalties;;6910;Licensavgifter och royalties;;;
;;692;Kostnader för egna patent;;6920;Kostnader för egna patent;;;
;;693;Kostnader för varumärken m.m.;;6930;Kostnader för varumärken m.m.;;;
;;694;Kontroll-, provnings- och stämpelavgifter;;6940;Kontroll-, provnings- och stämpelavgifter;;;
;;695;Tillsynsavgifter myndigheter;;6950;Tillsynsavgifter myndigheter;;;
;■;697;Tidningar, tidskrifter och facklitteratur;■;6970;Tidningar, tidskrifter och facklitteratur;;;
;■;698;Föreningsavgifter;■;6980;Föreningsavgifter;;;
;;;;;6981;Föreningsavgifter, avdragsgilla;;;
;;;;;6982;Föreningsavgifter, ej avdragsgilla;;;
;;699;Övriga externa kostnader;;6990;Övriga externa kostnader;;;
;;;;■;6991;Övriga externa kostnader, avdragsgilla;;;
;;;;■;6992;Övriga externa kostnader, ej avdragsgilla;;;
;;;;;6993;Lämnade bidrag och gåvor;;;
;;;;;6996;Betald utländsk inkomstskatt;;;
;;;;;6997;Obetald utländsk inkomstskatt;;;
;;;;;6998;Utländsk moms;;;
;;;;;6999;Ingående moms, blandad verksamhet;;;
;;7;Utgifter/kostnader för personal, avskrivningar m.m.;;;;;;
;;70;Löner till kollektivanställda;;;;;;
;;700;Löner till kollektivanställda (gruppkonto);;7000;Löner till kollektivanställda (gruppkonto);;;
;■;701;Löner till kollektivanställda;■;7010;Löner till kollektivanställda;;;
;;;;;7011;Löner till kollektivanställda;;;
;;;;;7012;Vinstandelar till kollektivanställda;;;
;;;;;7013;Lön växa-stöd kollektivanställda 10,21 %;;;
;;;;;7017;Avgångsvederlag till kollektivanställda;;;
;;;;;7018;Bruttolöneavdrag, kollektivanställda;;;
;;;;;7019;Upplupna löner och vinstandelar till kollektivanställda;;;
;;703;Löner till kollektivanställda (utlandsanställda);;7030;Löner till kollektivanställda (utlandsanställda);;;
;;;;;7031;Löner till kollektivanställda (utlandsanställda);;;
;;;;;7032;Vinstandelar till kollektivanställda (utlandsanställda);;;
;;;;;7037;Avgångsvederlag till kollektivanställda (utlandsanställda);;;
;;;;;7038;Bruttolöneavdrag, kollektivanställda (utlandsanställda);;;
;;;;;7039;Upplupna löner och vinstandelar till kollektivanställda (utlandsanställda);;;
;;708;Löner till kollektivanställda för ej arbetad tid;;7080;Löner till kollektivanställda för ej arbetad tid;;;
;;;;;7081;Sjuklöner till kollektivanställda;;;
;;;;;7082;Semesterlöner till kollektivanställda;;;
;;;;;7083;Föräldraersättning till kollektivanställda;;;
;;;;;7089;Övriga löner till kollektivanställda för ej arbetad tid;;;
;■;709;Förändring av semesterlöneskuld;■;7090;Förändring av semesterlöneskuld;;;
;;71;(Fri kontogrupp);;;;;;
;;72;Löner till tjänstemän och företagsledare;;;;;;
;;720;Löner till tjänstemän och företagsledare (gruppkonto);;7200;Löner till tjänstemän och företagsledare (gruppkonto);;;
;■;721;Löner till tjänstemän;■;7210;Löner till tjänstemän;;;
;;;;;7211;Löner till tjänstemän;;;
;;;;;7212;Vinstandelar till tjänstemän;;;
;;;;;7213;Lön växa-stöd tjänstemän 10,21 %;;;
;;;;;7217;Avgångsvederlag till tjänstemän;;;
;;;;;7218;Bruttolöneavdrag, tjänstemän;;;
;;;;;7219;Upplupna löner och vinstandelar till tjänstemän;;;
;■;722;Löner till företagsledare;■;7220;Löner till företagsledare;;;
;;;;;7221;Löner till företagsledare;;;
;;;;;7222;Tantiem till företagsledare;;;
;;;;;7227;Avgångsvederlag till företagsledare;;;
;;;;;7228;Bruttolöneavdrag, företagsledare;;;
;;;;;7229;Upplupna löner och tantiem till företagsledare;;;
;;723;Löner till tjänstemän och ftgsledare (utlandsanställda);;7230;Löner till tjänstemän och ftgsledare (utlandsanställda);;;
;;;;;7231;Löner till tjänstemän och ftgsledare (utlandsanställda);;;
;;;;;7232;Vinstandelar till tjänstemän och ftgsledare (utlandsanställda);;;
;;;;;7237;Avgångsvederlag till tjänstemän och ftgsledare (utlandsanställda);;;
;;;;;7238;Bruttolöneavdrag, tjänstemän och ftgsledare (utlandsanställda);;;
;;;;;7239;Upplupna löner och vinstandelar till tjänstemän och ftgsledare (utlandsanställda);;;
;■;724;Styrelsearvoden;■;7240;Styrelsearvoden;;;
;;728;Löner till tjänstemän och företagsledare för ej arbetad tid;;7280;Löner till tjänstemän och företagsledare för ej arbetad tid;;;
;;;;;7281;Sjuklöner till tjänstemän;;;
;;;;;7282;Sjuklöner till företagsledare;;;
;;;;;7283;Föräldraersättning till tjänstemän;;;
;;;;;7284;Föräldraersättning till företagsledare;;;
;;;;;7285;Semesterlöner till tjänstemän;;;
;;;;;7286;Semesterlöner till företagsledare;;;
;;;;;7288;Övriga löner till tjänstemän för ej arbetad tid;;;
;;;;;7289;Övriga löner till företagsledare för ej arbetad tid;;;
;■;729;Förändring av semesterlöneskuld;■;7290;Förändring av semesterlöneskuld;;;
;;;;;7291;Förändring av semesterlöneskuld till tjänstemän;;;
;;;;;7292;Förändring av semesterlöneskuld till företagsledare;;;
;;73;Kostnadsersättningar och förmåner;;;;;;
;;730;Kostnadsersättningar och förmåner (gruppkonto);;7300;Kostnadsersättningar och förmåner (gruppkonto);;;
;■;731;Kontanta extraersättningar;■;7310;Kontanta extraersättningar;;;
;;;;;7311;Ersättningar för sammanträden m.m.;;;
;;;;;7312;Ersättningar för förslagsverksamhet och uppfinningar;;;
;;;;;7313;Ersättningar för/bidrag till bostadskostnader;;;
;;;;;7314;Ersättningar för/bidrag till måltidskostnader;;;
;;;;;7315;Ersättningar för/bidrag till resor till och från arbetsplatsen;;;
;;;;;7316;Ersättningar för/bidrag till arbetskläder;;;
;;;;;7317;Ersättningar för/bidrag till arbetsmaterial och arbetsverktyg;;;
;;;;;7318;Felräkningspengar;;;
;;;;;7319;Övriga kontanta extraersättningar;;;
;;732;Traktamenten vid tjänsteresa;;7320;Traktamenten vid tjänsteresa;;;
;;;;■;7321;Skattefria traktamenten, Sverige;;;
;;;;■;7322;Skattepliktiga traktamenten, Sverige;;;
;;;;■;7323;Skattefria traktamenten, utlandet;;;
;;;;■;7324;Skattepliktiga traktamenten, utlandet;;;
;;733;Bilersättningar;;7330;Bilersättningar;;;
;;;;■;7331;Skattefria bilersättningar;;;
;;;;■;7332;Skattepliktiga bilersättningar;;;
;;;;;7333;Ersättning för trängselskatt, skattefri;;;
;;735;Ersättningar för föreskrivna arbetskläder;;7350;Ersättningar för föreskrivna arbetskläder;;;
;;737;Representationsersättningar;;7370;Representationsersättningar;;;
;■;738;Kostnader för förmåner till anställda;■;7380;Kostnader för förmåner till anställda;;;
;;;;;7381;Kostnader för fri bostad;;;
;;;;;7382;Kostnader för fria eller subventionerade måltider;;;
;;;;;7383;Kostnader för fria resor till och från arbetsplatsen;;;
;;;;;7384;Kostnader för fria eller subventionerade arbetskläder;;;
;;;;■;7385;Kostnader för fri bil;;;
;;;;;7386;Subventionerad ränta;;;
;;;;;7387;Kostnader för lånedatorer;;;
;;;;;7388;Anställdas ersättning för erhållna förmåner;;;
;;;;;7389;Övriga kostnader för förmåner;;;
;■;739;Övriga kostnadsersättningar och förmåner;■;7390;Övriga kostnadsersättningar och förmåner;;;
;;;;;7391;Kostnad för trängselskatteförmån;;;
;;;;;7392;Kostnad för förmån av hushållsnära tjänster;;;
;;74;Pensionskostnader;;;;;;)"
R"(;; 740; Pensionskostnader(gruppkonto);; 7400; Pensionskostnader(gruppkonto);;;
; ■; 741; Pensionsförsäkringspremier; ■; 7410; Pensionsförsäkringspremier;;;
;;;;;7411;Premier för kollektiva pensionsförsäkringar;;;
;;;;;7412;Premier för individuella pensionsförsäkringar;;;
;;742;Förändring av pensionsskuld;;7420;Förändring av pensionsskuld;;;
;;743;Avdrag för räntedel i pensionskostnad;;7430;Avdrag för räntedel i pensionskostnad;;;
;;744;Förändring av pensionsstiftelsekapital;;7440;Förändring av pensionsstiftelsekapital;;;
;;;;;7441;Överföring av medel till pensionsstiftelse;|;;
;;;;;7448;Gottgörelse från pensionsstiftelse;;;
;;746;Pensionsutbetalningar;;7460;Pensionsutbetalningar;;;
;;;;;7461;Pensionsutbetalningar till f.d. kollektivanställda;;;
;;;;;7462;Pensionsutbetalningar till f.d. tjänstemän;;;
;;;;;7463;Pensionsutbetalningar till f.d. företagsledare;;;
;;747;Förvaltnings- och kreditförsäkringsavgifter;;7470;Förvaltnings- och kreditförsäkringsavgifter;;;
;■;749;Övriga pensionskostnader;■;7490;Övriga pensionskostnader;;;
;;75;Sociala och andra avgifter enligt lag och avtal;;;;;;
;;750;Sociala och andra avgifter enligt lag och avtal (gruppkonto);;7500;Sociala och andra avgifter enligt lag och avtal (gruppkonto);;;
;;751;Arbetsgivaravgifter 31,42 %;;7510;Arbetsgivaravgifter 31,42 %;;;
;;;;■;7511;Arbetsgivaravgifter för löner och ersättningar;;;
;;;;■;7512;Arbetsgivaravgifter för förmånsvärden;;;
;;;;;7515;Arbetsgivaravgifter på skattepliktiga kostnadsersättningar;;;
;;;;;7516;Arbetsgivaravgifter på arvoden;;;
;;;;;7518;Arbetsgivaravgifter på bruttolöneavdrag m.m.;;;
;;;;■;7519;Arbetsgivaravgifter för semester- och löneskulder;;;
;■;753;Särskild löneskatt;■;7530;Särskild löneskatt;;;
;;;;;7531;Särskild löneskatt för vissa försäkringsersättningar m.m.;;;
;;;;;7532;Särskild löneskatt pensionskostnader, deklarationspost;;;
;;;;;7533;Särskild löneskatt för pensionskostnader;;;
;■;755;Avkastningsskatt på pensionsmedel;■;7550;Avkastningsskatt på pensionsmedel;;;
;;;;;7551;Avkastningsskatt 15 % försäkringsföretag m.fl. samt avsatt till pensioner;;;
;;;;;7552;Avkastningsskatt 15 % utländska pensionsförsäkringar;;;
;;;;;7553;Avkastningsskatt 30 % utländska försäkringsföretag m.fl.;;;
;;;;;7554;Avkastningsskatt 30 % utländska kapitalförsäkringar;;;
;■;757;Premier för arbetsmarknadsförsäkringar;■;7570;Premier för arbetsmarknadsförsäkringar;;;
;;;;;7571;Arbetsmarknadsförsäkringar;;;
;;;;;7572;Arbetsmarknadsförsäkringar pensionsförsäkringspremier, deklarationspost;;;
;■;758;Gruppförsäkringspremier;■;7580;Gruppförsäkringspremier;;;
;;;;;7581;Grupplivförsäkringspremier;;;
;;;;;7582;Gruppsjukförsäkringspremier;;;
;;;;;7583;Gruppolycksfallsförsäkringspremier;;;
;;;;;7589;Övriga gruppförsäkringspremier;;;
;■;759;Övriga sociala och andra avgifter enligt lag och avtal;■;7590;Övriga sociala och andra avgifter enligt lag och avtal;;;
;;76;Övriga personalkostnader;;;;;;
;■;760;Övriga personalkostnader (gruppkonto);■;7600;Övriga personalkostnader (gruppkonto);;;
;■;761;Utbildning;■;7610;Utbildning;;;
;;762;Sjuk- och hälsovård;;7620;Sjuk- och hälsovård;;;
;;;;■;7621;Sjuk- och hälsovård, avdragsgill;;;
;;;;■;7622;Sjuk- och hälsovård, ej avdragsgill;;;
;;;;;7623;Sjukvårdsförsäkring, ej avdragsgill;;;
;;763;Personalrepresentation;;7630;Personalrepresentation;;;
;;;;■;7631;Personalrepresentation, avdragsgill;;;
;;;;■;7632;Personalrepresentation, ej avdragsgill;;;
;;765;Sjuklöneförsäkring;;7650;Sjuklöneförsäkring;;;
;;767;Förändring av personalstiftelsekapital;;7670;Förändring av personalstiftelsekapital;;;
;;;;;7671;Avsättning till personalstiftelse;;;
;;;;;7678;Gottgörelse från personalstiftelse;;;
;;769;Övriga personalkostnader;;7690;Övriga personalkostnader;;;
;;;;;7691;Personalrekrytering;;;
;;;;;7692;Begravningshjälp;;;
;;;;;7693;Fritidsverksamhet;;;
;;;;;7699;Övriga personalkostnader;;;
;;77;Nedskrivningar och återföring av nedskrivningar;;;;;;
;;771;Nedskrivningar av immateriella anläggningstillgångar;;7710;Nedskrivningar av immateriella anläggningstillgångar;;;
;■;772;Nedskrivningar av byggnader och mark;■;7720;Nedskrivningar av byggnader och mark;;;
;■;773;Nedskrivningar av maskiner och inventarier;■;7730;Nedskrivningar av maskiner och inventarier;;;
;;774;Nedskrivningar av vissa omsättningstillgångar;;7740;Nedskrivningar av vissa omsättningstillgångar;;;
;;776;Återföring av nedskrivningar av immateriella anläggningstillgångar;;7760;Återföring av nedskrivningar av immateriella anläggningstillgångar;;;
;;777;Återföring av nedskrivningar av byggnader och mark;;7770;Återföring av nedskrivningar av byggnader och mark;;;
;;778;Återföring av nedskrivningar av maskiner och inventarier;;7780;Återföring av nedskrivningar av maskiner och inventarier;;;
;;779;Återföring av nedskrivningar av vissa omsättningstillgångar;;7790;Återföring av nedskrivningar av vissa omsättningstillgångar;;;
;;78;Avskrivningar enligt plan;;;;;;
;■;781;Avskrivningar på immateriella anläggningstillgångar;■;7810;Avskrivningar på immateriella anläggningstillgångar;;;
;;;;;7811;Avskrivningar på balanserade utgifter;;;
;;;;;7812;Avskrivningar på koncessioner m.m.;;;
;;;;;7813;Avskrivningar på patent;;;
;;;;;7814;Avskrivningar på licenser;;;
;;;;;7815;Avskrivningar på varumärken;;;
;;;;;7816;Avskrivningar på hyresrätter;;;
;;;;;7817;Avskrivningar på goodwill;;;
;;;;;7819;Avskrivningar på övriga immateriella anläggningstillgångar;;;
;■;782;Avskrivningar på byggnader och markanläggningar;■;7820;Avskrivningar på byggnader och markanläggningar;;;
;;;;;7821;Avskrivningar på byggnader;;;
;;;;;7824;Avskrivningar på markanläggningar;;;
;;;;;7829;Avskrivningar på övriga byggnader;;;
;■;783;Avskrivningar på maskiner och inventarier;■;7830;Avskrivningar på maskiner och inventarier;;;
;;;;;7831;Avskrivningar på maskiner och andra tekniska anläggningar;;;
;;;;;7832;Avskrivningar på inventarier och verktyg;;;
;;;;;7833;Avskrivningar på installationer;;;
;;;;;7834;Avskrivningar på bilar och andra transportmedel;;;
;;;;;7835;Avskrivningar på datorer;;;
;;;;;7836;Avskrivningar på leasade tillgångar;;;
;;;;;7839;Avskrivningar på övriga maskiner och inventarier;;;
;;784;Avskrivningar på förbättringsutgifter på annans fastighet;;7840;Avskrivningar på förbättringsutgifter på annans fastighet;;;
;;79;Övriga rörelsekostnader;;;;;;
;[Ej K2];794;Orealiserade positiva/negativa värdeförändringar på säkringsinstrument;[Ej K2];7940;Orealiserade positiva/negativa värdeförändringar på säkringsinstrument;;;
;;796;Valutakursförluster på fordringar och skulder av rörelsekaraktär;;7960;Valutakursförluster på fordringar och skulder av rörelsekaraktär;;;
;■;797;Förlust vid avyttring av immateriella och materiella anläggningstillgångar;■;7970;Förlust vid avyttring av immateriella och materiella anläggningstillgångar;;;
;;;;;7971;Förlust vid avyttring av immateriella anläggningstillgångar;;;
;;;;;7972;Förlust vid avyttring av byggnader och mark;;;
;;;;;7973;Förlust vid avyttring av maskiner och inventarier;;;
;■;799;Övriga rörelsekostnader;■;7990;Övriga rörelsekostnader;;;
;;8;Finansiella och andra inkomster/ intäkter och utgifter/kostnader;;;;;;
;;80;Resultat från andelar i koncernföretag;;;;;;
;;801;Utdelning på andelar i koncernföretag;;8010;Utdelning på andelar i koncernföretag;;;
;;;;;8012;Utdelning på andelar i dotterföretag;;;
;;;;;8016;Emissionsinsats, koncernföretag;;;
;;802;Resultat vid försäljning av andelar i koncernföretag;;8020;Resultat vid försäljning av andelar i koncernföretag;;;
;;;;;8022;Resultat vid försäljning av andelar i dotterföretag;;;
;;803;Resultatandelar från handelsbolag (dotterföretag);;8030;Resultatandelar från handelsbolag (dotterföretag);;;
;;807;Nedskrivningar av andelar i och långfristiga fordringar hos koncernföretag;;8070;Nedskrivningar av andelar i och långfristiga fordringar hos koncernföretag;;;
;;;;;8072;Nedskrivningar av andelar i dotterföretag;;;
;;;;;8076;Nedskrivningar av långfristiga fordringar hos moderföretag;;;
;;;;;8077;Nedskrivningar av långfristiga fordringar hos dotterföretag;;;
;;808;Återföringar av nedskrivningar av andelar i och långfristiga fordringar hos koncernföretag;;8080;Återföringar av nedskrivningar av andelar i och långfristiga fordringar hos koncernföretag;;;
;;;;;8082;Återföringar av nedskrivningar av andelar i dotterföretag;;;
;;;;;8086;Återföringar av nedskrivningar av långfristiga fordringar hos moderföretag;;;
;;;;;8087;Återföringar av nedskrivningar av långfristiga fordringar hos dotterföretag;;;
;;81;Resultat från andelar i intresseföretag;;;;;;
;;811;Utdelningar på andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;8110;Utdelningar på andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;8111;Utdelningar på andelar i intresseföretag;;;
;;;;;8112;Utdelningar på andelar i gemensamt styrda företag;;;
;;;;;8113;Utdelningar på andelar i övriga företag som det finns ett ägarintresse i;;;
;;;;;8116;Emissionsinsats, intresseföretag;;;
;;;;;8117;Emissionsinsats, gemensamt styrda företag;;;
;;;;;8118;Emissionsinsats, övriga företag som det finns ett ägarintresse i;;;
;;812;Resultat vid försäljning av andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;8120;Resultat vid försäljning av andelar i intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;8121;Resultat vid försäljning av andelar i intresseföretag;;;
;;;;;8122;Resultat vid försäljning av andelar i gemensamt styrda företag;;;
;;;;;8123;Resultat vid försäljning av andelar i övriga företag som det finns ett ägarintresse i;;;
;;813;Resultatandelar från handelsbolag (intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i);;8130;Resultatandelar från handelsbolag (intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i);;;
;;;;;8131;Resultatandelar från handelsbolag (intresseföretag);;;
;;;;;8132;Resultatandelar från handelsbolag (gemensamt styrda företag);;;
;;;;;8133;Resultatandelar från handelsbolag (övriga företag som det finns ett ägarintresse i);;;
;;817;Nedskrivningar av andelar i och långfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;8170;Nedskrivningar av andelar i och långfristiga fordringar hos intresseföretag, gemensamt styrda företag och övriga företag som det finns ett ägarintresse i;;;
;;;;;8171;Nedskrivningar av andelar i intresseföretag;;;
;;;;;8172;Nedskrivningar av långfristiga fordringar hos intresseföretag;;;
;;;;;8173;Nedskrivningar av andelar i gemensamt styrda företag;;;
;;;;;8174;Nedskrivningar av långfristiga fordringar hos gemensamt styrda företag;;;
;;;;;8176;Nedskrivningar av andelar i övriga företag som det finns ett ägarintresse i;;;
;;;;;8177;Nedskrivningar av långfristiga fordringar hos övriga företag som det finns ett ägarintresse i;;;
;;818;Återföringar av nedskrivningar av andelar i och långfristiga fordringar hos intresseföretag;;8180;Återföringar av nedskrivningar av andelar i och långfristiga fordringar hos intresseföretag;;;
;;;;;8181;Återföringar av nedskrivningar av andelar i intresseföretag;;;
;;;;;8182;Återföringar av nedskrivningar av långfristiga fordringar hos intresseföretag;;;
;;;;;8183;Återföringar av nedskrivningar av andelar i gemensamt styrda företag;;;
;;;;;8184;Återföringar av nedskrivningar av långfristiga fordringar hos gemensamt styrda företag;;;
;;;;;8186;Återföringar av nedskrivningar av andelar i övriga företag som det finns ett ägarintresse i;;;
;;;;;8187;Återföringar av nedskrivningar av långfristiga fordringar hos övriga företag som det finns ett ägarintresse i;;;
;;82;Resultat från övriga värdepapper och långfristiga fordringar (anläggningstillgångar);;;;;;
;■;821;Utdelningar på andelar i andra företag;■;8210;Utdelningar på andelar i andra företag;;;
;;;;;8212;Utdelningar, övriga företag;;;
;;;;;8216;Insatsemissioner, övriga företag;;;
;■;822;Resultat vid försäljning av värdepapper i och långfristiga fordringar hos andra företag;■;8220;Resultat vid försäljning av värdepapper i och långfristiga fordringar hos andra företag;;;
;;;;;8221;Resultat vid försäljning av andelar i andra företag;;;
;;;;;8222;Resultat vid försäljning av långfristiga fordringar hos andra företag;;;
;;;;;8223;Resultat vid försäljning av derivat (långfristiga värdepappersinnehav);;;
;;823;Valutakursdifferenser på långfristiga fordringar;;8230;Valutakursdifferenser på långfristiga fordringar;;;
;;;;;8231;Valutakursvinster på långfristiga fordringar;;;
;;;;;8236;Valutakursförluster på långfristiga fordringar;;;
;;824;Resultatandelar från handelsbolag (andra företag);;8240;Resultatandelar från handelsbolag (andra företag);;;
;■;825;Ränteintäkter från långfristiga fordringar hos och värdepapper i andra företag;■;8250;Ränteintäkter från långfristiga fordringar hos och värdepapper i andra företag;;;
;;;;;8251;Ränteintäkter från långfristiga fordringar;;;
;;;;;8252;Ränteintäkter från övriga värdepapper;;;
;;;;;8254;Skattefria ränteintäkter, långfristiga tillgångar;;;
;;;;;8255;Avkastningsskatt kapitalplacering;;;
;;826;Ränteintäkter från långfristiga fordringar hos koncernföretag;;8260;Ränteintäkter från långfristiga fordringar hos koncernföretag;;;
;;;;;8261;Ränteintäkter från långfristiga fordringar hos moderföretag;;;
;;;;;8262;Ränteintäkter från långfristiga fordringar hos dotterföretag;;;
;;;;;8263;Ränteintäkter från långfristiga fordringar hos andra koncernföretag;;;
;■;827;Nedskrivningar av innehav av andelar i och långfristiga fordringar hos andra företag;■;8270;Nedskrivningar av innehav av andelar i och långfristiga fordringar hos andra företag;;;
;;;;;8271;Nedskrivningar av andelar i andra företag;;;
;;;;;8272;Nedskrivningar av långfristiga fordringar hos andra företag;;;
;;;;;8273;Nedskrivningar av övriga värdepapper hos andra företag;;;
;;828;Återföringar av nedskrivningar av andelar i och långfristiga fordringar hos andra företag;;8280;Återföringar av nedskrivningar av andelar i och långfristiga fordringar hos andra företag;;;
;;;;;8281;Återföringar av nedskrivningar av andelar i andra företag;;;
;;;;;8282;Återföringar av nedskrivningar av långfristiga fordringar hos andra företag;;;
;;;;;8283;Återföringar av nedskrivningar av övriga värdepapper i andra företag;;;
;[Ej K2];829;Värdering till verkligt värde, anläggningstillgångar;[Ej K2];8290;Värdering till verkligt värde, anläggningstillgångar;;;
;;;;[Ej K2];8291;Orealiserade värdeförändringar på anläggningstillgångar;;;
;;;;[Ej K2];8295;Orealiserade värdeförändringar på derivatinstrument;;;
;;83;Övriga ränteintäkter och liknande resultatposter;;;;;;
;■;831;Ränteintäkter från omsättningstillgångar;■;8310;Ränteintäkter från omsättningstillgångar;;;
;;;;;8311;Ränteintäkter från bank;;;
;;;;;8312;Ränteintäkter från kortfristiga placeringar;;;
;;;;;8313;Ränteintäkter från kortfristiga fordringar;;;
;;;;■;8314;Skattefria ränteintäkter;;;
;;;;;8317;Ränteintäkter för dold räntekompensation;;;
;;;;;8319;Övriga ränteintäkter från omsättningstillgångar;;;
;[Ej K2];832;Värdering till verkligt värde, omsättningstillgångar;[Ej K2];8320;Värdering till verkligt värde, omsättningstillgångar;;;
;;;;[Ej K2];8321;Orealiserade värdeförändringar på omsättningstillgångar;;;
;;;;[Ej K2];8325;Orealiserade värdeförändringar på derivatinstrument (oms.-tillg.);;;
;■;833;Valutakursdifferenser på kortfristiga fordringar och placeringar;■;8330;Valutakursdifferenser på kortfristiga fordringar och placeringar;;;
;;;;;8331;Valutakursvinster på kortfristiga fordringar och placeringar;;;)"
R"(;;;;; 8336; Valutakursförluster på kortfristiga fordringar och placeringar;;;
;■;834;Utdelningar på kortfristiga placeringar;■;8340;Utdelningar på kortfristiga placeringar;;;
;■;835;Resultat vid försäljning av kortfristiga placeringar;■;8350;Resultat vid försäljning av kortfristiga placeringar;;;
;;836;Övriga ränteintäkter från koncernföretag;;8360;Övriga ränteintäkter från koncernföretag;;;
;;;;;8361;Övriga ränteintäkter från moderföretag;;;
;;;;;8362;Övriga ränteintäkter från dotterföretag;;;
;;;;;8363;Övriga ränteintäkter från andra koncernföretag;;;
;;837;Nedskrivningar av kortfristiga placeringar;;8370;Nedskrivningar av kortfristiga placeringar;;;
;;838;Återföringar av nedskrivningar av kortfristiga placeringar;;8380;Återföringar av nedskrivningar av kortfristiga placeringar;;;
;■;839;Övriga finansiella intäkter;■;8390;Övriga finansiella intäkter;;;
;;84;Räntekostnader och liknande resultatposter;;;;;;
;;840;Räntekostnader (gruppkonto);;8400;Räntekostnader (gruppkonto);;;
;■;841;Räntekostnader för långfristiga skulder;■;8410;Räntekostnader för långfristiga skulder;;;
;;;;;8411;Räntekostnader för obligations-, förlags- och konvertibla lån;;;
;;;;;8412;Räntedel i årets pensionskostnad;;;
;;;;;8413;Räntekostnader för checkräkningskredit;;;
;;;;;8415;Räntekostnader för andra skulder till kreditinstitut;;;
;;;;;8417;Räntekostnader för dold räntekompensation m.m.;;;
;;;;;8418;Avdragspost för räntesubventioner;;;
;;;;;8419;Övriga räntekostnader för långfristiga skulder;;;
;■;842;Räntekostnader för kortfristiga skulder;■;8420;Räntekostnader för kortfristiga skulder;;;
;;;;;8421;Räntekostnader till kreditinstitut;;;
;;;;■;8422;Dröjsmålsräntor för leverantörsskulder;;;
;;;;■;8423;Räntekostnader för skatter och avgifter;;;
;;;;;8424;Räntekostnader byggnadskreditiv;;;
;;;;;8429;Övriga räntekostnader för kortfristiga skulder;;;
;■;843;Valutakursdifferenser på skulder;■;8430;Valutakursdifferenser på skulder;;;
;;;;;8431;Valutakursvinster på skulder;;;
;;;;;8436;Valutakursförluster på skulder;;;
;;844;Erhållna räntebidrag;;8440;Erhållna räntebidrag;;;
;[Ej K2];845;Orealiserade värdeförändringar på skulder;[Ej K2];8450;Orealiserade värdeförändringar på skulder;;;
;;;;[Ej K2];8451;Orealiserade värdeförändringar på skulder;;;
;;;;[Ej K2];8455;Orealiserade värdeförändringar på säkringsinstrument;;;
;;846;Räntekostnader till koncernföretag;;8460;Räntekostnader till koncernföretag;;;
;;;;;8461;Räntekostnader till moderföretag;;;
;;;;;8462;Räntekostnader till dotterföretag;;;
;;;;;8463;Räntekostnader till andra koncernföretag;;;
;[Ej K2];848;Aktiverade ränteutgifter;[Ej K2];8480;Aktiverade ränteutgifter;;;
;;849;Övriga skuldrelaterade poster;;8490;Övriga skuldrelaterade poster;;;
;;;;;8491;Erhållet ackord på skulder till kreditinstitut m.m.;;;
;;85;(Fri kontogrupp);;;;;;
;;86;(Fri kontogrupp);;;;;;
;;87;(Fri kontogrupp);;;;;;
;;88;Bokslutsdispositioner;;;;;;
;;881;Förändring av periodiseringsfond;;8810;Förändring av periodiseringsfond;;;
;;;;■;8811;Avsättning till periodiseringsfond;;;
;;;;■;8819;Återföring från periodiseringsfond;;;
;;882;Mottagna koncernbidrag;;8820;Mottagna koncernbidrag;;;
;;883;Lämnade koncernbidrag;;8830;Lämnade koncernbidrag;;;
;;884;Lämnade gottgörelser;;8840;Lämnade gottgörelser;;;
;■;885;Förändring av överavskrivningar;■;8850;Förändring av överavskrivningar;;;
;;;;;8851;Förändring av överavskrivningar, immateriella anläggningstillgångar;;;
;;;;;8852;Förändring av överavskrivningar, byggnader och markanläggningar;;;
;;;;;8853;Förändring av överavskrivningar, maskiner och inventarier;;;
;;886;Förändring av ersättningsfond;;8860;Förändring av ersättningsfond;;;
;;;;;8861;Avsättning till ersättningsfond för inventarier;;;
;;;;;8862;Avsättning till ersättningsfond för byggnader och markanläggningar;;;
;;;;;8864;Avsättning till ersättningsfond för djurlager i jordbruk och renskötsel;;;
;;;;;8865;Ianspråktagande av ersättningsfond för avskrivningar;;;
;;;;;8866;Ianspråktagande av ersättningsfond för annat än avskrivningar;;;
;;;;;8869;Återföring från ersättningsfond;;;
;;889;Övriga bokslutsdispositioner;;8890;Övriga bokslutsdispositioner;;;
;;;;;8892;Nedskrivningar av konsolideringskaraktär av anläggningstillgångar;;;
;;;;;8896;Förändring av lagerreserv;;;
;;;;;8899;Övriga bokslutsdispositioner;;;
;;89;Skatter och årets resultat;;;;;;
;■;891;Skatt som belastar årets resultat;■;8910;Skatt som belastar årets resultat;;;
;;892;Skatt på grund av ändrad beskattning;;8920;Skatt på grund av ändrad beskattning;;;
;;893;Restituerad skatt;;8930;Restituerad skatt;;;
;[Ej K2];894;Uppskjuten skatt;[Ej K2];8940;Uppskjuten skatt;;;
;;898;Övriga skatter;;8980;Övriga skatter;;;
;■;899;Resultat;■;8990;Resultat;;;
;;;;■;8999;Årets resultat;;;
	)"};
} // namespace BAS