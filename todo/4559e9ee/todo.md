# Fix so that Box 30 for EU-VAT in VAT Returns form is correctly generated

I have now generated VAT Returns report M4 for 2026-03-31

But box 30 was NOT calculated?

* Cratchit user experience:

```text
cratchit:had>2

        to_box_49_amount += [10]:0.00 = 0.00
        to_box_49_amount += [30]:0.00 = 0.00
        to_box_49_amount += [60]:0.00 = 0.00
        to_box_49_amount += [48]:0.00 = 0.00
  ?BAS? Momsrapport 2026-01-01...2026-03-31 2051.00 20260331
UNDERLAG till MOMSRAPPORT
  Fält    Transaktion

  [5]

  [10]

  [20]
        20260331   BAS[9021] += 124.00     saldo:124.00    

  [30]

  [39]

  [48]
        20260331   BAS[2645] += 31.00      saldo:31.00     
        20251107   BAS[2641] += 123.00     saldo:123.00    
        20251117   BAS[2641] += 471.25     saldo:594.25    
        20251128   BAS[2641] += 231.75     saldo:826.00    
        20260111   BAS[2641] += 211.75     saldo:1037.75   
        20260112   BAS[2641] += 807.50     saldo:1845.25   
        20260116   BAS[2641] += 47.25      saldo:1892.50   
        20260227   BAS[2641] += 246.75     saldo:2139.25   
        20260302   BAS[2641] += 162.75     saldo:2302.00   
        20260216   BAS[2641] += 471.75     saldo:2773.75   
        20260319   BAS[2641] += 72.25      saldo:2846.00   
        20251231   BAS[2641] += -826.00    saldo:2020.00   

  [49]
        00000000   BAS[0] += 2051.00    saldo:2051.00   

  [50]

  [60]

cratchit:had:vat:summary:ok?>

Options:
VAT Report Summary ok?
0: No
1: Yes
cratchit:had:vat:summary:ok?>1

        to_box_49_amount += [10]:0.00 = 0.00
        to_box_49_amount += [30]:0.00 = 0.00
        to_box_49_amount += [60]:0.00 = 0.00
        to_box_49_amount += [48]:2051.00 = 2051.00VAT Returns Summary accepted ok
VAT Returns for 2026-01-01...2026-03-31
Created "to_skv/moms_202603.eskd"
<!DOCTYPE eSKDUpload PUBLIC "-//Skatteverket, Sweden//DTD Skatteverket eSKDUpload-DTD Version 6.0//SV" "https://www1.skatteverket.se/demoeskd/eSKDUpload_6p0.dtd">
<eSKDUpload Version="6.0">
  <OrgNr>556782-8172</OrgNr>
  <Moms>
   <Period>202603</Period>
   <ForsMomsEjAnnan>0</ForsMomsEjAnnan>
   <InkopVaruAnnatEg>124</InkopVaruAnnatEg>
   <MomsUlagImport>0</MomsUlagImport>
   <ForsTjSkskAnnatEg>0</ForsTjSkskAnnatEg>
   <MomsUtgHog>0</MomsUtgHog>
   <MomsInkopUtgHog>0</MomsInkopUtgHog>
   <MomsImportUtgHog>0</MomsImportUtgHog>
   <MomsIngAvdr>2051</MomsIngAvdr>
   <MomsBetala>-2051</MomsBetala>
  </Moms>
</eSKDUpload>
Created file "to_skv/periodisk_sammanstallning_26-1_20260416.csv" OK
SKV574008;
SE556782817201;26-1;Ville Vessla;555-244454;ville.vessla@foretaget.se
cratchit:had:vat:file:ok?>1
VAT Report DONE ok
 M _  "Momsrapport 2026-01-01...2026-03-31" 20260331
    "Momsfordran":1650  2051.00
    "Öres- och kronutjämning":3740  0.00
    "Debiterad ingående moms":2641  -2020.00
    "Beräknad ingående moms på förvärv från utlandet":2645  -31.00
    "Varuvärde Inlöp annat EG-land (Momsrapport ruta 20)":9021  -124.00
    "Motkonto Varuvärde Inköp EU/Import":9099 "Avbokning (20) 9021" 124.00
cratchit:had:vat:m_entry:ok?>1

 M4 "Momsrapport 2026-01-01...2026-03-31" 20260331
    "Momsfordran":1650  2051.00
    "Öres- och kronutjämning":3740  0.00
    "Debiterad ingående moms":2641  -2020.00
    "Beräknad ingående moms på förvärv från utlandet":2645  -31.00
    "Varuvärde Inlöp annat EG-land (Momsrapport ruta 20)":9021  -124.00
    "Motkonto Varuvärde Inköp EU/Import":9099 "Avbokning (20) 9021" 124.00 STAGED
cratchit:had>
```
* Box 30 should have contained '20260331   BAS[2645] += 31.00      saldo:31.00 '
* There is also something wrong with box 49 on the form?
  - The SKV web service seems to want to adjust the net VAT to 2020 SEK?
  - And that seems logical as the EU-VAT actually cancels out as can be seen in A48 for an EU purchase.
  - 2650 and 2645 cancels out (as it should as no VAT is actually tranfered in the EU purchase)

