# Resources used by cratchit

The cratchit app is able to generate and interact with external services.

This 'resources' folder is a work in progress to gatcher files with information and specifications defined by actors in the book keeping environment and the swdish rax agency and bolagdverket web services.


## 'Latest' resources

AS of 20250826 the folder resources/_Nyheter_from_beskattningsperiod_2024P4 contains the files in zip folder downloaded from SKV.

See [_Nyheter_from_beskattningsperiod_2024P4/README](_Nyheter_from_beskattningsperiod_2024P4/README.md);

## 'Older' resources 

These resources has been downloaded from the web as follows.




file: resources/INK1_SKV2000-31-02-0021-01_SKV269.xls
file: resources/INK2_SKV2002-30-01-20-02_SKV269.xls
file: resources/K10_SKV2110-34-04-21-02.xls
	URL: https://skatteverket.se/privat/etjansterochblanketter/blanketterbroschyrer/broschyrer/info/269
	These are selected files from the downloaded file structure from SKV.

file: resources/INK1_SKV2000-31-02-0021-01_SKV269.csv
file: resources/K10_SKV2110-34-04-21-02.csv
	URL: - (created from csv-export of SKV xls-files with the same name
	These are csv-files created from xls-files with the same name for import
	to cratchit as base for SRU-file creation.

file: resources/Exempelfil_01_Vanliga_lontagare_v1.1.9.xml
	URL: *Swedish Tax agency*
	This is a TAX Returns form for upload to Swedish tax agency

file: resources/momsexempel_v6.txt
	URL: *Swedish tax agency*
	This file is an example of an eskd-file (XML format)
	with a VAT Returns form for upload to Swedish tax agency

file: resources/Kontoplan-2022.csv
file: resources/Kontoplan-2022.xlsx
	URL: https://www.bas.se/kontoplaner/
	These are BAS account plan 2022 in excel
	The CSV-file was created in macOS by exporting from Numbers

file: resources/INK2_19-P1-exkl-version-2.csv
file: resources/INK2_19-P1-exkl-version-2.xls
file: resources/INK2_19_P1-intervall-vers-2.csv
file: resources/INK2_19_P1-intervall-vers-2.xls
	URL: https://www.bas.se/kontoplaner/sru/
	These are mapping between SKV SRU-codes and BAS accounts
	for INK2 Income TAX Return.
	The csv-files are exported from macOS Numbers from correspodning xls-file.
	File resources/INK2_19_P1-intervall-vers-2.csv us easier to parse for an algorithm
	as BAS accounts are givven in explicit intervals.