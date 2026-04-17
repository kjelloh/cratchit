# Consider to fix SIE import 'CONFLICTED! : No longer valid entry' for what seems identical entries?

I have just created VAT Returns report M4 20260331

* I exported '/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/cratchit_2025-05-01_2026-04-30.se' to external tool Fortnox
* And then imported back as 'sie_in/TheITfiedAB20260417_115940.se'

* [startup_log](./startup_log.txt)
* [](./sie_import_command_log.txt)

* For A39 we see:

CONFLICTED! : No longer valid entry

A39 "Account:NORDEA::Anonymous Text:AVGIFTER NORDEA | SEK" 20260107
    "PlusGiro":1920 "" -1303.70
    "Bankkostnader":6570 "" 1303.70

Men:

A39 "Account:NORDEA::Anonymous Text:AVGIFTER NORDEA / SEK" 20260107
    "PlusGiro":1920 "" -1303.70
    "Bankkostnader":6570 "" 1303.70

  - Why does cratchit detect a CONFLICT for what looks like an identical A39?
  - Is the detection mechanism too sensitive to white spaces or some other invisible raw data changes?

A39 "Account:NORDEA::Anonymous Text:AVGIFTER NORDEA | SEK" 20260107
A39 "Account:NORDEA::Anonymous Text:AVGIFTER NORDEA / SEK" 20260107
    "PlusGiro":1920 "" -1303.70
    "PlusGiro":1920 "" -1303.70
    "Bankkostnader":6570 "" 1303.70
    "Bankkostnader":6570 "" 1303.70
