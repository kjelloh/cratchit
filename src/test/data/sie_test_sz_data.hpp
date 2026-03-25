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
