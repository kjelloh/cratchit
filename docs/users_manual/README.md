# cratchit users manual

Cratchit currently run by switching between 'zeroth' varaint and 'first' variant.

When cratchit is started it begins in 'zeroth' varaint which exposes a command line input user interface.

The command 'q' exits from 'zeroth' variant and enters 'first' variant.

The 'first' variant exposes an NCurses text window user interface with three panes. 

* The top pane shows currently selected 'data'.
* The middle pane shows mapping from sibgle keyes to 'states'.
* The bottom pane shows the states 'breadcrumb' and the input prompt. 

## Creating a VAT Returns report in zeroth variant

### Importing bank and tax account statement csv-files

The cratchit app will read account statement csv-file in the cratchit sub-folder 'from_bank_or_skv'.

In this example we are running cratchit from a folder called 'workspace'.

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit % tree workspace/from_bank_or_skv 
workspace/from_bank_or_skv
├── PLUSGIROKONTO FTG 51 86 87-9 - 2025-07-20 12.01.10.csv
├── bokf_trans_165567828172.skv
└── consumed
    ├── PLUSGIROKONTO FTG 51 86 87-9 - 2024-10-27 15.33.59.csv
    ├── PLUSGIROKONTO FTG 51 86 87-9 - 2025-02-11 11.12.22.csv
    ├── PLUSGIROKONTO FTG 51 86 87-9 - 2025-05-09 16.54.45.csv
    ├── PLUSGIROKONTO FTG 51 86 87-9 - 20250401-20250430.csv
    ├── SPARKONTO FÖRETAG 3259 23 17244 - 2024-10-27 15.34.21.csv
    ├── SPARKONTO FÖRETAG 3259 23 17244 - 20240501-20250430.csv
    ├── SPARKONTO FÖRETAG 3259 23 17244 - 2025-02-11 11.12.58.csv
    ├── SPARKONTO FÖRETAG 3259 23 17244 - 2025-05-09 17.00.26.csv
    └── bokf_trans_165567828172.skv

