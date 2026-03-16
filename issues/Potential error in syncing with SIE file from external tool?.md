# Issue: Potential error in syncing with SIE file from external tool?

## Abstract

I exported an SIE file from Fortnox and saved it as 'sie_in/TheITfiedAB20260112_113456.se'.

In cratchit I executed command '-sie sie_in/TheITfiedAB20260112_113456.se'.

Cratchit found the sie-file and imported it as the SIE 'truth' for the current fiscal year 

## Potential issues

* Does cratchit now agree with fortbnox (same SIE view or the books)?
* Does cratchit still agree with Fortnox about the latest VAT returns (does the conflicting entries affect the VAT returns report)?
* What are the conflicting entries about (are the conflicting entries now under different series/verno)?

## Cratchit console output

Importing SIE to current year from "sie_in/TheITfiedAB20260112_113456.se"

STAGE of cracthit entries FAILED when merging with posted (from external tool)
Entries in sie-file:sie_in/TheITfiedAB20260112_113456.se overrides values in cratchit staged entries
CONFLICTED! : No longer valid entry  A13 "Account:NORDEA Text:KORT             CLAUDE AI SUBSC 26 Message:CLAUDE AI SUBSC 2656" 20250702
    "PlusGiro":1920 "" -205.46
    "Datakommunikation":6230 "" 205.46
OK - valid entry  A14 "Account:NORDEA Text:PRIS ENL SPEC" 20250703
    "PlusGiro":1920 "" -3.70
    "Bankkostnader":6570 "" 3.70
OK - valid entry  A15 "Account:NORDEA Text:Insättning Message:SPAR TILL KORT From:32592317244" 20250722
    "PlusGiro":1920 "" 6000.00
    "Företagskonto/checkkonto/affärskonto":1930 "" -6000.00
CONFLICTED! : No longer valid entry  A16 "Account:NORDEA Text:KORT             CLAUDE AI SUBSC 26 Message:CLAUDE AI SUBSC 2656" 20250801
    "PlusGiro":1920 "" -205.20
    "Datakommunikation":6230 "" 205.20
OK - valid entry  A17 "Account:NORDEA Text:PRIS ENL SPEC" 20250805
    "PlusGiro":1920 "" -1.85
    "Bankkostnader":6570 "" 1.85
OK - valid entry  A18 "SKV Skattekonto Intäktsränta" 20250705
    "Avräkning för skatter och avgifter (skattekonto)":1630 "" 1.00
    "Skattefria ränteintäkter":8314 "" -1.00
OK - valid entry  A19 "SKV Skattekonto SKV Moms april 2025 - juni 2025" 20250713
    "Avräkning för skatter och avgifter (skattekonto)":1630 "" 879.00
    "Momsfordran":1650 "" -879.00
OK - valid entry  A20 "Account:NORDEA Text:BG KONTOINS Message:5050-1030 SK5567828172" 20250822
    "Avräkning för skatter och avgifter (skattekonto)":1630 "" -879.00
    "PlusGiro":1920 "" 879.00
CONFLICTED! : No longer valid entry  A21 "Account:NORDEA Text:KORT             CLAUDE AI SUBSC 26 Message:CLAUDE AI SUBSC 2656" 20250901
    "PlusGiro":1920 "" -204.45
    "Datakommunikation":6230 "" 204.45
OK - valid entry  A22 "Account:NORDEA Text:AVGIFTER NORDEA" 20250903
    "PlusGiro":1920 "" -1.85
    "Bankkostnader":6570 "" 1.85
OK - valid entry  A23 "Skattekonto SKV Intäktsränta" 20250906
    "Avräkning för skatter och avgifter (skattekonto)":1630 "" 1.00
    "Skattefria ränteintäkter":8314 "" -1.00
OK - valid entry  A24 "Moms från Websupport OCR 62440033387542 240228" 20250817
    "Debiterad ingående moms":2641 "" 179.25
    "OBS":1999 "" -179.25
OK - valid entry  A25 "Obokad moms M 4 2024-03-31" 20250817
    "Debiterad ingående moms":2641 "" 156.75
    "OBS":1999 "" -156.75
OK - valid entry  D7 "FAKTURA Postnord" 20250711
    "Leverantörsskulder":2440 "" -3925.00
    "Postbefordran":6250 "Postbox 1/2-år" 3140.00
    "Debiterad ingående moms":2641 "" 785.00
OK - valid entry  D8 "FAKTURA Fortnox" 20250808
    "Leverantörsskulder":2440 "" -615.00
    "Programvaror":5420 "" 492.00
    "Debiterad ingående moms":2641 "" 123.00
    "Öres- och kronutjämning":3740 "" 0.00
OK - valid entry  D9 "FAKTURA Telia" 20250815
    "Leverantörsskulder":2440 "" -2359.00
    "Mobiltelefon":6212 "" 1887.00
    "Debiterad ingående moms":2641 "" 471.75
    "Öres- och kronutjämning":3740 "" 0.25
OK - valid entry  D10 "FAKTURA Websupport" 20250828
    "Leverantörsskulder":2440 "" -1083.75
    "Telekommunikation":6210 "" 867.00
    "Debiterad ingående moms":2641 "" 216.75
OK - valid entry  D11 "FAKTURA Websupport" 20250920
    "Leverantörsskulder":2440 "" -323.75
    "Telekommunikation":6210 "" 259.00
    "Debiterad ingående moms":2641 "" 64.75
OK - valid entry  E7 "Account:NORDEA Text:PostNord Sverige AB Message:903006266022 To:5249-4309" 20250723
    "PlusGiro":1920 "" -3925.00
    "Leverantörsskulder":2440 "" 3925.00
OK - valid entry  E8 "Account:NORDEA Text:Fortnox Finans AB Message:563271743711158 To:5020-7042" 20250828
    "PlusGiro":1920 "" -615.00
    "Leverantörsskulder":2440 "" 615.00
OK - valid entry  E9 "Account:NORDEA Text:TELIA SVERIGE AB Message:20230570259 To:824-3040" 20250915
    "PlusGiro":1920 "" -2359.00
    "Leverantörsskulder":2440 "" 2359.00
OK - valid entry  E10 "Account:NORDEA Text:LOOPIA AB (WEBSUPPORT) Message:62500011564840 To:5343-2795" 20250929
    "PlusGiro":1920 "" -1083.75
    "Leverantörsskulder":2440 "" 1083.75
 year id:current - ALL POSTED OK
cratchit:>
