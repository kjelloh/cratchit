// NOTE: This file contains string literals and thus will be encoded as defined by this source file encoding.
// Current code consumes this file assuming UTF-8 encoding (e.g. use sie_from_utf8_sv)
// BUT: SIE actual files are defined to be encoded using the CP437 character set (see irl file reading sie_from_cp437_stream)

inline char const* sz_minimal_sie_text = 
R"(#GEN 20251026
#RAR 0 20250501 20260430)";

inline char const* sz_sie_three_transactions_text = R"(
#FLAGGA 0
#FORMAT PC8
#SIETYP 4
#PROGRAM "Fortnox" 3.57.11
#GEN 20250829
#FNR 503072
#FNAMN "The ITfied AB"
#ADRESS "Adam Smith" "Gatan 7" "123 45 Bullerbyn" "123-567789" 
#RAR 0 20240501 20250430
#RAR -1 20230501 20240430
#ORGNR 112233-4455
#OMFATTN 20250430
#KPTYP EUBAS97

#VER A 1 20240512 "Korrigerad M4 för korrektur till nästa M1" 20240706
{
#TRANS 1650 {} 180 "" "Korrigera upp till M4 rapporterat belopp" 0
#TRANS 2641 {} -180 "" "Korrigerad M4 för korrektur till nästa M1" 0
}
#VER A 2 20240504 "Account:SKV Text:Intäktsränta" 20240706
{
#TRANS 1630 {} 1 "" "" 0
#TRANS 8314 {} -1 "" "" 0
}
#VER A 3 20240506 "Account:NORDEA Text:PRIS ENL SPEC" 20240706
{
#TRANS 1920 {} -3.7 "" "" 0
#TRANS 6570 {} 3.7 "" "" 0
}

)";

inline char const* sz_sie_base_1 = R"(
#FLAGGA 0
#FORMAT PC8
#SIETYP 4
#PROGRAM "Fortnox" 3.57.11
#GEN 20250829
#FNR 503072
#FNAMN "The ITfied AB"
#ADRESS "Adam Smith" "Gatan 7" "123 45 Bullerbyn" "123-567789" 
#RAR 0 20240501 20250430
#RAR -1 20230501 20240430
#ORGNR 112233-4455
#OMFATTN 20250430
#KPTYP EUBAS97

#KONTO 1010 "Utvecklingsutgifter"
#SRU 1010 7201
#KONTO 1011 "Balanserade utgifter f�r forskning och utveckling"
#SRU 1011 7201
#KONTO 1012 "Balanserade utgifter f�r programvaror"
#SRU 1012 7201

#DIM 1 "Kostnadsst�lle"
#DIM 6 "Projekt"

#IB 0 1221 195687.27 0
#UB 0 1221 195687.27 0
#IB 0 1226 125266.2 0
#UB 0 1226 125266.2 0
#IB 0 1227 38493.89 0
#UB 0 1227 38493.89 0

#PSALDO 0 202505 1630 {} 0 0
#PSALDO 0 202506 1630 {} 2 0
#PSALDO 0 202507 1630 {} 880 0
#PSALDO 0 202508 1630 {} -879 0
#PSALDO 0 202509 1630 {} 1 0
#PSALDO 0 202510 1630 {} 1 0
#PSALDO 0 202511 1630 {} 0 0
#PSALDO 0 202512 1630 {} 1 0
#PSALDO 0 202505 1650 {} -3994 0
#PSALDO 0 202506 1650 {} 879 0
#PSALDO 0 202507 1650 {} -879 0
#PSALDO 0 202509 1650 {} 1997 0
#PSALDO 0 202511 1650 {} -1997 0
#PSALDO 0 202512 1650 {} 826 0

#VER A 1 20250506 "Account:NORDEA Text:PRIS ENL SPEC" 20250812
{
#TRANS 1920 {} -1.85 "" "" 0
#TRANS 6570 {} 1.85 "" "" 0
}
#VER A 2 20250512 "Account:SKV Text:Moms jan 2025 - mars 2025" 20250812
{
#TRANS 1630 {} 1997 "" "" 0
#TRANS 1650 {} -1997 "" "" 0
}
#VER A 3 20250516 "Account:NORDEA Text:BG KONTOINS Message:5050-1030 SK5567828172" 20250812
{
#TRANS 1630 {} -1997 "" "" 0
#TRANS 1920 {} 1997 "" "" 0
}
#VER A 4 20250601 "Account:SKV Text:Int�ktsr�nta" 20250812
{
#TRANS 1630 {} 1 "" "" 0
#TRANS 8314 {} -1 "" "" 0
}
#VER A 5 20250604 "Account:NORDEA Text:PRIS ENL SPEC" 20250812
{
#TRANS 1920 {} -1.85 "" "" 0
#TRANS 6570 {} 1.85 "" "" 0
}