2 directories, 11 files
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit % 
```

As of this writing cratchit can read NORDEA bank statement file (in this example 'PLUSGIROKONTO FTG 51 86 87-9 - 2025-07-20 12.01.10.csv') and swedish tax agency tax account statement csv-file (in this example 'bokf_trans_165567828172.skv).

The files are processed and turned into 'tagged amounts' when cratchit is started. The result is logged to the console.

NOTE: AS of this writing a lot of development traces are also output into the console.

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit/workspace % ./cratchit 
Argument 0: ./cratchit
cratchit/0.6: Hello World Debug!
  cratchit/0.6: __aarch64__ defined
  cratchit/0.6: __cplusplus202302
  cratchit/0.6: __GNUC__4
  cratchit/0.6: __GNUC_MINOR__2
  cratchit/0.6: __clang_major__16
  cratchit/0.6: __apple_build_version__16000026
cratchit/0.6 test_package

current_date_to_year_id_map(financial_year_start_month:5,index_count:7)
        index:-1 range:20240501...20250430
        index:-2 range:20230501...20240430
        index:-3 range:20220501...20230430
        index:-4 range:20210501...20220430
        index:-5 range:20200501...20210430
        index:-6 range:20190501...20200430
        index:0 range:20250501...20260430
model_from_environment
date_ordered_tagged_amounts_from_sie_environment
Opening Saldo Date:20230501
        saldo_ta :  20230501 162495,27
        |--> "BAS=1221"
        |--> "IB=True"
        saldo_ta :  20230501 105496,03
        |--> "BAS=1226"
        |--> "IB=True"
        saldo_ta :  20230501 36605,97
        |--> "BAS=1227"
        |--> "IB=True"
        saldo_ta :  20230501 -15408,44
        |--> "BAS=1228"
        |--> "IB=True"
        saldo_ta :  20230501 -236867,71
        |--> "BAS=1229"
        |--> "IB=True"
        saldo_ta :  20230501 375,09
        |--> "BAS=1610"
        |--> "IB=True"
        saldo_ta :  20230501 630,00
        |--> "BAS=1630"
        |--> "IB=True"
        saldo_ta :  20230501 0,00
        |--> "BAS=1650"
        |--> "IB=True"
        saldo_ta :  20230501 36147,89
        |--> "BAS=1920"
        |--> "IB=True"
        saldo_ta :  20230501 217551,83
        |--> "BAS=1930"
        |--> "IB=True"
        saldo_ta :  20230501 156,75
        |--> "BAS=1999"
        |--> "IB=True"
        saldo_ta :  20230501 -100800,00
        |--> "BAS=2081"
        |--> "IB=True"
        saldo_ta :  20230501 -278845,23
        |--> "BAS=2091"
        |--> "IB=True"
        saldo_ta :  20230501 0,00
        |--> "BAS=2098"
        |--> "IB=True"
        saldo_ta :  20230501 84192,50
        |--> "BAS=2099"
        |--> "IB=True"
        saldo_ta :  20230501 -5459,55
        |--> "BAS=2440"
        |--> "IB=True"
        saldo_ta :  20230501 1331,46
        |--> "BAS=2641"
        |--> "IB=True"
        saldo_ta :  20230501 -2601,86
        |--> "BAS=2893"
        |--> "IB=True"
        saldo_ta :  20230501 -5000,00
        |--> "BAS=2898"
        |--> "IB=True"
date_ordered_tagged_amounts_from_sie_environment
Opening Saldo Date:20240501
        saldo_ta :  20240501 162495,27
        |--> "BAS=1221"
        |--> "IB=True"
        saldo_ta :  20240501 114976,20
        |--> "BAS=1226"
        |--> "IB=True"
        saldo_ta :  20240501 38223,49
        |--> "BAS=1227"
        |--> "IB=True"
        saldo_ta :  20240501 -15408,44
        |--> "BAS=1228"
        |--> "IB=True"
        saldo_ta :  20240501 -257552,58
        |--> "BAS=1229"
        |--> "IB=True"
        saldo_ta :  20240501 375,09
        |--> "BAS=1610"
        |--> "IB=True"
        saldo_ta :  20240501 645,00
        |--> "BAS=1630"
        |--> "IB=True"
        saldo_ta :  20240501 1026,00
        |--> "BAS=1650"
        |--> "IB=True"
        saldo_ta :  20240501 8129,56
        |--> "BAS=1920"
        |--> "IB=True"
        saldo_ta :  20240501 111124,72
        |--> "BAS=1930"
        |--> "IB=True"
        saldo_ta :  20240501 -100800,00
        |--> "BAS=2081"
        |--> "IB=True"
        saldo_ta :  20240501 -348498,23
        |--> "BAS=2091"
        |--> "IB=True"
        saldo_ta :  20240501 0,00
        |--> "BAS=2093"
        |--> "IB=True"
        saldo_ta :  20240501 278845,50
        |--> "BAS=2098"
        |--> "IB=True"
        saldo_ta :  20240501 30547,01
        |--> "BAS=2099"
        |--> "IB=True"
        saldo_ta :  20240501 896,25
        |--> "BAS=2440"
        |--> "IB=True"
        saldo_ta :  20240501 156,75
        |--> "BAS=2641"
        |--> "IB=True"
        saldo_ta :  20240501 -181,59
        |--> "BAS=2893"
        |--> "IB=True"
        saldo_ta :  20240501 -25000,00
        |--> "BAS=2898"
        |--> "IB=True"

BEGIN: Processing files in "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv"

BEGIN File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/.DS_Store"
to_tagged_amounts("/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/.DS_Store")
*Ignored* "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/.DS_Store" (Failed to understand file content)
END File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/.DS_Store"

BEGIN File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/consumed"
END File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/consumed"

BEGIN File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/PLUSGIROKONTO FTG 51 86 87-9 - 2025-07-20 12.01.10.csv"
to_tagged_amounts("/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/PLUSGIROKONTO FTG 51 86 87-9 - 2025-07-20 12.01.10.csv")
        Valid entries count:8
        Consumed account statement file move DISABLED = NOT moved to "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/consumed/PLUSGIROKONTO FTG 51 86 87-9 - 2025-07-20 12.01.10.csv"
END File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/PLUSGIROKONTO FTG 51 86 87-9 - 2025-07-20 12.01.10.csv"

BEGIN File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/bokf_trans_165567828172.skv"
to_tagged_amounts("/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/bokf_trans_165567828172.skv")
        Valid entries count:6
        Consumed account statement file move DISABLED = NOT moved to "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/consumed/bokf_trans_165567828172.skv"
END File: "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv/bokf_trans_165567828172.skv"
END: Processed Files in "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/from_bank_or_skv"
date_ordered_tagged_amounts_from_account_statement_files RETURNS 14 entries
BEGIN: Processing files in "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/skv_specs"
END: Processing files in "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/skv_specs"
sie[-1] from "sie_in/TheITfiedAB20250211_145723.se"
sie[current] from "sie_in/TheITfiedAB20250510_190029.se"
DESIGN_UNSUFFICIENCY - No proper synchronization of tagged amounts with SIE files yet in place (dublicate SIE entries may remain in tagged amounts)
Init from "/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/cratchit.env"
cratchit:>

```