```text
*A48 "Account:NORDEA::Anonymous Text:Kortköp 260330 SP LAMYSHOP | SEK" 20260331
    "PlusGiro":1920 "" -157.00
    "Kontorsmateriel":6110 "" 157.00
    "Redovisningskonto för moms":2650 "" -31.00
    "Beräknad ingående moms på förvärv från utlandet":2645 "" 31.00
    "Varuvärde Inlöp annat EG-land (Momsrapport ruta 20)":9021 "" 124.00
    "Motkonto Varuvärde Inköp EU/Import":9099 "" -124.00
```

* So where is the actual error?
  - AHA! M4 is also wrong!
  - M4 should NOT reverse 2656
  - OR: M4 should reverse both 2656 AND 2650?
  - ANd Box 49 should contain ONLY the real Swedish VAT (in this case )

I manually edited M4 to cancel out also 2650

* I dont think I can leave the VAT in 2645 and 2650?
  - If I do I have not fully transferred all VATs to 1650
  - Even though 2646 and 2650 canels eachother oiuyt for EU-VAT
  - All VAT Accounts should be empty after a VAT Returns report?

```text
M4 "Momsrapport 2026-01-01...2026-03-31" 20260331
    "Momsfordran":1650 "" 2020.00
    "Öres- och kronutjämning":3740 "" 0.00
    "Debiterad ingående moms":2641 "" -2020.00
    "Beräknad ingående moms på förvärv från utlandet":2645 "" -31.00
    "Redovisningskonto för moms":2650 "" 31.00
    "Varuvärde Inlöp annat EG-land (Momsrapport ruta 20)":9021 "" -124.00
    "Motkonto Varuvärde Inköp EU/Import":9099 "Avbokning (20) 9021" 124.00
```

I manually edited the eskd-file

* Added: ```text <MomsInkopUtgHog>31</MomsInkopUtgHog> ````
* Adjusted: ```text <MomsBetala>-2020</MomsBetala>```

```text
<!DOCTYPE eSKDUpload PUBLIC "-//Skatteverket, Sweden//DTD Skatteverket eSKDUpload-DTD Version 6.0//SV" "https://www1.skatteverket.se/demoeskd/eSKDUpload_6p0.dtd">
<eSKDUpload Version="6.0">
  <OrgNr>556782-8172</OrgNr>
  <Moms>
   <Period>202603</Period>
   <ForsMomsEjAnnan>0</ForsMomsEjAnnan>
   <InkopVaruAnnatEg>124</InkopVaruAnnatEg>
   <MomsUlagImport>0</MomsUlagImport>
   <ForsTjSkskAnnatEg>0</ForsTjSkskAnnatEg>
   <MomsUtgHog>0</MomsUtgHog>
   <MomsInkopUtgHog>31</MomsInkopUtgHog>
   <MomsImportUtgHog>0</MomsImportUtgHog>
   <MomsIngAvdr>2051</MomsIngAvdr>
   <MomsBetala>-2020</MomsBetala>
  </Moms>
</eSKDUpload>
```

So where is the code?

* It seems to be:

```c++
			std::optional<FormBoxMap> to_form_box_map(SIEArchive const& sie_archive,auto md_ap_predicate) {
        logger::scope_logger log_raii(logger::development_trace,"to_form_box_map");
          // ...
					box_map[30] = to_vat_returns_md_aps(30,sie_archive,md_ap_predicate);
          // ...
```

```c++
			BAS::MDAccountPostings to_vat_returns_md_aps(BoxNo box_no,SIEArchive const& sie_archive,auto md_ap_predicate) {
				auto account_nos = to_accounts(box_no);
        // Log
        if (true) {
          logger::development_trace("to_vat_returns_md_aps: box_no:{} account_nos::size:{}",box_no,account_nos.size());
        }
				return to_md_aps(
           sie_archive
          ,[&md_ap_predicate,&account_nos](BAS::MDAccountPosting const& md_ap) {
					  return (md_ap_predicate(md_ap) and is_any_of_accounts(md_ap,account_nos));
				});
			}
```

```c++
			inline BAS::MDAccountPostings to_md_aps(SIEDocument const& sie_doc,auto const& matches_md_ap) {
				BAS::MDAccountPostings result{};
				auto x = [&matches_md_ap,&result](BAS::MDAccountPosting const& md_ap){
					if (matches_md_ap(md_ap)) result.push_back(md_ap);
				};
				for_each_md_account_posting(sie_doc,x);
				return result;
			}
```