#VER D 1 20250508 "Faktura Fortnox" 20250812
{
#TRANS 2440 {} -615 "" "" 0
#TRANS 5420 {} 492 "" "" 0
#TRANS 2641 {} 123 "" "" 0
#TRANS 3740 {} 0 "" "" 0
}
#VER D 2 20250516 "Telia Faktura" 20250812
{
#TRANS 2440 {} -2359 "" "" 0
#TRANS 6212 {} 1887 "" "" 0
#TRANS 2641 {} 471.75 "" "" 0
#TRANS 3740 {} 0.25 "" "" 0
}
#VER D 3 20250529 "Faktura Loopia AB Webbsupport" 20250812
{
#TRANS 2440 {} -1083.75 "" "" 0
#TRANS 6210 {} 867 "" "" 0
#TRANS 2641 {} 216.75 "" "" 0
}
#VER D 4 20250508 "Faktura Fortnox" 20250812
{
#TRANS 2440 {} -615 "" "" 0
#TRANS 5420 {} 492 "" "" 0
#TRANS 2641 {} 123 "" "" 0
#TRANS 3740 {} 0 "" "" 0
}
#VER D 5 20250516 "Telia Faktura" 20250812
{
#TRANS 2440 {} -2359 "" "" 0
#TRANS 6212 {} 1887 "" "" 0
#TRANS 2641 {} 471.75 "" "" 0
#TRANS 3740 {} 0.25 "" "" 0
}

#VER E 1 20250528 "Account:NORDEA Text:Fortnox Finans AB Message:583371610007452 To:5020-7042" 20250812
{
#TRANS 1920 {} -615 "" "" 0
#TRANS 2440 {} 615 "" "" 0
}
#VER E 2 20250616 "Account:NORDEA Text:TELIA SVERIGE AB Message:19797379252 To:824-3040" 20250812
{
#TRANS 1920 {} -2359 "" "" 0
#TRANS 2440 {} 2359 "" "" 0
}
#VER E 3 20250630 "Account:NORDEA Text:LOOPIA AB (WEBSUPPORT) Message:62500007488343 To:5343-2795" 20250812
{
#TRANS 1920 {} -1083.75 "" "" 0
#TRANS 2440 {} 1083.75 "" "" 0
}
#VER E 4 20250528 "Account:NORDEA Text:Fortnox Finans AB Message:583371610007452 To:5020-7042" 20250812
{
#TRANS 1920 {} -615 "" "" 0
#TRANS 2440 {} 615 "" "" 0
}
#VER E 5 20250616 "Account:NORDEA Text:TELIA SVERIGE AB Message:19797379252 To:824-3040" 20250812
{
#TRANS 1920 {} -2359 "" "" 0
#TRANS 2440 {} 2359 "" "" 0
}

#VER I 1 20250501 "F�rra �rets resultat" 20250817
{
#TRANS 2099 {} -46701.78 "" "" 0
#TRANS 2098 {} 46701.78 "" "" 0
}
#VER M 1 20250630 "Momsrapport 2025-04-01...2025-06-30" 20250812
{
#TRANS 1650 {} 879 "" "" 0
#TRANS 3740 {} 0 "" "" 0
#TRANS 2641 {} -879 "" "" 0
}
#VER M 2 20250930 "Momsrapport_2025-07-01-2025-09-30" 20251111
{
#TRANS 2641 {} -1997.25 "" "" 0
#TRANS 1650 {} 1997 "" "" 0
#TRANS 3740 {} 0.25 "" "" 0
}
#VER M 3 20251231 "Momsrapport 2025-10-01...2025-12-31" 20260212
{
#TRANS 1650 {} 826 "" "" 0
#TRANS 3740 {} 0 "" "" 0
#TRANS 2641 {} -826 "" "" 0
}
)";