We may now process these account statement entries as input for our VAR returns report


### cratchit:> -tas 20250401 20250630

The '-tas' command selects 'Tagged Amounts' in a date range. So the command '-tas 20250401 20250630' selects tagged amounts from 1'th of april to 30'th of june 2025.

```sh
<SELECTED>
0.  20250401 656,00
        |--> "Account=SKV"
        |--> "Text=Ingående saldo 2025-04-01"
        |--> "type=saldo"
1.  20250403 -11,10
        |--> "Account=NORDEA"
        |--> "Belopp=-11,10"
        |--> "Bokföringsdag=2025/04/03"
        |--> "Namn=PRIS ENL SPEC"
        |--> "Saldo=689134"
        |--> "Text=PRIS ENL SPEC"
        |--> "Valuta=SEK"
        |--> "Ytterligare detaljer=PRIS ENL SPEC"
2.  20250405 1,00
        |--> "Account=SKV"
        |--> "Belopp=1"
        |--> "Bokföringsdag=2025-04-05"
        |--> "Rubrik=Intäktsränta"
        |--> "Text=Intäktsränta"
3.  20250422 -323,75
...
```

### cratchit:tas>-has_tag Account

In this example the range of tagged amounts only contains such created from account statements.

But lets look at an example where the tagged amounts also contains already registered financial events (SIE entries aggregating BAS account entries).

```sh
cratchit:tas>-tas 20250101 20250331

<SELECTED>
0.  20250101 654,00
        |--> "Account=SKV"
        |--> "Text=Ingående saldo 2025-01-01"
        |--> "type=saldo"
1.  20250107 -1303,70
        |--> "BAS=1920"
        |--> "Ix=0"
        |--> "parent_SIE=A48"
2.  20250107 1303,70
        |--> "BAS=6570"
        |--> "Ix=1"
        |--> "parent_SIE=A48"
3.  20250107 1303,70
        |--> "SIE=A48"
        |--> "_members=79e6ff1babea1366^0ef6f017e667db16"
        |--> "type=aggregate"
        |--> "vertext=Account:NORDEA Text:PRIS ENL SPEC"
4.  20250107 -1303,70
        |--> "Account=NORDEA"
        |--> "Belopp=-1303,70"
        |--> "Bokföringsdag=2025/01/07"
        |--> "Namn=PRIS ENL SPEC"
        |--> "Saldo=5851280"
        |--> "Text=PRIS ENL SPEC"
        |--> "Valuta=SEK"
        |--> "Ytterligare detaljer=PRIS ENL SPEC"
5.  20250110 3240,00
        |--> "BAS=6250"
        |--> "Ix=0"
        |--> "TRANSTEXT=Postbox 1/2-år"
        |--> "parent_SIE=A49"
6.  20250110 810,00
        |--> "BAS=2641"
        |--> "Ix=1"
        |--> "parent_SIE=A49"
7.  20250110 -4050,00
        |--> "BAS=2440"
        |--> "Ix=2"
        |--> "parent_SIE=A49"
8.  20250110 4050,00
        |--> "SIE=A49"
        |--> "_members=b96d0e928fc83ba4^6f362aba04af54bc^6c846f5788dc8eb8"
        |--> "type=aggregate"
        |--> "vertext=Postnord Postbox"
9.  20250111 -996,25
        |--> "BAS=2440"
        |--> "Ix=0"
        |--> "parent_SIE=D14"

...

121.  20250331 1997,50
        |--> "SIE=M4"
        |--> "_members=130dcba2041c0646^bec82b9878b93d5e^a746ff66506071ea"
        |--> "type=aggregate"
        |--> "vertext=Momsrapport 20250101...20250331"
122.  20250331 -1083,75
        |--> "Account=NORDEA"
        |--> "Belopp=-1083,75"
        |--> "Bokföringsdag=2025/03/31"
        |--> "Egna anteckningar=Webhotell Q2"
        |--> "Meddelande=62500003193947"
        |--> "Message=62500003193947"
        |--> "Mottagare=5343-2795"
        |--> "Namn=LOOPIA AB (WEBSUPPORT)"
        |--> "Notes=Webhotell Q2"
        |--> "Saldo=690244"
        |--> "Text=LOOPIA AB (WEBSUPPORT)"
        |--> "To=5343-2795"
        |--> "Valuta=SEK"
        |--> "Ytterligare detaljer=LOOPIA AB (WEBSUPPORT)"
123.  20250331 656,00
        |--> "Account=SKV"
        |--> "Text=Utgående saldo 2025-03-31"
        |--> "type=saldo"
cratchit:tas>
```

To filter out account statement tagged amounts we can use the '-has_tag' command.

