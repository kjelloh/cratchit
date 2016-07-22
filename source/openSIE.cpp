/*
==> Consider to propose an SIE-structure-key-paths for an SIE-file Composite (http://www.sie.se/?page_id=250)?

// Note: Keys spelled in capital letters corresponds to SIE-encoding of #-element (e.g., sie.ADRESS corresponds to SIE file member #ADRESS)
// Note: In key-path the key name is the SIE Element name with spaces replaced with ‘_’ (e.g., SIE Element “distribution address” becomes key “distribution_address”
// Note: Elements that implicitly form a list is assigned a list key [*] (the ‘*’ indicates a possible index 0…n)
// Note: Member element with sub-elements {dimension_no object_no} has been assigned the key “object” (to correspond to referred OBJECT element)
// Note: Specified #RES member “year’s” has bee assigned the key “year_no”
// Note: Specified TRANS member element “object list” has been assigned the list key “objects[]” each conforming to key object (see specification 8.21)

sie.ADRESS.contact
sie.ADRESS.distribution_address
sie.ADRESS.postal_address
sie.ADRESS.tel

sie.BKOD.SNI_code

sie.DIM[*].dimension_no
sie.DIM[*].name

sie.ENHET[*].account_no
sie.ENHET[*].unit

sie.FLAGGA.x

sie.FNAMN.company_name

sie.FNR.company_id

sie.FORMAT
sie.FTYP

sie.GEN.date
sie.GEN.sign

sie.IB.year_no
sie.IB.account
sie.IB.balance
sie.IB.quantity	// optional

sie.KONTO[*].account_no
sie.KONTO[*].account_name

sie.KPTYP

sie.KSUMMA[0…1]

sie.KTYP.account_no
sie.KTYP.account_type

sie.OBJEKT[*].dimension_no
sie.OBJEKT[*].object_no
sie.OBJEKT[*].object_name

sie.OIB[*].year_no
sie.OIB[*].account
sie.OIB[*].object.dimension_no
sie.OIB[*].object.object_no
sie.OIB[*].balance
sie.OIB[*].quantity // optional

sie.OMFATTN

sie.ORGNR.CIN
sie.ORGNR.acq_no
sie.ORGNR.act_no

sie.OUB[*].year_no
sie.OUB[*].account
sie.OUB[*].object.dimension_no
sie.OUB[*].object.object_no
sie.OUB[*].balance
sie.OUB[*].quantity // optional

sie.PBUDGET[*].year_no
sie.PBUDGET[*].period
sie.PBUDGET[*].account
sie.PBUDGET[*].object.dimension_no
sie.PBUDGET[*].object.object_no
sie.PBUDGET[*].balance
sie.PBUDGET[*].quantity

sie.PROGRAM.program_name
sie.PROGRAM.version

sie.PROSA

sie.PSALDO[*].year_no
sie.PSALDO[*].period
sie.PSALDO[*].account
sie.PSALDO[*].object.dimension_no
sie.PSALDO[*].object.object_no
sie.PSALDO[*].balance

sie.RAR[*].year_no
sie.RAR[*].start
sie.RAR[*].end

sie.RES[*].year_no
sie.RES[*].account
sie.RES[*].balance
sie.RES[*].quantity

sie.SIETYP

sie.SRU[*].account
sie.SRU[*].SRU_code

sie.TAXAR

sie.UB[*].year_no
sie.UB[*].account
sie.UB[*].balance
sie.UB[*].quantity

sie.UNDERDIM[*].dimension_no
sie.UNDERDIM[*].name
sie.UNDERDIM[*].superdimension

sie.VALUTA

sie.VER[*].series
sie.VER[*].verno
sie.VER[*].verdate
sie.VER[*].vertext // optional presence
sie.VER[*].regdate
sie.VER[*].sign

sie.VER[*].RTRANS[*].account_no
sie.VER[*].RTRANS[*].objects[*].dimension_no
sie.VER[*].RTRANS[*].objects[*].object_no
sie.VER[*].RTRANS[*].amount
sie.VER[*].RTRANS[*].transdate // optional
sie.VER[*].RTRANS[*].transtext // optional
sie.VER[*].RTRANS[*].quantity // optional presence
sie.VER[*].RTRANS[*].sign

sie.VER[*].TRANS[*].account_no
sie.VER[*].TRANS[*].objects[*].dimension_no
sie.VER[*].TRANS[*].objects[*].object_no
sie.VER[*].TRANS[*].amount
sie.VER[*].TRANS[*].transdate // optional
sie.VER[*].TRANS[*].transtext // optional
sie.VER[*].TRANS[*].quantity // optional presence
sie.VER[*].TRANS[*].sign

sie.VER[*].BTRANS[*].account_no
sie.VER[*].BTRANS[*].objects[*].dimension_no
sie.VER[*].BTRANS[*].objects[*].object_no
sie.VER[*].BTRANS[*].amount
sie.VER[*].BTRANS[*].transdate // optional
sie.VER[*].BTRANS[*].transtext // optional
sie.VER[*].BTRANS[*].quantity // optional presence
sie.VER[*].BTRANS[*].sign

*/