```sh
cratchit:tas>-has_tag Account
cratchit:tas>

Options:
The following options are available for Tagged Amounts selection.
<Enter> : Lists the currently selected tagged amounts
<index> - Selects tagged amount with provided index
-has_tag <regular expression> - Keep tagged amounts with tag matching regular expression
-has_not_tag <regular expression> - Keep tagged amounts with tag NOT matching regular expression
-is_tagged <tag name>=<regular expression> - Keep tagged amounts with named tag value matching regular expression
-is_not_tagged <tag name>=<regular expression> - Keep tagged amounts with named tag value NOT matching regular expression
-to_bas_account <bas account number> - Tag current selection of tagged amounts with provided BAS account number.
-amount_trails - Groups tagged amounts on transaction amount and lists them
-aggregates - Reduces tagged amounts to only aggregates (E.g., SIE entries referring to single account tagged amounts)
-todo - Lists tagged amounts subject to 'TODO' actions.
<SELECTED>
0.  20250101 654,00
        |--> "Account=SKV"
        |--> "Text=Ingående saldo 2025-01-01"
        |--> "type=saldo"
1.  20250107 -1303,70
        |--> "Account=NORDEA"
        |--> "Belopp=-1303,70"
        |--> "Bokföringsdag=2025/01/07"
        |--> "Namn=PRIS ENL SPEC"
        |--> "Saldo=5851280"
        |--> "Text=PRIS ENL SPEC"
        |--> "Valuta=SEK"
        |--> "Ytterligare detaljer=PRIS ENL SPEC"

...

19.  20250331 -1083,75
        |--> "Account=NORDEA"
        |--> "Belopp=-1083,75"
        |--> "Bokföringsdag=2025/03/31"
        |--> "Egna anteckningar=Webhotell Q2"
        |--> "Meddelande=62500003193947"
        |--> "Message=62500003193947"
        |--> "Mottagare=5343-2795"
        |--> "Namn=LOOPIA AB (WEBSUPPORT)"
        |--> "Notes=Webhotell Q2"
        |--> "Saldo=690244"
        |--> "Text=LOOPIA AB (WEBSUPPORT)"
        |--> "To=5343-2795"
        |--> "Valuta=SEK"
        |--> "Ytterligare detaljer=LOOPIA AB (WEBSUPPORT)"
20.  20250331 656,00
        |--> "Account=SKV"
        |--> "Text=Utgående saldo 2025-03-31"
        |--> "type=saldo"
cratchit:tas>
```

Now we can turn these tagged amounts into 'HADs' (Heading Amount Date) entries.

### cratchit:tas>-to_hads

When a range of tagged amounts has been selected (using the '-tas' command and required filtering command) the command '-to_hads' creates 'HADs' from these tagged amounts.

A 'HAD' is a 'Heading Amount Date' entry which serves as a seed for a financial event. So a HAD is later edited to become a full entry about the event.

```sh
cratchit:tas>-to_hads

Creating Heading Amount Date entries (HAD:s) from selected Tagged Amounts
cratchit:tas>
```

### cratchit:tas>-hads

The '-hads' command enters the 'hads' state (prompted as 'cratchit:had>').

As of this writing this command also triggers generated a HAD for the VAT returns report.

```sh
cratchit:tas>-hads

        to_box_49_amount += [10]:0.00 = 0.00
        to_box_49_amount += [30]:0.00 = 0.00
        to_box_49_amount += [60]:0.00 = 0.00
        to_box_49_amount += [48]:0.00 = 0.00
Period:20250401...20250930
        [5]
        [10]
        [20]
        [30]
        [39]
        [48]
        [49]
                00000000 account_amounts[0] += 0.00 saldo:0.00
        [50]
        [60]
        to_box_49_amount += [10]:0.00 = 0.00
        to_box_49_amount += [30]:0.00 = 0.00
        to_box_49_amount += [60]:0.00 = 0.00
        to_box_49_amount += [48]:0.00 = 0.00
Period:20250101...20250630
        [5]
        [10]
        [20]
        [30]
        [39]
        [48]
                20250110 account_amounts[2641] += 810.00 saldo:810.00
                20250111 account_amounts[2641] += 199.25 saldo:1009.25
                20250115 account_amounts[2641] += 47.25 saldo:1056.50
                20250205 account_amounts[2641] += 123.00 saldo:1179.50
                20250207 account_amounts[2641] += 64.75 saldo:1244.25
                20250217 account_amounts[2641] += 471.75 saldo:1716.00
                20250227 account_amounts[2641] += 216.75 saldo:1932.75
                20250319 account_amounts[2641] += 64.75 saldo:1997.50
                20250331 account_amounts[2641] += -1997.50 saldo:0.00
        [49]
                00000000 account_amounts[0] += 0.00 saldo:0.00
        [50]
        [60]
Please select:
  0   nnnn Dummy HAD 6.00 20250706
  1   nnnn Dummy HAD 5.00 20250705
  2   nnnn Dummy HAD 4.00 20250704
  3   nnnn Dummy HAD 3.00 20250703
  4   nnnn Dummy HAD 2.00 20250702
  5   nnnn Dymmy HAD 1.00 20250701
  6   nnnn Account:SKV Text:Utgående saldo 2025-06-30 658.00 20250630
  7   nnnn Kvitto Anthropic Claude 180.00 20250630
  8   nnnn Account:NORDEA Text:LOOPIA AB (WEBSUPPORT) Message:62500007488343 To:5343-2795 -1083.75 20250630
  9   nnnn Account:NORDEA Text:TELIA SVERIGE AB Message:19797379252 To:824-3040 -2359.00 20250616
  10   nnnn Account:NORDEA Text:PRIS ENL SPEC -1.85 20250604
  11   nnnn Account:SKV Text:Intäktsränta 1.00 20250601
  12   nnnn Faktura Loopia AB Webbsupport 1083.75 20250529
  13   nnnn Account:NORDEA Text:Fortnox Finans AB Message:583371610007452 To:5020-7042 -615.00 20250528
  14   nnnn Telia Faktura 2359.00 20250516
  15   nnnn Account:NORDEA Text:BG KONTOINS Message:5050-1030 SK5567828172 1997.00 20250516
  16   nnnn Utbetalning Moms 1997.00 20250514
  17   nnnn Account:SKV Text:Utbetalning -1997.00 20250514
  18   nnnn Account:SKV Text:Moms jan 2025 - mars 2025 1997.00 20250512
  19   nnnn Account:SKV Text:Utgående saldo 2025-05-10 657.00 20250510
  20   nnnn Faktura Fortnox 615.00 20250508
  21   nnnn Account:NORDEA Text:PRIS ENL SPEC -1.85 20250506
  22   nnnn Account:NORDEA Text:LOOPIA AB (WEBSUPPORT) Message:62500004169649 To:5343-2795 -323.75 20250422
  23   nnnn Account:SKV Text:Intäktsränta 1.00 20250405
  24   nnnn Account:NORDEA Text:PRIS ENL SPEC -11.10 20250403
  25   nnnn Account:SKV Text:Ingående saldo 2025-04-01 656.00 20250401
cratchit:had>
```

In this example we can see all the HAD entries created from the previously selected tagged amounts.

But we can also see some prte-existing HADs like '0   nnnn Dummy HAD 6.00 20250706' from testing the app and '14   nnnn Telia Faktura 2359.00 20250516' which was entered manually.

### cratchit:had>-

The command '-' exits current state and returns to root state.

It also shows an experimental 'command help' listing.

```sh
cratchit:had>-

to_previous_state
to_prompt_for_entering_state
Options:
<Heading> <Amount> <Date> : Entry of new Heading Amount Date (HAD) Transaction entry
-hads : lists current Heading Amount Date (HAD) entries
-sie <sie file path> : imports a new input sie file. Please enclose path with "" if it contains space.
-sie : lists transactions in input sie-file
-tas <first date> <last date> : Selects tagged amounts in the period first date .. last date
-tas : Selects last selected tagged amounts
-csv <csv file path> : Imports Comma Seperated Value file of Web bank account transactions
                       Stores them as Heading Amount Date (HAD) entries.
'q' or 'Quit'
cratchit:>
```

### Manual enter of HAD


In the root state, entering 'cratchit:>HAD Example 16,00 20250616' will cause cratchit to parse the input as a HAD (Heading Amount Date). 

```sh
cratchit:>HAD Example 16,00 20250616

  nnnn HAD Example 16.00 20250616
cratchit:>
```
## Deleting a HAD

To delete a HAD you have to first enter the HADs state with the command '-hads'. 