/*
-2) Överväg att ta reda på hur jag skall bokföra de verifikationer jag ännu inte skickat till SE-konsult?

==> Faktura 30871 SE-konsult

==> Tidigare faktura har bokförts så här (Verifikat D:57 kodas som #VER	4	57)
==> Kollade flera stycken och alla verkar se ut så här?

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	4	57	20160229	"FA422/SE Konsult"	20160711
{
#TRANS	2440	{}	-3325,00	""	"FA422/SE Konsult"
#TRANS	2640	{}	665,00	""	"FA422/SE Konsult"
#TRANS	6530	{}	2660,00	""	"FA422/SE Konsult"
}

==> Konton

6530	Redovisningstjänster
2440	Leverantörsskulder
2640	Ingående moms

==> Och betalning så här (Verifikat E:54 kodas som #VER	5	54)

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	5	54	20160318	"BE"	20160701
{
#TRANS	1920	{}	-3325,00	""	"BE"
#TRANS	2440	{}	3325,00	""	"BE/FA422/SE Konsult"
}

==> Konton

1920	PlusGiro
2440	Leverantörsskulder

==> Skattekonto Maj 2016 (inklusive periodmoms)

==> Q: Hur bokföra EU-moms som ingår just denna gång?
==> A: Bokför betalning som vanlig SKV inbet med moms (momskonto 2650 mot inbet skattekonto 1630)?
Ex:
#TRANS	2650	{}	-9418,00	""	"Moms okt-dec"
#TRANS	1630	{}	9418,00	""	"Moms okt-dec"

==> A: EU-inköpet och momsen verkar vara bokförd så här (Verifikat A 91 kodat som #VER	1	91)

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	1	91	20160219	"Digicert"	20160711
{
#TRANS	1920	{}	-3956,13	""	"Digicert"
#TRANS	6230	{}	3956,13	""	"Digicert"
#TRANS	2645	{}	989,00	""	"Digicert"
#TRANS	2615	{}	-989,00	""	"Digicert"
}

==> Konton
1920	PlusGiro
6230	Datakommunikation
2645	Ber ing moms på förvärv från u
2615	Ber utg moms varuförvärv EU-la
==> Notera att EU-momsen bokförts i balans både som ingående (2645) och utgående (2615)?

==> Denna moms verkar ha “tömts” vid periodslutet jan-mars och sammanställts på 2650 (Verifikat A 108 kodat som #VER	1	108)

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	1	108	20160401	"Momstömning jan - mars"	20160711
{
#TRANS	2610	{}	28875,00	""	"Momstömning jan - mars"
#TRANS	2615	{}	989,00	""	"Momstömning jan - mars"
#TRANS	2640	{}	-6562,04	""	"Momstömning jan - mars"
#TRANS	2645	{}	-989,00	""	"Momstömning jan - mars"
#TRANS	2650	{}	-22313,00	""	"Momstömning jan - mars"
#TRANS	3740	{}	0,04	""	"Momstömning jan - mars"
}

==> Konton:

2640	Ingående moms
2610	Utgående moms 25%
2650	Redovisningskonto för moms

2615	Ber utg moms varuförvärv EU-la
2645	Ber ing moms på förvärv från u
3740	Öres- och kronutjämning

==> Not: 2650 inkluderar EU-momsen vilket indikerar att vid betalning behöver inte EU-moms-konton påverkas?

==> Tidigare momstömning med EU-moms har bokförts 140701 apr-jun (Verifikat A:42 kodat som #VER	1	42)

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20140501	20150430

#VER	1	42	20140701	"Momsomföring april-juni"	20160711
{
#TRANS	2610	{}	130499,00	""	"Momsomföring april-juni"
#TRANS	2615	{}	-1135,74	""	"Momsomföring april-juni"
#TRANS	2640	{}	-11120,00	""	"Momsomföring april-juni"
#TRANS	2645	{}	1135,74	""	"Momsomföring april-juni"
#TRANS	2650	{}	-119379,00	""	"Momsomföring april-juni"
}

==> Och inbetalning inklusive periodmoms och EU-moms bokfördes (FÖRSENAD) så här

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20140501	20150430

#VER	1	48	20141013	"Inbet. SKV"	20160711
{
#TRANS	1920	{}	-32000,00	""	"Inbet. SKV"
#TRANS	1630	{}	32000,00	""	"Inbet. SKV"
#TRANS	1630	{}	-500,00	""	"Förseningsavgift"
#TRANS	6992	{}	500,00	""	"Förseningsavgift"
#TRANS	2710	{}	10269,00	""	"Pers.skatt"
#TRANS	1630	{}	-10269,00	""	"Pers.skatt"
#TRANS	2730	{}	12253,00	""	"AGA sept"
#TRANS	1630	{}	-12253,00	""	"AGA sept"
#TRANS	2510	{}	8786,00	""	"Prel.skatt"
#TRANS	1630	{}	-8786,00	""	"Prel.skatt"
#TRANS	1630	{}	4,00	""	"Ränta"
#TRANS	8314	{}	-4,00	""	"Ränta"
}

==> Not: Verkar INTE beröra några EU-konton?
==> Not: Hittar inga EU-momskonton för tidigare år.

==> Tidigare momstömning okt-dec har bokförts så här (Verifikat A 82 kodat som #VER	1	82)

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	1	82	20160104	"Årspris kort"	20160711
{
#TRANS	1920	{}	-144,00	""	"Årspris kort"
#TRANS	6570	{}	144,00	""	"Årspris kort"
#TRANS	2640	{}	-12568,00	""	"momssaldo okt-dec15"
#TRANS	2610	{}	3150,00	""	"momssaldo okt-dec15"
#TRANS	2650	{}	9418,00	""	"momssaldo okt-dec15"
}

==> Konton, exklusive "årspris kort" (jämför momstömning med EU-moms A:108 ovan):

2640	Ingående moms
2610	Utgående moms 25%
2650	Redovisningskonto för moms

==> Tidigare redovisad inbet. SKV inklusive periodmoms okt-dec har bokförts så här (Verifikat A 89 kodat som #VER	1	89)

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	1	89	20160212	"Inbet t SKV"	20160701
{
#TRANS	1920	{}	-23838,00	""	"Inbet t SKV"
#TRANS	1630	{}	23838,00	""	"Inbet t SKV"
#TRANS	2650	{}	-9418,00	""	"Moms okt-dec"
#TRANS	1630	{}	9418,00	""	"Moms okt-dec"
#TRANS	2730	{}	12568,00	""	"AGA jan"
#TRANS	1630	{}	-12568,00	""	"AGA jan"
#TRANS	2710	{}	11270,00	""	"Avdr.skatt jan"
#TRANS	1630	{}	-11270,00	""	"Avdr.skatt jan"
}

==> Konton

1920	PlusGiro
1630	Skattekonto
2730	Lagstadgade sociala avgifter/l
2710	Personalskatt

2650	Redovisningskonto för moms

==> Telia Faktura 160517

==> Tidigare Telia-faktura verkar vara bokförd så här (Verifikat D:42 kodad som #VER	4	42)

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	4	42	20151116	"FA407/Telia"	20160711
{
#TRANS	2440	{}	-1330,00	""	"FA407/Telia"
#TRANS	2640	{}	265,94	""	"FA407/Telia"
#TRANS	6212	{}	1064,06	""	"FA407/Telia"
}

==> Konton

2440	Leverantörsskulder
2640	Ingående moms
6212	Mobiltelefon

==> Och betalning verkar vara bokförd så här (Verifikat E:41 kodat som #VER	5	41)

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	5	41	20151216	"BE"	20160701
{
#TRANS	1920	{}	-1330,00	""	"BE"
#TRANS	2440	{}	1330,00	""	"BE/FA407/Telia"
}

==> Konton

1920	PlusGiro
2440	Leverantörsskulder

==> Privat kostnad iTunes 17 Maj

==> Detta verkar vara en tidigare iTunes som bokförts privat (Verifikat A:2 kodat som #VER	1	2)

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	1	2	20150506	"ITunes"	20160711
{
#TRANS	1920	{}	-49,00	""	"ITunes"
#TRANS	2893	{}	49,00	""	"ITunes"
}

==> Konton

1920	PlusGiro
2893	Skulder närstående personer, k

==> Skattekonto Juni 2016 (ej periodmoms)

==> Tidigare redovisad inbet SKV har bokförts så här (Verifikat A 111 kodat som #VER	1	111)
==> Kollade några andra SKV-verifikat och alla “vanliga” (Löneskatt och avg) verkar se ut så här.

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	1	111	20160414	"Inbet. till SKV"	20160711
{
#TRANS	1920	{}	-23838,00	""	"Inbet. till SKV"
#TRANS	1630	{}	23838,00	""	"Inbet. till SKV"
#TRANS	2730	{}	12568,00	""	"AGA mars"
#TRANS	1630	{}	-12568,00	""	"AGA mars"
#TRANS	2710	{}	11270,00	""	"Avdr. skatt mars"
#TRANS	1630	{}	-11270,00	""	"Avdr. skatt mars"
}

==> Konton

1920	PlusGiro
1630	Skattekonto
2730	Lagstadgade sociala avgifter/l
2710	Personalskatt

==> Verkar som vi skall göra momstömning för apr-jun den 1 Juli?

==> Tidigare bokföring verkar göra en momstömning för varje avslutat momsperiod (t.ex okt-dec 2015)


#TRANS	2640	{}	-12568,00	""	"momssaldo okt-dec15"
#TRANS	2610	{}	3150,00	""	"momssaldo okt-dec15"
#TRANS	2650	{}	9418,00	""	"momssaldo okt-dec15"

2640	Ingående moms
2610	Utgående moms 25%
2650	Redovisningskonto för moms


==> Faktura Binero 160529

==> Tidigare Bindero-faktura på 702 SEK är bokförd så här (Verifikat D:16 kodat som #VER	4	16)

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	4	16	20150828	"FA381/Binero"	20160701
{
#TRANS	2440	{}	-702,00	""	"FA381/Binero"
#TRANS	2640	{}	140,00	""	"FA381/Binero"
#TRANS	6230	{}	562,00	""	"FA381/Binero"
}

==> Konton

2440	Leverantörsskulder
2640	Ingående moms
6230	Datakommunikation

==> Not: ALLA Binero-fakturor verkar bokas som datakommunikation (även domäner)?
==> Not: Jag borde egentligen boka domäner på något annat?

==> Transaktioner PG-konto Maj 2016

==> Betalt Telia-faktura
==> Betalt Beanstalk
==> Utbet Lön
==> iTunes (redan bokförd ovan)
==> Betalt SE-faktura
==> SKV inbet (redan bokförd ovan)?
==> Pris enligt spec.
==> Betalt Beanstalk

==> Transaktioner PG-konto Juni 2016

==> Se pdf.

==> Faktura SE-konsult 160630

==> Se ovan

Verkar vara alla för nu? ...


-1) Överväg att dokumentera de konton som dokumenterade verifikationer använder?

==> Se bifogad excel-file “ITFIED - Kontoplan - 160711 190841” som Edison rapporterar som den aktuella kontoplanen?

2893 "Skulder närstående personer, k"
4010 “Inköp varor och material” verkar användas till alla möjliga inköp?

1) Överväg att dokumentera vanliga verifikationer i den “löpande bokföringen” (Serie A)

==> Se bifogad excel-fil “ITFIED - Verifikatlista - 160711 185592.xlsx” för huvudbok Serie A “Löpande bokföring”




==> Detta verkar vara skickad kund-faktura till Swedbank

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	1	113	20160430	"Kundfaktura 83 3/11"	20160711
{
#TRANS	1790	{}	57273,00	""	"Kundfaktura 83 3/11"
#TRANS	3590	{}	-57273,00	""	"Kundfaktura 83 3/11"
}

==> Detta verkar vara en betalning som är privat

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	1	2	20150506	"ITunes"	20160711
{
#TRANS	1920	{}	-49,00	""	"ITunes"
#TRANS	2893	{}	49,00	""	"ITunes"
}

==> Detta verkar vara ett kvitto på Kjell&Compandy betalt med kort

#FNR	"ITFIED"
#FNAMN	"The ITfied AB"
#ORGNR	"556782-8172"
#RAR	0	20150501	20160430

#VER	1	104	20160323	"Kjell & Company"	20160711
{
#TRANS	1920	{}	-648,90	""	"Kjell & Company"
#TRANS	2640	{}	130,00	""	"Kjell & Company"
#TRANS	5410	{}	519,00	""	"Kjell & Company"
#TRANS	3740	{}	-0,10	""	"Kjell & Company"
}

*/