```sh
cratchit:>-hads

...

Please select:
  0   nnnn Dummy HAD 6.00 20250706
  1   nnnn Dummy HAD 5.00 20250705
  2   nnnn Dummy HAD 4.00 20250704
  3   nnnn Dummy HAD 3.00 20250703
  4   nnnn Dummy HAD 2.00 20250702
  5   nnnn Dymmy HAD 1.00 20250701
  6   nnnn Account:SKV Text:Utgående saldo 2025-06-30 658.00 20250630
  7   nnnn Kvitto Anthropic Claude 180.00 20250630
  8   nnnn Account:NORDEA Text:LOOPIA AB (WEBSUPPORT) Message:62500007488343 To:5343-2795 -1083.75 20250630
  9   nnnn HAD Example 16.00 20250616
  10   nnnn Account:NORDEA Text:TELIA SVERIGE AB Message:19797379252 To:824-3040 -2359.00 20250616
  11   nnnn Account:NORDEA Text:PRIS ENL SPEC -1.85 20250604
  12   nnnn Account:SKV Text:Intäktsränta 1.00 20250601
  13   nnnn Faktura Loopia AB Webbsupport 1083.75 20250529
  14   nnnn Account:NORDEA Text:Fortnox Finans AB Message:583371610007452 To:5020-7042 -615.00 20250528
  15   nnnn Telia Faktura 2359.00 20250516
  16   nnnn Account:NORDEA Text:BG KONTOINS Message:5050-1030 SK5567828172 1997.00 20250516
  17   nnnn Utbetalning Moms 1997.00 20250514
  18   nnnn Account:SKV Text:Utbetalning -1997.00 20250514
  19   nnnn Account:SKV Text:Moms jan 2025 - mars 2025 1997.00 20250512
  20   nnnn Account:SKV Text:Utgående saldo 2025-05-10 657.00 20250510
  21   nnnn Faktura Fortnox 615.00 20250508
  22   nnnn Account:NORDEA Text:PRIS ENL SPEC -1.85 20250506
  23   nnnn Account:NORDEA Text:LOOPIA AB (WEBSUPPORT) Message:62500004169649 To:5343-2795 -323.75 20250422
  24   nnnn Account:SKV Text:Intäktsränta 1.00 20250405
  25   nnnn Account:NORDEA Text:PRIS ENL SPEC -11.10 20250403
  26   nnnn Account:SKV Text:Ingående saldo 2025-04-01 656.00 20250401
cratchit:had>
```

Then you can delete a HAD by entering the index of the HAD to delete prefixed with '-'-

```sh
cratchit:had>-9

  nnnn HAD Example 16.00 20250616 REMOVED
cratchit:>
```

You then need to do the command '-hads' again to show the updated list of HADs.

### Prune the HADs to all events to register for the VAT returns report.

In this example we have 20 HADs as a result from placing relevant bank account statement csv-file as well as SKV (Swedish tax agency) tax account statement csv-file into the cratchit folder 'from_bank_or_skv'.

```sh
cratchit:>-hads

...

Please select:
  0   nnnn Kvitto Anthropic Claude 180.00 20250630
  1   nnnn Account:NORDEA Text:LOOPIA AB (WEBSUPPORT) Message:62500007488343 To:5343-2795 -1083.75 20250630
  2   nnnn Account:NORDEA Text:TELIA SVERIGE AB Message:19797379252 To:824-3040 -2359.00 20250616
  3   nnnn Account:NORDEA Text:PRIS ENL SPEC -1.85 20250604
  4   nnnn Account:SKV Text:Intäktsränta 1.00 20250601
  5   nnnn Faktura Loopia AB Webbsupport 1083.75 20250529
  6   nnnn Account:NORDEA Text:Fortnox Finans AB Message:583371610007452 To:5020-7042 -615.00 20250528
  7   nnnn Telia Faktura 2359.00 20250516
  8   nnnn Account:NORDEA Text:BG KONTOINS Message:5050-1030 SK5567828172 1997.00 20250516
  9   nnnn Utbetalning Moms 1997.00 20250514
  10   nnnn Account:SKV Text:Utbetalning -1997.00 20250514
  11   nnnn Account:SKV Text:Moms jan 2025 - mars 2025 1997.00 20250512
  12   nnnn Account:SKV Text:Utgående saldo 2025-05-10 657.00 20250510
  13   nnnn Faktura Fortnox 615.00 20250508
  14   nnnn Account:NORDEA Text:PRIS ENL SPEC -1.85 20250506
  15   nnnn Account:NORDEA Text:LOOPIA AB (WEBSUPPORT) Message:62500004169649 To:5343-2795 -323.75 20250422
  16   nnnn Account:SKV Text:Intäktsränta 1.00 20250405
  17   nnnn Account:NORDEA Text:PRIS ENL SPEC -11.10 20250403
cratchit:had>
```

### Register HADs as book keeping events

In the HADs state (command '-hads' and prompt 'cratchit:had>') we can now just enter the index of the HAD to register as a financial event.

```sh
cratchit:had>17

  nnnn Account:NORDEA Text:PRIS ENL SPEC -11.10 20250403
    0  E16 "Account:NORDEA Text:SVEA INKASSO AKTIEBOLAG Message:4414710655790067 To:486 66 03-6" 20250313
         ? = sort_code: 0x0 : "PlusGiro":1920 "" -4967.00
         ? = sort_code: 0x0 : "Leverantörsskulder":2440 "Inkasso faktisk avgift" -707.00
         ? = sort_code: 0x0 : "Leverantörsskulder":2440 "" 4967.00
         ? = sort_code: 0x0 : "Övriga externa kostnader, ej avdragsgilla":6992 "Inkass faktisk avgift" 707.00
    1  A16 "Account:NORDEA Text:Insättning Message:FÖR UTDELNING From:32592317244" 20230918
         gross = sort_code: 0x3 : "PlusGiro":1920 "" 50000.00
         gross = sort_code: 0x3 : "Företagskonto/checkkonto/affärskonto":1930 "" -50000.00
    2  A32 "Account:NORDEA Message:FRåN SPARKONTO To:5186879" 20240121
         gross = sort_code: 0x3 : "PlusGiro":1920 "" 10000.00
         gross = sort_code: 0x3 : "Företagskonto/checkkonto/affärskonto":1930 "" -10000.00

...

    40  A2 "PRIS ENLIGT SPEC" 20230504
         gross = sort_code: 0x3 : "PlusGiro":1920 "" -1.75
         gross = sort_code: 0x3 : "Bankkostnader":6570 "" 1.75
    41  A8 "Account:NORDEA Text:PRIS ENL SPEC" 20230705
         gross = sort_code: 0x3 : "PlusGiro":1920 "" -3.70
         gross = sort_code: 0x3 : "Bankkostnader":6570 "" 3.70
    42  A14 "Account:NORDEA Text:PRIS ENL SPEC" 20230905
         gross = sort_code: 0x3 : "PlusGiro":1920 "" -1.85
         gross = sort_code: 0x3 : "Bankkostnader":6570 "" 1.85
    43  A29 "Account:NORDEA Text:PRIS ENL SPEC" 20240104
         gross = sort_code: 0x3 : "PlusGiro":1920 "" -1307.40
         gross = sort_code: 0x3 : "Bankkostnader":6570 "" 1307.40
    44  A45 "Account:NORDEA Text:PRIS ENL SPEC" 20240305
         gross = sort_code: 0x3 : "PlusGiro":1920 "" -7.40
         gross = sort_code: 0x3 : "Bankkostnader":6570 "" 7.40

...

    67  A12 "Account:NORDEA Text:KORT PAYPAL *IFIXIT 26 Message:PAYPAL *IFIXIT 2656" 20230801
         gross = sort_code: 0x3 : "PlusGiro":1920 "" -1153.43
         eu_vat vat = sort_code: 0x56 : "Utgående moms omvänd skattskyldighet, 25 %":2614 "" -230.69
         eu_vat vat = sort_code: 0x56 : "Ingående moms":2640 "" 230.69
         gross = sort_code: 0x3 : "Reparation och underhåll (gruppkonto)":5500 "" 1153.43
         eu_purchase = sort_code: 0x2 : "Varuvärde Inlöp annat EG-land (Momsrapport ruta 20)":9021 "" 1153.43
         eu_purchase = sort_code: 0x2 : "Motkonto Varuvärde Inköp EU/Import":9099 "" -1153.43
    68  A _  "Bestallning Inventarie" 20250403
         net = sort_code: 0x4 : "Inventarier":1221  800.00
         gross = sort_code: 0x3 : "Leverantörsskulder":2440  -1000.00
         vat = sort_code: 0x6 : "Debiterad ingående moms":2641  200.00
cratchit:had:je>
```

The cratchit app will list previously registered financial events that somehow matches the HAD selected.

NOT: As of this writing this matching is generous and not restricted in time. So the list can become very long...

But we can find entries like '42  A14 "Account:NORDEA Text:PRIS ENL SPEC" 20230905' which seems to be a sutable template for our HAD.

To choose a templatem just enter the index of the entry to use as a template.

```sh
cratchit:had:je>42

"gross" count:2
Template is an NO VAT, all gross amount transaction :)
vat_type = No VAT
Plain transfer  A _  "Account:NORDEA Text:PRIS ENL SPEC" 20250403
  "PlusGiro":1920 "" -11.10
  "Bankkostnader":6570 "" 11.10
cratchit:had:je:1*at>
```

To see the options at this stage just press Enter.

```sh
cratchit:had:je:1*at>

Options:
0 1 x counter transactions aggregate
1 n x counter transactions aggregate
2 Edit account transactions
3 STAGE as-is
cratchit:had:je:1*at>
```

For this example we just press '3' to register the financial event as proposed.

```sh
cratchit:had:je:1*at>3

"gross" count:2
Template is an NO VAT, all gross amount transaction :)
 A _  "Account:NORDEA Text:PRIS ENL SPEC" 20250403
         gross = sort_code: 0x3 : "PlusGiro":1920 "" -11.10
         gross = sort_code: 0x3 : "Bankkostnader":6570 "" 11.10
 A60 "Account:NORDEA Text:PRIS ENL SPEC" 20250403
  "PlusGiro":1920 "" -11.10
  "Bankkostnader":6570 "" 11.10 STAGED
Please enter a valid had index
cratchit:had>
```

The HAD is now registered (staged) as the financial event A60

Note: The the entry is 'STAGED' means it is detected as 'not posted' to any external book keeping system using an SIE export. Or rather, the latest SIE import does NOT contain this entry.

We can now do command '-hads' again and process entry 16

```sh
cratchit:had>-hads

...

  15   nnnn Account:NORDEA Text:LOOPIA AB (WEBSUPPORT) Message:62500004169649 To:5343-2795 -323.75 20250422
  16   nnnn Account:SKV Text:Intäktsränta 1.00 20250405
cratchit:had>16

  nnnn Account:SKV Text:Intäktsränta 1.00 20250405
    0  A44 "Account:SKV Text:Intäktsränta" 20240706
         gross = sort_code: 0x3 : "Avräkning för skatter och avgifter (skattekonto)":1630 "" 2.00
         gross = sort_code: 0x3 : "Skattefria ränteintäkter":8314 "" -2.00
...

    9  A12 "Account:SKV Text:Intäktsränta" 20240601
         transfer gross = sort_code: 0x13 : "Avräkning för skatter och avgifter (skattekonto)":1630 "" 1.00
         transfer gross = sort_code: 0x13 : "Skattefria ränteintäkter":8314 "" -1.00
    10  A43 "Account:SKV Text:Intäktsränta" 20240601
         transfer gross = sort_code: 0x13 : "Avräkning för skatter och avgifter (skattekonto)":1630 "" 1.00
         transfer gross = sort_code: 0x13 : "Skattefria ränteintäkter":8314 "" -1.00
    11  A51 "Account:SKV Text:Intäktsränta" 20250201
         transfer gross = sort_code: 0x13 : "Avräkning för skatter och avgifter (skattekonto)":1630 "" 1.00
         transfer gross = sort_code: 0x13 : "Skattefria ränteintäkter":8314 "" -1.00

...

cratchit:had:je>10

"gross" count:2
"transfer" count:2
vat_type = SKV Interest
Tax free SKV interest  A _  "Account:SKV Text:Intäktsränta" 20250405
  "Avräkning för skatter och avgifter (skattekonto)":1630 "" 1.00
  "Skattefria ränteintäkter":8314 "" -1.00
cratchit:had:je:1*at>3

"gross" count:2
"transfer" count:2
DESIGN INSUFFICIENCY - Unknown JournalEntryVATType SKV Interest
 A _  "Account:SKV Text:Intäktsränta" 20250405
         transfer gross = sort_code: 0x13 : "Avräkning för skatter och avgifter (skattekonto)":1630 "" 1.00
         transfer gross = sort_code: 0x13 : "Skattefria ränteintäkter":8314 "" -1.00
 A61 "Account:SKV Text:Intäktsränta" 20250405
  "Avräkning för skatter och avgifter (skattekonto)":1630 "" 1.00
  "Skattefria ränteintäkter":8314 "" -1.00 STAGED
Please enter a valid had index
cratchit:had>-hads
...

```

Note: As of this writing cratchit logs 'DESIGN INSUFFICIENCY - Unknown JournalEntryVATType SKV Interest' which we can ignore.

### BEWARE: cratchit 'zeroth' does NOT protect against wrong fiscal year!

In this example, processing '16   nnnn Account:SKV Text:Intäktsränta 1.00 20250405' will result in an A62 with the date 20250506 although this is in fact an entry that should go into the fiscal year that starts as 20250501 for the example organisation.

```sh
...
cratchit:had:je:1*at>3

"gross" count:2
Template is an NO VAT, all gross amount transaction :)
 A _  "Account:NORDEA Text:PRIS ENL SPEC" 20250506
         gross = sort_code: 0x3 : "PlusGiro":1920 "" -1.85
         gross = sort_code: 0x3 : "Bankkostnader":6570 "" 1.85
 A62 "Account:NORDEA Text:PRIS ENL SPEC" 20250506
  "PlusGiro":1920 "" -1.85
  "Bankkostnader":6570 "" 1.85 STAGED
Please enter a valid had index
cratchit:had>
```

