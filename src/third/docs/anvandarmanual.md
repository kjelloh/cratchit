# Användarmanual för cratchit

cratchit är ett bokföringsprogram för terminalen, på svenska.

Manualen beskriver det som finns i cratchit i dag och växer i takt med programmet. Det som ännu inte finns står under [Kommer senare](#kommer-senare).

## Innehåll

1. [Starta och avsluta](#starta-och-avsluta)
2. [Att använda cratchit](#att-använda-cratchit)
3. [Samlingsmappen](#samlingsmappen)
4. [Bokföringar](#bokföringar)
5. [Dina filer](#dina-filer)
6. [Om bokföringen har blivit fel](#om-bokföringen-har-blivit-fel)
7. [Kommer senare](#kommer-senare)
8. [Ordlista](#ordlista)

## Starta och avsluta

<!-- hjälp: start -->
Du står i början av cratchit. Tryck `0` för att öppna samlingsmappen med dina bokföringar. Har mappen du startade cratchit i ingen bokföring, skapar `1` en ny bokföring i den. Tryck `q` två gånger för att avsluta.
<!-- /hjälp -->

Starta cratchit genom att skriva `cratchit` i en terminal och trycka Enter:

```text
Byggd: 14:32:05 2026-11-15 (Release)
Från: /usr/local/bin/cratchit
Version: 0.1.0
Tryck ? för gemensamma val.

  0 Samlingsmapp
Cratchit> 
```

De tre första raderna visar vilket cratchit som körs:

* `Byggd`: när programmet byggdes, och om det är byggt för utveckling (`Debug`) eller för användning (`Release`).
* `Från`: var programfilen ligger.
* `Version`: cratchits version.

Kontrollera raderna när du har byggt eller installerat cratchit. Visar de en gammal tid eller en annan mapp, så kör du ett annat cratchit än du tror.

### Starta i en bokföring

Startar du cratchit i en bokförings mapp, alltså en mapp med en `.cratchit`-mapp som har `project.txt`, öppnar cratchit den bokföringen direkt:

```text
cratchit öppnade bokföringen i arbetsmappen: The ITfied AB
Samlingsmappen kommer ihåg The ITfied AB.
Tryck ? för gemensamma val.

...
Cratchit > Samlingsmapp > The ITfied AB> 
```

* Du står då i bokföringen, som om du hade valt samlingsmappen och sedan bokföringen. Med `-` går du tillbaka till samlingsmappen.
* Samlingsmappen kommer ihåg bokföringen. Nästa gång du startar cratchit i en annan mapp finns den i listan i samlingsmappen.
* cratchit tittar bara i mappen du startar i, inte i mapparna ovanför.
* Två bokföringar med samma `uuid` i `project.txt` räknas som samma bokföring, till exempel en kopia av din bokföring som du provar på. Startar du i kopian kommer samlingsmappen ihåg kopian i stället för originalet, och cratchit säger det: `Samlingsmappen kommer ihåg The ITfied AB här i stället för i …`. Starta i originalet igen för att samlingsmappen ska komma ihåg det.
* En mapp har en bokföring. Har mappen fått en annan bokföring glömmer samlingsmappen den gamla och kommer ihåg den nya.
* Går bokföringen inte att öppna börjar cratchit i början och säger varför.

Tryck `q` två gånger för att avsluta.

### Kommandoraden

cratchit kan också köra ett kommando utan att starta gränssnittet. Med ett kommando gör cratchit bara det och avslutar: inga rader om bygget, ingen logg och ingen samlingsmapp. Varje kommando har sin egen manual, på engelska, som `--help` efter kommandot visar.

| Kommando | Gör |
|---|---|
| `cratchit --walk-pdf --text faktura.pdf` | Skriver pdf:ens text, sida för sida (walk_pdf) |
| `cratchit --walk-pdf faktura.pdf` | Visar pdf:ens uppbyggnad (walk_pdf) |
| `cratchit --sie-to-ar bokföring.se` | Gör årsredovisningen av en SIE-fil, i en ny mapp `<orgnr>_<start>_<slut>` i mappen du står i, med årsredovisningen, en rapport och en kopia av SIE-filen (cratchit_ar) |
| `cratchit --sie-to-ar --trace bokföring.se` | Visar konton och verifikationer bakom varje belopp i årsredovisningen, utan att skriva filer |
| `cratchit --sie-to-ink2 bokföring.se "deklaration 2025"` | Gör inkomstdeklaration 2 (INK2R, INK2S och INK2 sidan 1) av en SIE-fil, med förra årets deklaration (dess `BLANKETTER.SRU` eller mapp) för underskottet, i en ny mapp `<orgnr>_<start>_<slut>`: deklarationen som text, en rapport, en kopia av SIE-filen och, när inget saknas, SRU-filerna till Skatteverkets filöverföring (cratchit_ink2) |
| `cratchit --sie-to-ink2 bokföring.se --previous-deficit 0` | Samma, med förra årets underskott angivet i kronor i stället för förra årets deklaration |
| `cratchit --sie-to-vat bokföring.se` | Listar räkenskapsårets momskvartal med periodens moms, avräknat och debiterat (momsrapporten från take 3) |
| `cratchit --sie-to-vat bokföring.se förra_året.se --period 2026Q2` | Gör momsdeklarationen för kvartalet i en ny mapp `<orgnr>_moms_<första dag>_<sista dag>`: momsrapporten med kontroller, förslaget till momsverifikation (bokför den själv), eSKD-filen till Skatteverkets e-tjänst *Lämna momsdeklaration* och en kopia av SIE-filen. Förra årets SIE-fil behövs när kvartalet börjar i förra räkenskapsåret |
| `cratchit --walk-pdf --help`, `cratchit --sie-to-ar --help`, `cratchit --sie-to-ink2 --help`, `cratchit --sie-to-vat --help` | Kommandots manual |

Texten går till standard output, så du kan spara den, till exempel med `> faktura.txt`. Andra argument känner cratchit inte igen; då säger cratchit det och startar inte.

## Att använda cratchit

### Prompten

Raden som slutar med `>` är prompten. Den visar var du är i programmet, från början till där du står nu:

```text
Cratchit > Samlingsmapp > Lägg till> 
```

Ovanför prompten visar cratchit det som hör till platsen du står på, och sedan valen du har där.

### Välja med en tangent

Varje val har en tangent framför sig. Tryck tangenten, utan Enter, så går du dit:

```text
  1 Befintlig
  2 Ny
Cratchit > Samlingsmapp > Lägg till> 
```

Här tar `1` dig till *Befintlig* och `2` till *Ny*. Menyvalen är korta. Trycker du `?` två gånger förklarar cratchit vad de betyder.

Tryck exakt den tangent som står framför valet. Står det `J Ja` är det versalt `J` som gäller; `j` gör ingenting.

Ett val som inte kan göra något just då, till exempel `>` på den sista sidan av en lista, gör ingenting. Tangenten hamnar inte heller på raden som text.

### Hjälp

Tryck `?` var du än står, så visar cratchit de gemensamma valen. Har platsen en hjälptext säger cratchit det, och trycker du `?` en gång till visar cratchit hjälptexten ur den här manualen. Trycker du `?` ännu en gång visas de gemensamma valen igen.

### Gemensamma val

Några tangenter fungerar överallt. De står inte bland valen. Tryck `?` så visar cratchit dem:

| Tangent | Gör |
|---|---|
| `-` | Tillbaka till platsen före |
| `?` | Visa de gemensamma valen. Tryck igen för hjälp om platsen du står på. |
| Enter | Visa valen igen |
| Esc | Töm raden du skriver på (även Ctrl-U) |
| `q` | Avsluta: cratchit frågar först, och `q` en gång till avslutar (se [Avsluta](#avsluta)) |
| Ctrl-D | Avsluta (även Ctrl-C och Ctrl-Z) |

### Skriva text

Trycker du en tangent som inte är ett val börjar du skriva text. Prompten slutar då med `:` i stället för `>`:

```text
Cratchit > Samlingsmapp > Lägg till: x
```

Medan du skriver är alla tangenter text, även `-`, `?` och `q`.

* **Backspace** suddar det sista tecknet. Suddar du allt väljer du med tangenter igen.
* **Esc** tömmer raden, och sedan väljer du med tangenter igen.
* **Enter** lämnar texten till cratchit.

Tar platsen inte emot text säger cratchit det och låter texten stå kvar, så att du kan sudda den:

```text
Programmet tar inte emot text här.
Cratchit > Samlingsmapp > Lägg till: x
```

### Skriva en sökväg

När cratchit vill ha en mapp står en sökväg redan på raden, och du fortsätter skriva därifrån.

* **Tab** fyller i resten av ett mappnamn. Passar flera mappar fyller cratchit i så långt deras namn är lika. Kan cratchit inte fylla i mer listar den mapparna och skriver raden igen under listan.
* När du letar upp en **SIE-fil** fyller Tab också i filnamn. Listar Tab flera SIE-filer grupperas de på räkenskapsår, med det senaste året först. Inom ett år står den senast skapade filen först, och varje fil har en rad i kolumner:

  ```text
  Cratchit > Ny bokföring här > Från SIE: /Users/anna/Företaget/SIE-filer/
    2026-05-01 – 2027-04-30
      TheITfiedAB20261019_105108.se  pågående       skapad 2026-10-19   17 ver.  t.o.m. 2026-06-30  A10 · D3 · M1
    2025-05-01 – 2026-04-30
      TheITfiedAB20261112_102636.se  fullbordat     skapad 2026-11-12  120 ver.  t.o.m. 2026-04-30  A59 · D26 · M5
      TheITfiedAB20261019_105220.se  ofullbordat    skapad 2026-10-19  116 ver.  t.o.m. 2026-04-30  A59 · D26 · M5
    2015-05-01 – 2016-04-30
      2326 ITFied 1505-1604.se       ofullbordat !  skapad 2016-06-17  247 ver.  t.o.m. 2016-04-30  A113 · D63
    Går inte att läsa
      Sie4 fil tom mars -10.se       Unknown encoding: …
    Mappar
      arkiv/
  ```

  Kolumnerna är filens namn, räkenskapsårets status (se nedan), när filen skapades, hur många verifikationer den har, datumet på den senaste och det sista numret i varje serie. En fil utan räkenskapsår står under *Utan räkenskapsår*, och en fil som inte går att läsa under *Går inte att läsa*, med orsaken. Uppgifter som filen saknar lämnas tomma.

  När Tab fyller i en enda fil, eller när hela namnet redan står på raden, visar cratchit filens innehåll på en rad under sökvägen, med räkenskapsåret och dess status först.

#### Räkenskapsårets status i en SIE-fil

cratchit visar vad SIE-filen säger om sitt räkenskapsår, räknat från dagens datum:

* **pågående**: räkenskapsåret har inte tagit slut än.
* **ofullbordat**: räkenskapsåret är slut, men balansräkningen i filen balanserar inte, och inget resultat är bokfört på 8999 (*Årets resultat*). Det gäller till exempel ett år utan bokslut, eller en fil som skapades innan året var slut.
* **fullbordat**: balansräkningen i filen balanserar, det vill säga årets resultat är bokfört.
* **misstämmer**: ett resultat är bokfört på 8999, men balansräkningen balanserar ändå inte.

Ett `!` efter *ofullbordat* betyder att räkenskapsåret är äldre än förra året, alltså slutade för mer än ett år sedan. Ett sådant år ska vara fullbordat. Kontrollera då om det finns en annan fil för samma år.

*Fullbordat* säger bara att bokföringen i filen hänger ihop och att resultatet är bokfört. Det säger inte att årsredovisningen är inlämnad, eller att året är låst i programmet som skapade filen. cratchit hindrar inte att du väljer en fil med någon status; du avgör om filen passar.
* **`-`** går tillbaka så länge du inte har ändrat sökvägen som stod där. Har du ändrat den tömmer du först raden med Esc, och sedan går `-` tillbaka.
* **Enter** lämnar sökvägen till cratchit. Går det inte att använda den visar cratchit varför, och sökvägen står kvar så att du kan rätta den.

### Avsluta

<!-- hjälp: avsluta -->
Du tryckte `q`. Tryck `q` igen för att avsluta cratchit, eller `N` eller `-` för att gå tillbaka dit du var.
<!-- /hjälp -->

`q` avslutar inte direkt. Det leder först till platsen *Avsluta*, så att ett `q` av misstag inte stänger programmet:

```text
Vill du avsluta programmet?
  q Avsluta
  N Nej
Cratchit > Samlingsmapp > Avsluta> 
```

Två `q` i rad avslutar alltså. Bara litet `q` gör det; stort `Q` är vanlig text. Medan du skriver text är `q` text som alla andra tecken. Ctrl-D, Ctrl-C och Ctrl-Z avslutar direkt, utan att fråga.

### Fel

<!-- hjälp: fel -->
cratchit kunde inte göra det du valde, och orsaken står ovanför prompten. Tryck `-` för att gå tillbaka.
<!-- /hjälp -->

Kan cratchit inte göra det du valde hamnar du alltså på platsen *Fel*.

### Inte än

<!-- hjälp: inte-an -->
Valet du gjorde finns i menyn men cratchit kan inte utföra det än. Vad som saknas står ovanför prompten. Tryck `-` för att gå tillbaka.
<!-- /hjälp -->

Ett val kan finnas i menyn innan cratchit kan utföra det. Det leder då till platsen *Inte än*, som säger vad som saknas, till exempel:

```text
Utvecklaren av cratchit har inte infört att … ännu.
Cratchit > … > Inte än (<valet>)> 
```

Tryck `-` för att gå tillbaka. Just nu gäller det valen i [Årsflöde](#årsflöde) och i dess *Bokslut*.

## Samlingsmappen

<!-- hjälp: samlingsmapp -->
Samlingsmappen listar de bokföringar cratchit kommer ihåg, med namn, organisationsnummer, när de skapades och mappen de ligger i. Tryck en bokförings siffra för att öppna den. Tryck `+` för att lägga till en bokföring.
<!-- /hjälp -->

Tryck `0` i början för att öppna samlingsmappen.

Samlingsmappen listar de bokföringar den kommer ihåg, med tangenterna `1` till `9`, i bokstavsordning efter namn:

```text
/Users/anna/Documents/cratchit
    Namn           Org.nr       Skapad      Mapp
  1 Borttagen AB                            /Users/anna/Borttagen AB  (finns inte längre)
  2 Exempel AB     556677-8899  2026-11-09  Exempel AB
  3 The ITfied AB  556782-8172  2026-10-23  ~/Documents/Bokföringar/The ITfied AB
  + Lägg till bokföring
Cratchit > Samlingsmapp> 
```

* *Mapp* visar var bokföringen ligger. Ligger den i samlingsmappen står bara mappens namn, och ligger den i din hemkatalog börjar sökvägen med `~`.
* Uppgifterna hämtas från bokföringen själv varje gång listan visas. Samlingsmappen kommer bara ihåg var bokföringarna ligger.
* *(finns inte längre)* betyder att mappen är borttagen eller flyttad. Har du flyttat bokföringen lägger du till den igen på det nya stället (se [Flytta en bokföring](#flytta-en-bokföring)).
* *(går inte att öppna)* betyder att mappen finns, men att den inte innehåller den bokföring samlingsmappen kommer ihåg.

### Glömma en bokföring

<!-- hjälp: glom -->
Bokföringen finns inte längre där samlingsmappen tror, eller går inte att öppna. Orsaken står ovanför prompten. Tryck `0` för att samlingsmappen ska glömma den, eller `-` för att gå tillbaka utan att glömma. Inga filer tas bort.
<!-- /hjälp -->

Väljer du en bokföring som är märkt *(finns inte längre)* eller *(går inte att öppna)* visar cratchit varför den inte går att öppna och erbjuder `0` *Glöm*:

```text
Bokföringen fanns i /Users/anna/Borttagen AB, men mappen finns inte längre.
Den kan vara borttagen eller flyttad. Har du flyttat den kan du lägga till den på det nya stället med + i samlingsmappen.
  0 Glöm
Cratchit > Samlingsmapp > Borttagen AB> 
```

Tryck `0`, så glömmer samlingsmappen bokföringen och du kommer tillbaka till samlingsmappen, där den inte längre finns i listan. Glöm tar bara bort bokföringen ur samlingsmappens lista. Inga filer eller mappar tas bort.

Har du fler än nio bokföringar visar cratchit de nio första och säger att det finns fler.

## Bokföringar

En bokföring är bokföringen för en juridisk person, till exempel ett aktiebolag. Den ligger i en egen mapp, som kan ligga var som helst på datorn.

### Öppna en bokföring

<!-- hjälp: bokforing -->
Du har öppnat en bokföring. Ovanför prompten står mappen den ligger i. Valen är bokföringens räkenskapsår: `0` är det år som dagens datum ligger i, och `1`, `2`, `3` är åren bakåt. Efter varje år står hur mycket som är bokfört. Tryck ett års siffra för att öppna året. Tryck `-` för att gå tillbaka.
<!-- /hjälp -->

Tryck bokföringens siffra i samlingsmappen. Prompten får då bokföringens namn, och cratchit visar mappen den ligger i:

```text
/Users/anna/Documents/Bokföringar/The ITfied AB
  0 2026-05-01 – 2027-04-30  bokfört: 37 verifikationer
  1 2025-05-01 – 2026-04-30  bokfört: 116 verifikationer
  2 2024-05-01 – 2025-04-30  bokfört: 115 verifikationer
  3 2023-05-01 – 2024-04-30  bokfört: inget
Cratchit > Samlingsmapp > The ITfied AB> 
```

Valen är bokföringens räkenskapsår. `0` är det aktuella året, det som dagens datum ligger i, och `1`, `2`, `3` är åren före det, på samma sätt som i en SIE-fil men utan minustecken. Listan går tillbaka till det äldsta året med bokföring, och sist står året före det, med `bokfört: inget`. Det året finns där för att du ska kunna importera ett tidigare år från en SIE-fil. När du har gjort det står nästa tidigare år sist, och så vidare. Listan visar varje års första och sista dag. När du öppnar ett år får det ett kort namn i prompten: `2025` för ett år som följer kalenderåret, och till exempel `2025/26` för ett som går över två kalenderår.

**cratchit hanterar för närvarande det aktuella räkenskapsåret och högst 9 äldre.** Listan har plats för tio år, `0`–`9`, och det tomma året före det äldsta bokförda räknas in. Du kan alltså importera bakåt tills listan är full. Har bokföringen fler år syns de inte, och det går inte att importera fler år bakåt; cratchit säger då `cratchit visar bara det aktuella räkenskapsåret och 9 äldre. Bokföringen har fler.` Det är en begränsning tills vidare.

Räkenskapsåren finns oavsett om något är bokfört i dem. När ett nytt räkenskapsår börjar dyker det upp som `0` med `bokfört: inget`, och de andra åren flyttas ett steg. Du behöver inte skapa det. Varje år efter det första är tolv månader och börjar dagen efter förra årets sista dag. Ett första år som är kortare eller längre följs alltså av helt vanliga år. Ett räkenskapsår som läggs om kan du bara ta in från en SIE-fil (se [Importera från SIE](#importera-från-sie)).

```text
  0 2027-05-01 – 2028-04-30  bokfört: inget
  1 2026-05-01 – 2027-04-30  bokfört: 37 verifikationer
```

Ett år där inget är bokfört fungerar som de andra: du kan importera till det och exportera det, och du kan beskriva händelser, läsa kontoutdrag och bokföra i det.

**Bokföringen löper vidare över åren.** Du behöver ingen SIE-fil för ett nytt år: första gången du skapar ett utkast i det lägger cratchit till året i bokföringen (och åren mellan, om du har hoppat över något). Ett år som följer direkt efter ett annat öppnar med förra årets utgående saldon på balanskontona, så ett kontos saldo är alltid det verkliga: den första ingående balansen plus alla konteringar sedan dess. Ändrar du något i ett tidigare år följer saldona med. Resultatkontona börjar på noll varje år; de visar årets resultat. Tills bokslutet för förra året är bokfört ligger dess resultat inte i det egna kapitalet, så balansräkningen går jämnt ut först då. Ingående och utgående balanser behöver du bara ange i rapporterna, som årsredovisningen. Ett äldre år än det äldsta i bokföringen kommer fortfarande bara från en SIE-fil.

### Räkenskapsår

<!-- hjälp: rakenskapsar -->
Du har öppnat ett räkenskapsår i bokföringen. Överst står vilken period det gäller och vad året innehåller. Under det visar cratchit årets verifikationer och utkast, de nyaste först. Listan är bara en vy, raderna är inga val. Tryck `0` för årets arbete i *Årsflöde*, `1` för att exportera året till en SIE-fil, `2` för att importera en SIE-fil för året, eller `3` för årets *Rapporter*. Tryck `-` för att gå tillbaka till bokföringen.
<!-- /hjälp -->

Tryck räkenskapsårets siffra i bokföringen. Prompten får då årets namn:

```text
Räkenskapsår 2025-05-01 – 2026-04-30
Hela året: 116 verifikationer
  M5            2026-04-30         89.98  Momsrapport_2026-04-01-2026-04-30
  A59           2026-04-22       2020.00  Skatt
  E25           2026-04-20        361.25  LOOPIA AB
  …
  E21           2026-03-30       1233.75  LOOPIA AB
  … 106 äldre
  0 Årsflöde
  1 Exportera till SIE
  2 Importera från SIE
  3 Rapporter
Cratchit > Samlingsmapp > The ITfied AB > 2025/26> 
```

Andra raden säger vad listan gäller och vad den innehåller, till exempel `Hela året: 37 verifikationer, 1 utkast`. Är inget bokfört står det `Hela året: inget bokfört`. Än så länge visar listan alltid hela året.

Listan visar de tio nyaste raderna. Finns det fler står det sist hur många äldre som inte visas. En verifikation visar serie och nummer, datum, belopp och text. Ett utkast visar i stället hur långt det har kommit:

| Står det | Betyder |
|---|---|
| `händelse` | Utkastet är en händelse, till exempel en faktura eller ett kvitto: datum och kanske text och belopp, men inga konteringar |
| `kontohändelse` | Utkastet är en kontohändelse, ett belopp in på eller ut från ett konto, till exempel en rad i ett kontoutdrag: en enda kontering, ingen text och inget belopp |
| `samma` | Utkastet har flera delar med samma belopp (utan tecken), en händelse och kontohändelser eller kontohändelser som balanserar, till exempel efter [Samma?](#samma), men kan inte bokföras än. Det kan delas igen med *Inte samma* |
| `obalanserat` | Utkastet har flera delar med olika belopp, men det kan inte bokföras än: det saknar text eller belopp, eller konteringarna balanserar inte eller stämmer inte med beloppet |
| `klart` | Utkastet kan bokföras |

Utkast skapar du i [Utkast](#utkast) under Årsflöde, från ett kontoutdrag eller genom att beskriva händelser.

### Rapporter

<!-- hjälp: rapporter -->
Räkenskapsårets rapporter, som de ser ut i bokföringen just nu. Tryck `1` för balansrapporten, `2` för resultatrapporten, `3` för verifikationslistan och `4` för huvudboken för ett konto. Tryck `-` för att gå tillbaka till räkenskapsåret.
<!-- /hjälp -->

Tryck `3` i räkenskapsåret:

```text
Rapporter för räkenskapsåret 2025/26.
  1 Balansrapport
  2 Resultatrapport
  3 Verifikationslista
  4 Huvudbok
Cratchit > Samlingsmapp > The ITfied AB > 2025/26 > Rapporter> 
```

cratchit läser bokföringen när du väljer en rapport, så rapporten visar det som är bokfört just då. Utkast kommer inte med.

| Rapport | Visar |
|---|---|
| Balansrapport | Varje balanskonto med ingående balans (IB), årets förändring och utgående balans (UB), under *Tillgångar* (konton 1xxx) och *Eget kapital och skulder* (konton 2xxx) med summor. Sist *Beräknat resultat*, resultatkontonas summa, och *Summa*, som är noll när bokföringen går ihop. Beloppen är som de är bokförda: debet positivt och kredit negativt, så en vinst är negativ i *Beräknat resultat*. Under rapporten står var de ingående balanserna kommer ifrån. |
| Resultatrapport | Varje resultatkonto med årets belopp, i grupper efter BAS-kontoklass med summor: *Rörelsens intäkter* (3), *Varor, material och vissa köpta tjänster* (4), *Övriga externa kostnader* (5–6), *Personal, avskrivningar m.m.* (7) och *Finansiella och andra intäkter och kostnader* (8). Sist *Årets resultat*. Här är intäkter positiva och kostnader negativa, så en vinst är positiv. |
| Verifikationslista | Årets verifikationer i bokföringsordning med sina rader i kolumnerna Debet och Kredit, och summor. En rad som har strukits eller lagts till efter att verifikationen registrerades är märkt `(struken)` eller `(tillagd)`, med signaturen när det finns en. En struken rad räknas inte med i summan. |
| Huvudbok | Ett kontos ingående balans, varje rad på kontot i datumordning med saldot efter raden, och utgående balans. |

Har årets resultat bokförts på kontona 8990–8999 i bokslutet, visar resultatrapporten de raderna efter *Årets resultat*, med det som är kvar när de räknas med. Det är noll när resultatet är bokfört. Balansrapporten har då inget beräknat resultat kvar.

Går de ingående balanserna inte ihop säger balansrapporten det. Oftast beror det på att förra årets resultat inte är bokfört på eget kapital än (se *Bokföringen löper vidare över åren* ovan). Är året fört vidare från förra året listar rapporten också de konton där källan, till exempel en SIE-fil, angav en annan ingående balans än den cratchit för vidare.

#### Läsa en rapport

<!-- hjälp: rapport -->
Rapporten visas hel, alla rader på en gång. Är den längre än fönstret skrollar du i terminalen. Tryck `-` för att gå tillbaka till Rapporter.
<!-- /hjälp -->

Ett exempel, från en liten testbokföring med en försäljning och en bankavgift:

```text
Balansrapport 2025-01-01 – 2025-12-31
Test AB

Konto                IB     Förändring             UB  Namn

Tillgångar
1930            1000.00         490.00        1490.00  Bank
                1000.00         490.00        1490.00  Summa tillgångar

Eget kapital och skulder
2099           -1000.00           0.00       -1000.00  Result
               -1000.00           0.00       -1000.00  Summa eget kapital och skulder

                   0.00        -490.00        -490.00  Beräknat resultat
                   0.00           0.00           0.00  Summa

Belopp: debet positivt, kredit negativt. Beräknat resultat är resultatkontonas summa (en vinst är negativ).
Summa är noll när bokföringen går ihop.
Ingående balanser är som källan angav dem (året före finns inte i bokföringen).
Cratchit > Samlingsmapp > Test AB > 2025 > Rapporter > Balansrapport> 
```

#### Huvudbok för ett konto

<!-- hjälp: huvudbok -->
Skriv kontots nummer, till exempel `1930`, och tryck Enter för att se kontots huvudbok för räkenskapsåret. Numret skrivs med bara siffror. Ett konto som inte finns i bokföringen och inte har några rader under året ger ett besked under prompten, och det du skrev står kvar. Tryck `-` för att gå tillbaka till Rapporter.
<!-- /hjälp -->

Tryck `4` i Rapporter, skriv kontonumret och tryck Enter. Huvudboken visas hel, som de andra rapporterna, med prompten `Huvudbok 1930`. Tryck `-` för att komma tillbaka till frågan om kontonummer och se ett annat konto.

### Årsflöde

<!-- hjälp: arsflode -->
Årsflöde samlar arbetet med räkenskapsåret, i den ordning det brukar göras under året och efter det. Tryck valets siffra. `0 Utkast`, `1 Bokfört`, `2 Momsrapport` och `3 Bokslut` leder vidare, `4 Årsredovisning` och `7 Deklaration till SKV` visar hur du gör dem från kommandoraden, de andra valen säger vad som saknas än så länge. Tryck `-` för att gå tillbaka till räkenskapsåret.
<!-- /hjälp -->

Tryck `0` i räkenskapsåret:

```text
Arbetet med räkenskapsåret, i den ordning det brukar göras.
  0 Utkast
  1 Bokfört
  2 Momsrapport
  3 Bokslut
  4 Årsredovisning
  5 Årsstämma
  6 Rapport till Bolagsverket
  7 Deklaration till SKV
Cratchit > Samlingsmapp > The ITfied AB > 2025/26 > Årsflöde> 
```

Valen finns i menyn, men de flesta kan cratchit inte utföra än (se [Inte än](#inte-än) och [Kommer senare](#kommer-senare)).

#### Utkast

<!-- hjälp: utkast -->
Räkenskapsårets utkast, händelser som ännu inte är bokförda, det senaste överst, 20 i taget. `…` överst betyder att det finns nyare, och `…` nederst att det finns äldre. Tryck `>` för nästa sida och `<` för föregående. Vid första och sista sidan gör de ingenting. Skriv ett utkasts nummer som ett ord, till exempel `#3`, och tryck Enter för att öppna det. Under listan står vad du nyss gjorde. Tryck `0` för att beskriva händelser, en i taget, och skapa ett utkast av var och en, `1` för att se vilka utkast som kan vara samma händelse, `2` för att fullborda utkasten ett i taget, `3` för att läsa ett kontoutdrag och skapa utkast av dess rader, `4` för att välja en mapp med pdf-underlag, eller `5` för att göra en bokförd verifikation till ett utkast igen (se [Gör till utkast](#gör-till-utkast)). Tryck `-` för att gå tillbaka till Årsflöde.
<!-- /hjälp -->

Tryck `0` i Årsflöde:

```text
Utkast 2026/27: utkast 1–12 av 12
  #1            2026-09-28      -1308.75  1920  5343-2795 | LOOPIA AB (WEBSUPPORT) | 62600010249540 | Webhotell 12/10 - 11/1 (kontohändelse)
  #2            2026-09-10       -204.68  1920  Kortköp 260910 ANTHROPIC* CLAUDE SU | ANTHROPIC* CLAU 2656 (kontohändelse)
  #3            2026-09-05          1.00  1630  Intäktsränta (kontohändelse)
  …
  #12           2026-07-03         -3.70  1920  AVGIFTER NORDEA (kontohändelse)
  0 Beskriv händelser
  1 Samma?
  2 Fullborda utkast
  3 Läs kontoutdrag
  4 Läs pdf-underlag
  5 Gör bokfört till utkast
  < Föregående sida
  > Nästa sida
Cratchit > Samlingsmapp > The ITfied AB > 2026/27 > Årsflöde > Utkast> 
```

Saknar året utkast står det `Utkast 2026/27: inga utkast.`

Varje rad visar utkastets nummer, datum, belopp och text, och sist hur långt utkastet har kommit inom parentes (se tabellen i [Räkenskapsår](#räkenskapsår)). Har utkastet ingen rubrik, som ett utkast från ett kontoutdrag, visar raden kontot och konteringens text i stället. Ett utkast med stadiet `samma` visar delarnas texter, rubriken först, med ` : ` emellan och utan kontonummer, på högst 40 tecken. En del som är kortare än sin andel står kvar hel, och de längre delarna delar lika på resten och slutar med `..`. Till exempel `Anthropic Claude : Kortköp 260910 ANTH.. (samma)`, eller för en överföring `Utbetalning : Skatt | 5050-1030 SK5567.. (samma)`. I bästa fall är det redan texten för verifikationen. cratchit räknar fram den av delarna varje gång och sparar den inte; utkastets vy visar rubriken och konteringarnas fullständiga texter var för sig. Beloppet är händelsens, med tecken (se [Beskriv händelser](#beskriv-händelser)). Saknar händelsen belopp och utkastet har en enda kontering, som ett utkast från ett kontoutdrag, är det konteringens belopp med tecken: minus är ut från kontot. Har utkastet flera konteringar är det vad de flyttar, utan tecken.

Numret, till exempel `#3`, är utkastets plats bland årets utkast, räknat från det nyaste: det översta i listan är `#1` och det äldsta har det högsta numret. Det är inget fast id: numren ändras när ett utkast med senare datum tillkommer eller när ett nyare utkast försvinner ur listan. Bokför du det äldsta utkastet behåller de andra sina nummer.

##### Utkastet

<!-- hjälp: utkastet -->
Utkastet i samma delar som en verifikation. Överst numret, räkenskapsåret, hur långt utkastet har kommit och när det noterades. Sedan *Händelse*: datum, rubrik och belopp; det utkastet saknar står tomt. Sedan *Konteringar*. Är utkastet inte klart står sist, under `Innan utkastet kan bokföras:`, vad som saknas och var du ändrar det, till exempel `Rubrik saknas: tryck 0 och sedan 1.` Det kan också vara belopp, konteringar, att konteringarna inte balanserar, eller att debet inte är beloppet. Tryck `0` för att ändra händelsen i [Ändra händelsen](#ändra-händelsen) och `1` för att ändra konteringarna i [Kontera](#kontera). Är utkastet klart finns också `2` för att [bokföra](#bokför) det. Tryck `3` för att fullborda det med en [mall](#använd-mall). `9` är *Glöm* för ett utkast utan kontohändelser och för ett ensamt utkast. Ett utkast med kontohändelser och flera delar med samma belopp har i stället *Inte samma*, som delar det direkt, och ett med kontohändelser och olika belopp har inget `9`, så att raderna från kontoutdraget finns kvar. När du kommer tillbaka visar cratchit utkastet som det är nu. Tryck `-` för att gå tillbaka till Utkast.
<!-- /hjälp -->

Skriv utkastets nummer i Utkast, till exempel `#12`, och tryck Enter:

```text
Utkast #12 i 2026/27 (kontohändelse)
  Noterad:        2026-11-17
Händelse:
  Datum:          2026-07-03
  Rubrik:
  Belopp:
Konteringar:
  1920  PlusGiro        -3.70  AVGIFTER NORDEA
  0 Händelse
  1 Konteringar
  9 Glöm
Cratchit > … > Utkast > #12> 
```

Skriver du något annat än `#` och ett nummer säger cratchit `Skriv utkastets id som #13.`, och finns inget utkast med numret `Det finns inget utkast #20 i 2026/27.` Raden står kvar så att du kan rätta den.

Ett utkast ändrar du på ett enda sätt, i [Kontera](#kontera), oavsett om du öppnar det här eller i [Fullborda utkast](#fullborda-utkast). Har du ändrat datumet kan utkastet ha fått ett nytt nummer, som syns överst.

###### Glöm

<!-- hjälp: glom-utkast -->
Utkastet tas bort ur bokföringen om du trycker `9` en gång till, och går då inte att få tillbaka. Tryck `N` eller `-` för att ångra och gå tillbaka till utkastet.
<!-- /hjälp -->

Tryck `9` i utkastet för att glömma det, till exempel ett utkast du skapade för att prova. `9` står för sig, skilt från de andra valen, så att det inte trycks av misstag, och cratchit frågar först:

```text
Glöm utkast #14?
  2026-09-28       1308.75  Loopia webhotell (händelse)
Utkastet tas bort ur bokföringen och går inte att få tillbaka.
  9 Glöm
  N Nej
Cratchit > … > Utkast > #14 > Glöm> 
```

Tryck `9` igen för att glömma utkastet. cratchit går då tillbaka till Utkast och skriver under listan till exempel `cratchit glömde utkast #14: Loopia webhotell (händelse).` De utkast som är äldre än det glömda får nya nummer. Loggen får det glömda utkastet med alla dess konteringar.

Glömmer du ett utkast från ett kontoutdrag och läser kontoutdraget igen blir raden ett nytt utkast, eftersom den varken är bokförd eller ett utkast längre.

Vilka utkast som har `9 Glöm`:

* **Ett utkast utan kontohändelser**, alltså utan konteringar från ett kontoutdrag, oavsett hur många delar det har. Inget går förlorat som du inte kan skapa igen: en [ändringsverifikation](#ändringsverifikation) du skapade för att prova skapar du igen från verifikationen, och det du skrivit skriver du igen.
* **Ett ensamt utkast**: en händelse eller en kontohändelse.

Ett utkast med kontohändelser och flera delar glöms inte direkt, så att raderna från kontoutdraget inte försvinner med det. Har delarna samma belopp trycker du `9 Inte samma` och glömmer sedan delarna var för sig. Annars tar du först bort konteringarna du skrivit med `9 Ta bort sista` i [Kontera](#kontera) tills delarna har samma belopp.

###### Inte samma

Ett utkast kan bestå av flera delar: en händelse och konteringar, eller flera konteringar. Har alla delarna samma belopp, utan tecken, är det samma händelse sett från olika håll, till exempel en faktura och betalningen på kontoutdraget, eller en överföring med en rad på vart kontoutdrag. Ett sådant utkast kommer oftast från [Samma?](#samma). Har det kontohändelser har `9` texten *Inte samma*:

```text
Utkast #14 i 2026/27 (samma)
  …
  0 Händelse
  1 Konteringar
  9 Inte samma
Cratchit > … > Utkast > #14>
```

`9` delar utkastet direkt i en händelse och en kontohändelse för varje kontering, och du kommer tillbaka till Utkast, som säger till exempel `cratchit delade #14 i 2 utkast.` Varje kontohändelse får konteringens datum och minns sin fil och rad, och alla får utkastets noteringsdag. cratchit frågar inte först, eftersom du kan slå ihop dem igen i Samma?.

Ett sådant utkast, med kontohändelser, glöms inte direkt; dela det först och glöm sedan delarna. Ett utkast utan kontohändelser har i stället `9 Glöm` (se [Glöm](#glöm)). Har konteringarna olika belopp, som i en färdig kontering av en faktura med moms, är utkastet mer än samma händelse och har inget `9`.

##### Fullborda utkast

<!-- hjälp: fullborda -->
Utkasten ett i taget, det äldsta först, som när du öppnar ett utkast. Det äldsta har det högsta numret, så numret räknar ner när du går framåt. Tryck `>` för nästa och `<` för föregående; vid första och sista gör de ingenting. Valen är desamma som när du öppnar ett utkast: `0` ändrar händelsen, `1` konteringarna i [Kontera](#kontera), `2` [bokför](#bokför) utkastet när det är klart, `3` [använder en mall](#använd-mall) och leder dig sedan till det som saknas och till Bokför, och `9` glömmer eller delar det. Som i utkastets vy står sist vad som saknas för att bokföra. Vyn visar bara utkastet och dess val; vad du har gjort står i Utkast när du går tillbaka dit. Tryck `-` för att gå tillbaka till Utkast.
<!-- /hjälp -->

Att fullborda ett utkast är att göra det klart att bokföra: välja en mall, fylla i det som saknas och få konteringarna att balansera. Att bokföra är sista steget. Tryck `2` i Utkast:

```text
Utkast #18 av 18 i 2026/27 (kontohändelse)
  Noterad:        2026-11-17
Händelse:
  Datum:          2026-07-03
  Rubrik:
  Belopp:
Konteringar:
  1920  PlusGiro        -3.70  AVGIFTER NORDEA
  0 Händelse
  1 Konteringar
  3 Använd mall
  9 Glöm
  < Föregående
  > Nästa
Cratchit > … > Utkast > Fullborda #18>
```

Verifikationer bokförs i den ordning händelserna inträffade, så cratchit börjar med det äldsta utkastet. Prompten har utkastets nummer, och överst står hur många utkast det finns. Valen är desamma som i [utkastets vy](#utkastet), så att ett utkast alltid går att ändra var du än ser det, och att bokföra när det är klart. `2 Bokför` finns bara för ett klart utkast och leder till [Bokför](#bokför). Glömmer eller delar du utkastet står du kvar, och cratchit läser utkasten igen. Saknar året utkast står det `Inga utkast att fullborda i 2026/27.`

###### Kontera

<!-- hjälp: kontera -->
Skriv en kontering, konto och belopp, och tryck Enter, till exempel `6570 3,70`. Debet är positivt och kredit negativt. Skriver du bara kontot får konteringen det som återstår för att balansera. Du kan också skriva ord ur kontots namn, till exempel `bankkost 3,70`: passar exakt ett konto visar cratchit raden igen med numret, `6570 3,70`, så att du kan kontrollera beloppet och trycka Enter en gång till. Tab fyller i ett kontonummer, eller visar vilka konton namnet passar. Under utkastet står vad som återstår. Tryck `9` för att ta bort den sista konteringen du skrev. Händelsen, rubrik, belopp och datum, ändrar du inte här utan i Händelse: tryck `-` och sedan `0`. Tryck `-` för att gå tillbaka.
<!-- /hjälp -->

Här bygger du utkastet tills det balanserar, och här ändrar du det. Du kommer hit med `1 Konteringar` i Fullborda eller i utkastets vy. Till exempel bankavgiften från kontoutdraget:

```text
Utkast #18 i 2026/27 (kontohändelse)
  …
Konteringar:
  1920  PlusGiro        -3.70  AVGIFTER NORDEA

Kvar: debet 3.70.
Rubrik och belopp ändrar du i Händelse: tryck - och sedan 0.
Cratchit > … > Utkast > Fullborda #18 > Kontera: 6570
```

Varje kontering läggs till direkt, och du står kvar för nästa. Raderna under utkastet säger var konteringarna står:

* `Kvar: debet 3.70.` eller `Kvar: kredit 3.70.`: det som fattas för att debet och kredit ska bli lika. Skriver du bara kontot får konteringen just det.
* `Konteringarna balanserar.`
* `Debet 0.00, beloppet 1118.00.`: debet stämmer inte med händelsens belopp.
* `Utkastet är klart att bokföra: tryck - och sedan 2.` Ett val för att bokföra direkt i Kontera skulle krocka med kontonumren, så du bokför från Fullborda eller utkastets vy, ett `-` bort.

Det fungerar bäst om du först konterar hela beloppet, till exempel `2440 -1118` för en leverantörsfaktura, och sedan motkontona med det största beloppet först. Den sista raden, till exempel öresavrundningen, kan du då skriva med bara kontot.

Ett konto som inte finns i kontoplanen går inte att använda.

Du behöver inte kunna kontonumret. Skriv ord ur kontots namn i stället, ett eller flera, och eventuellt beloppet efter:

```text
… > Kontera: bankkost
6570  Bankkostnader passar. Kontrollera beloppet och tryck Enter.
… > Kontera: 6570 3.70
```

* Ett konto passar när dess namn innehåller alla orden, i vilken ordning som helst. Stora och små bokstäver räknas lika.
* Passar exakt ett konto skriver cratchit raden igen med kontots nummer och beloppet: det du skrev, eller, om du inte skrev något, det som återstår. Ändra beloppet om det behövs och tryck Enter. Balanserar konteringarna redan står bara numret där, och du skriver beloppet.
* Passar flera konton visar cratchit dem (högst tio), och raden står kvar så att du kan skriva fler ord eller numret, till exempel `debiterad moms` i stället för `moms`. Passar inget säger cratchit det.
* Tab under tiden du skriver visar vilka konton orden passar, och när det är ett enda att Enter väljer det.
* Beloppet är de sista orden som kan läsas som ett belopp, till exempel `1 118,00`.

Så kommer du också åt ett konto vars nummer börjar på `9`, som är ett val i Kontera: skriv namnet i stället.

En kontering från ett kontoutdrag står sist med fil och rad, till exempel `1920 från bank/nordea.csv rad 8`.

`9 Ta bort sista` tar bort den sista konteringen du skrev. En kontering från ett kontoutdrag tas inte bort här; den delar du ut med [Inte samma](#inte-samma).

###### Ändra händelsen

<!-- hjälp: kontera-handelse -->
Utkastets händelse: datum, rubrik och belopp, som för en verifikation, och serien det ska bokföras i. Tryck `0`, `1`, `2` eller `3` för att ändra den delen. Raden står ifylld; ändra den och tryck Enter för att spara och komma tillbaka hit. Tryck `-` för att gå tillbaka utan att ändra.
<!-- /hjälp -->

Ett utkast behöver en rubrik och ett belopp för att bli klart. Tryck `0` i utkastets vy eller i Fullborda. Kontera har inget val för händelsen, eftersom där arbetar du bara med konteringarna:

```text
Händelsen i #18
  0 Datum: 2026-07-03
  1 Rubrik:
  2 Belopp:
  3 Serie:
Cratchit > … > Utkast > Fullborda #18 > Händelse>
```

Det utkastet saknar står tomt. Varje val visar en rad som är ifylld med det utkastet redan har:

* **Datum**: datumet, skrivet `2026-10-22` eller `20261022`. Det måste ligga i räkenskapsåret. Ändrar du datumet kan utkastet få ett nytt nummer.
* **Rubrik**: rubriken, eller, för ett utkast med stadiet `samma` utan rubrik, delarnas text (se [Utkast](#utkast)). En tom rubrik går inte.
* **Belopp**: beloppet, eller, när händelsen saknar belopp, vad delarna har för belopp. Ett minustecken betyder att händelsen går åt andra hållet än vanligt. Noll går inte.
* **Serie**: serien utkastet ska bokföras i, till exempel `D`, som [Bokför](#bokför) börjar med. En mall ger sin serie. En tom rad tar bort serien.

Ändra raden och tryck Enter. cratchit sparar ändringen direkt, och du är tillbaka i Händelse, där valen visar det nya. Går raden inte att använda säger cratchit varför, och raden står kvar så att du kan rätta den.

###### Använd mall

<!-- hjälp: anvand-mall -->
Tidigare verifikationer som liknar utkastet, de bästa först, var och en med sina konteringar. Skriv mallens nummer och tryck Enter för att använda den: utkastet får mallens konteringar, skalade till utkastets belopp, och mallens serie. Sedan leder cratchit dig till det som saknas, en sak i taget, och sist till Bokför. Tryck `-` för att gå tillbaka utan att använda någon.
<!-- /hjälp -->

En mall är en tidigare verifikation som du utgår från. Tryck `3` i utkastets vy eller i Fullborda:

```text
Utkastet:
  #18           2026-07-03         -3.70  1920  AVGIFTER NORDEA (kontohändelse)
Tidigare verifikationer med samma konton och samma tecken som utkastets konteringar:
  1  A2            2026-05-06          1.85  AVGIFTER NORDEA
       1920  PlusGiro        -1.85
       6570  Bankkostnader    1.85
Skriv mallens nummer och tryck Enter.
Cratchit > … > Fullborda #18 > Mall>
```

Vilka verifikationer som visas:

* **Utkastet har konteringar**, till exempel en kontohändelse: verifikationer som har en kontering på vart och ett av utkastets konton, med samma tecken. Bankavgiften på 1920 i kredit hittar alltså tidigare verifikationer med 1920 i kredit.
* **Utkastet har bara en händelse**: verifikationer vars text delar ord med rubriken.
* **Ett utkast med minus** (åt andra hållet än vanligt, som en kreditfaktura) jämförs med tecknen vända, och mallen används vänd. En vanlig faktura blir då mall för kreditfakturan.

Bara verifikationer med ett belopp och minst två konteringar räknas, från alla år i bokföringen. Först står de vars text delar flest ord med utkastets, sedan de senaste. Har flera samma konton på samma sidor visas bara den senaste av dem, och högst nio.

Skriv numret och tryck Enter. Mallen syns i sin helhet innan du väljer, så cratchit frågar inte en gång till. Utkastet får

* **mallens konteringar** på de konton utkastet inte redan har: utkastets egna konteringar, som kontohändelsen, står kvar. Varje rad skalas till utkastets belopp och avrundas till hela ören; den sista raden får det som återstår, så att konteringarna balanserar. Ett konto som inte finns i kontoplanen kommer inte med, och cratchit säger vilket;
* **beloppet**: utkastets eget, eller, när händelsen saknar belopp, vad delarna har för belopp;
* **mallens serie**, som [Bokför](#bokför) börjar med. Du ändrar den i [Händelse](#ändra-händelsen).

Rubriken tas inte från mallen; den gäller en annan händelse.

cratchit säger till exempel `cratchit använde A2 som mall: 1 rad tillagd, serie A.` och leder dig sedan vidare till det som saknas för att bokföra, en sak i taget och i den här ordningen:

1. **Rubrik**, om utkastet saknar en. Delarnas texter, eller kontohändelsens text, står ifyllda; skriv om och tryck Enter.
2. **Belopp**, om utkastet saknar ett. Det konteringarna flyttar står ifyllt.
3. **[Kontera](#kontera)**, om konteringarna inte balanserar eller debet inte är beloppet. Så fort utkastet är klart går du vidare.
4. **[Bokför](#bokför)**, med mallens serie ifylld. Enter bokför.

Det som redan finns hoppar cratchit över, så ett utkast som blir klart av mallen går direkt till Bokför:

```text
Cratchit > … > Fullborda #18 > Mall> 1
cratchit använde A2 som mall: 1 rad tillagd, serie A.
Skriv rubriken och tryck Enter.
Cratchit > … > Fullborda #18 > Rubrik: AVGIFTER NORDEA
…
Cratchit > … > Fullborda #18 > Bokför: A
```

Varje steg sparas när du trycker Enter. Tryck `-` i vilket steg som helst för att lämna kedjan; du är då tillbaka vid utkastet, som visar vad som återstår (se [Utkastet](#utkastet)). Efter Bokför är du i Fullborda vid nästa utkast, eller i Utkast om du började i utkastets vy. Det mallen lade till tar du bort med `9 Ta bort sista` i [Kontera](#kontera).

###### Bokför

<!-- hjälp: bokfor-utkast -->
Verifikationen som utkastet blir, i sin helhet, och bokföringens serier med nästa nummer i varje. Skriv serien, till exempel `D`, och tryck Enter för att bokföra. Verifikationen går inte att ändra efteråt. Tryck `-` för att gå tillbaka till Fullborda utan att bokföra.
<!-- /hjälp -->

Tryck `2 Bokför` i Fullborda eller i utkastets vy för ett utkast som är klart:

```text
Bokför utkast #18 som verifikation i 2026/27:
  Datum:          2026-07-03
  Text:           AVGIFTER NORDEA
  Belopp:         3.70
  Registreras:    2026-11-18
Konteringar:
  1920  PlusGiro              -3.70  AVGIFTER NORDEA
  6570  Bankkostnader          3.70
Serier:
  A  Redovisning (nästa A11)
  B  Kundfakturor (nästa B1)
  …
Skriv serien och tryck Enter för att bokföra. Verifikationen går inte att ändra efteråt.
Cratchit > … > Fullborda #18 > Bokför>
```

Du väljer alltid serien själv; cratchit gissar inte. Har utkastet en serie, från en [mall](#använd-mall) eller från Händelse, står den ifylld, och du trycker Enter eller skriver en annan. Stora och små bokstäver räknas lika. Numret är det högsta i serien under räkenskapsåret plus ett, så numreringen börjar om varje år. Serien och Enter är bekräftelsen: verifikationen står där i sin helhet innan du bokför, och cratchit frågar inte en gång till.

Verifikationen får

* **datum, text och belopp** från utkastets händelse; beloppet utan tecken, som alla verifikationer;
* **konteringarna** med sina datum och texter, men inte vilken kontoutdragsrad de kom från;
* **registreringsdagen** i dag, och ingen signatur än.

cratchit sparar verifikationen i räkenskapsårets `vouchers.csv` och `postings.csv`, tar bort utkastet och skriver det i loggen. I terminalen skriver cratchit en gång till exempel `cratchit bokförde #18 som A11: AVGIFTER NORDEA.` Bokförde du från Fullborda är du tillbaka där på samma plats, där nu nästa utkast står, här `#17`. Bokförde du från utkastets vy är du tillbaka i Utkast, eftersom utkastet inte finns längre. Utkast säger detsamma, och verifikationen finns i [Bokfört](#bokfört). För att få in den i Fortnox exporterar du skillnaden till en SIE-fil (se [Exportera till SIE](#exportera-till-sie)).

Den sista verifikationen i en serie kan du göra till ett utkast igen, se [Gör till utkast](#gör-till-utkast). Ett stängt räkenskapsår går inte att bokföra i, och dess verifikationer går inte att göra till utkast; cratchit säger då att året är stängt.

##### Läs kontoutdrag

<!-- hjälp: las-kontoutdrag -->
Skriv sökvägen till kontoutdraget och tryck Enter. Tab fyller i mappar och filer. cratchit begränsar inte vilka filer du kan välja. Väljer du en mapp, en fil som inte finns eller en fil som cratchit inte kan läsa som kontoutdrag, säger cratchit varför och sökvägen står kvar. Tryck `-` för att gå tillbaka till Utkast.
<!-- /hjälp -->

Ett kontoutdrag är en fil från banken eller från Skatteverket med raderna för ett konto, till exempel Nordeas CSV-fil eller Skatteverkets *Bokförda transaktioner*. Varje rad i kontoutdraget blir ett utkast med en enda kontering på det BAS-konto du anger. Utkastet har ingen rubrik än; det fyller du i senare.

Tryck `3` i Utkast. Sökvägen börjar i bokföringens mapp, och du tar dig fram med Tab som när du väljer en SIE-fil. Alla filer räknas; det är du som väljer rätt fil. När du går tillbaka, och nästa gång du läser ett kontoutdrag eller pdf-underlag, börjar sökvägen i mappen du valde senast. Kontoutdrag och underlag för en period ligger ofta i samma mapp.

```text
Skriv sökvägen till kontoutdraget och tryck Enter. Tab fyller i mappar och filer.
Cratchit > … > Årsflöde > Utkast > Läs kontoutdrag> /Users/me/Documents/The ITfied AB/
```

cratchit läser filen och låter den ligga kvar. Den kopieras inte, och cratchit ändrar den inte.

**Så läser cratchit ett kontoutdrag.** cratchit hittar raderna genom vad cellerna innehåller, inte genom kolumnernas namn:

* Teckenkodningen: UTF-8, med eller utan BOM i början, annars ASCII. Har filen tecken som varken är UTF-8 eller ASCII säger cratchit vilken rad det gäller; spara då filen som UTF-8.
* Cellerna skiljs åt med `;`, tabb eller `,`, det som ger flest rader med lika många celler. En cell får stå inom citattecken.
* En rad i kontoutdraget har exakt ett datum (`2026-09-28` eller `2026/09/28`, ensamt i sin cell) och minst ett belopp (`-1308,75`, `2 292`, ensamt i sin cell).
* Kontoutdragets rader är den längsta följden av sådana rader i rad, med lika många celler och datumet på samma plats. Rader före och efter, som en rubrik eller *Ingående saldo*, hör inte till.
* Finns det en kolumn med saldo måste den stämma rad för rad: saldot före plus beloppen sedan dess. En rad utan saldo går bra. Saldot visar också om filen har den nyaste eller den äldsta raden först; utan saldo avgör datumen.
* Radens text är dess andra celler, åtskilda med ` | `, utan tomma celler och utan celler som är lika på alla rader (som `SEK`).

När cratchit inte kan avgöra något, till exempel när inget saldo stämmer, säger cratchit varför och läser inte in något.

##### Konto

<!-- hjälp: kontoutdrag-konto -->
Ovanför prompten står vad kontoutdraget innehåller. Skriv BAS-kontot som kontoutdraget gäller, till exempel `1930`, och tryck Enter. Kontot måste finnas i bokföringens kontoplan. Har cratchit ett förslag står det redan på prompten. Tryck `-` för att välja en annan fil.
<!-- /hjälp -->

```text
Kontoutdrag: /Users/me/Documents/The ITfied AB/PLUSGIROKONTO FTG 51 86 87-9 - 2026-10-20 11.05.39.csv
  Kodning   UTF-8 med BOM
  Rader     7, 2026-07-03 – 2026-09-28 (rad 2–8, nyast först)
  Saldo     6644.68 före, 6183.45 efter, stämmer på varje rad som har saldo
  Rad 1 är ingen rad i kontoutdraget.
Förslag: 1920 PlusGiro (1 av 7 rader stämmer med konteringar på 1920 i bokföringen).
Skriv BAS-kontot som kontoutdraget gäller och tryck Enter.
Cratchit > … > Läs kontoutdrag > Konto> 1920
```

Förslaget är det konto som har flest konteringar i bokföringens verifikationer med samma dag och belopp som en rad i kontoutdraget. Har två konton lika många, eller inget konto någon, föreslår cratchit inget.

##### Bekräfta kontoutdrag

<!-- hjälp: bekrafta-kontoutdrag -->
Ovanför prompten står vad kontoutdraget ger: nya utkast, rader som redan är bokförda eller redan är utkast, och rader utanför räkenskapsåret. Tryck `J` för att skapa utkasten, eller `N` för att välja en annan fil. Finns inget nytt är det enda valet `N Tillbaka`.
<!-- /hjälp -->

```text
Kontoutdrag: /Users/me/Documents/The ITfied AB/PLUSGIROKONTO FTG 51 86 87-9 - 2026-10-20 11.05.39.csv
  Konto           1920 PlusGiro
  Nya utkast      6
  Redan bokförda  1
  Redan utkast    0
  Nya:
    2026-08-26       -923.00  5020-7042 | Fortnox Finans AB | 574512366662450 | Fortnox sep-nov
    …
Skapa 6 utkast?
  J Ja
  N Nej
```

* En rad är *redan bokförd* när en verifikation i räkenskapsåret har en kontering på kontot med samma dag och belopp.
* En rad är *redan utkast* när ett utkast i räkenskapsåret har en sådan kontering.
* Varje kontering räknas bara för en rad. Två lika rader behöver alltså två konteringar.
* Rader utanför räkenskapsåret läses inte in.

Du kan alltså läsa samma fil flera gånger, och filer som överlappar varandra går bra: bara det nya blir utkast.

`J` skapar utkasten och går tillbaka till Utkast, som säger hur många det blev. Varje utkast har radens datum, och en kontering på kontot med radens belopp (in på kontot är debet), datum och text. Konteringen minns vilken fil och rad den kom från. Filen förs in i räkenskapsårets förteckning (se [Dina filer](#dina-filer)).

##### Beskriv händelser

<!-- hjälp: beskriv-handelse -->
Skriv händelsen på en rad: rubrik, belopp och datum, i vilken ordning som helst, till exempel `Telia faktura oktober 1118,00 2026-10-22`, och tryck Enter. Datumet måste finnas och ligga i räkenskapsåret. Ett minustecken betyder att händelsen går åt andra hållet än vanligt, till exempel en kreditfaktura eller en återbetalning. cratchit skapar utkastet och du står kvar, med en tom rad för nästa händelse; det du skapat står under *Skapade nu*. Är något fel säger cratchit vad, och raden står kvar så att du kan rätta den. Tryck `-` på en tom rad för att gå tillbaka till Utkast.
<!-- /hjälp -->

En händelse är det som hände, när och för hur mycket, till exempel en faktura eller ett kvitto. Tryck `0` i Utkast och skriv den på en rad. Du kan skriva den ena händelsen efter den andra utan att lämna platsen:

```text
Skriv händelsen: rubrik, belopp och datum, i vilken ordning som helst, och tryck Enter.
Till exempel: Telia faktura oktober 1118,00 2026-10-22
I dag är 2026-11-17. Räkenskapsåret är 2026-05-01 – 2027-04-30.
Cratchit > … > Utkast > Beskriv händelser: Telia faktura oktober 1118,00 2026-10-22
```

Så läser cratchit raden:

* **Datumet** är det sista ordet som är ett datum, skrivet `2026-10-22` eller `20261022`. Det måste finnas; cratchit tar inte dagens datum av sig själv.
* **Beloppet** är det sista andra talet: siffror, och decimalkomma eller decimalpunkt med högst två decimaler (`1118`, `1118,5`, `1118.00`). Ett hårt mellanslag inne i talet, som en bank kan skriva, räknas inte.
* **Minustecken** (`-1245,00`) betyder att händelsen går åt andra hållet än vanligt: en kreditfaktura, en återbetalning eller en kreditering. Det betyder inte "pengar ut"; en vanlig leverantörsfaktura är positiv. Utkastet behåller tecknet, och listan visar det. När utkastet bokförs får verifikationen beloppet utan tecken, som alla verifikationer; tecknet säger då att konteringarna går åt andra hållet än för en vanlig sådan händelse.
* **Rubriken** är resten, i den ordning du skrev den. Tal i rubriken står kvar så länge de inte står sist: i `Faktura 87 Payex 500 2026-10-22` är rubriken `Faktura 87 Payex`.

Raden börjar som all annan text: med en tangent som inte är ett val eller en gemensam tangent. Börjar rubriken med litet `q` leder det till [Avsluta](#avsluta); tryck `N` och skriv `Q` i stället, eller börja med något annat ord.

###### Belopp?

<!-- hjälp: belopp-otydligt -->
Beloppet kan läsas på flera sätt, eftersom ett mellanslag kan skilja tusental. Tryck siffran för det du menar, så skapar cratchit utkastet. Tryck `-` för att gå tillbaka och ändra raden.
<!-- /hjälp -->

När tal står före beloppet kan ett mellanslag skilja tusental (`1 118,00`) eller ett tal i rubriken från beloppet (`Faktura 87 500`). cratchit gissar inte utan visar varje läsning:

```text
Beloppet är otydligt: mellanslag kan skilja tusental. Vilket menar du?
  1       118.00  Telia faktura oktober 1
  2      1118.00  Telia faktura oktober
Cratchit > … > Beskriv händelser > Belopp?> 
```

Ett ord räknas bara som en grupp tusental när det har exakt tre siffror och inget tecken, och ordet före har högst tre siffror och inga decimaler. Så `Faktura 8700 500` och `Faktura 87 50,00` är tydliga. Skriv beloppet utan mellanslag så slipper du frågan.

När cratchit har läst raden skapas utkastet direkt, utan att du behöver bekräfta. Raden töms och du står kvar för nästa händelse. Det du har skapat sedan du kom hit står under *Skapade nu*, med datum, belopp och rubrik (utan nummer, eftersom numren ändras när utkast tillkommer):

```text
Skriv händelsen: rubrik, belopp och datum, i vilken ordning som helst, och tryck Enter.
Till exempel: Telia faktura oktober 1118,00 2026-10-22
I dag är 2026-11-17. Räkenskapsåret är 2026-05-01 – 2027-04-30.

Skapade nu:
  2026-07-15        815.00  Fortnox bokföring
  2026-08-29       1308.75  Loopia webhotell
Cratchit > … > Utkast > Beskriv händelser> 
```

Valde du ett belopp i *Belopp?* kommer du också tillbaka hit. cratchit föreslår inget datum: skriv det på varje rad. Tryck `-` på en tom rad för att gå tillbaka till Utkast. Där står de nya utkasten i listan, med stadiet `händelse` eftersom de inte har några konteringar än, och under listan till exempel `cratchit skapade 2 utkast.`

##### Samma?

<!-- hjälp: samma -->
Utkast som kan vara samma händelse: två utkast som kompletterar varandra, eller ett utkast och en verifikation som redan är bokförd, med samma belopp (utan tecken) och högst 4 dagar emellan. De troligaste står först. Skriv parets nummer och tryck Enter för att se det i sin helhet. Det är du som avgör om det är samma händelse. Tryck `>` för nästa sida och `<` för föregående. Tryck `-` för att gå tillbaka till Utkast.
<!-- /hjälp -->

Samma händelse kan bli två utkast: kvittot du beskrev och kortköpet på kontoutdraget, eller en överföring mellan två egna konton, en rad på vart kontoutdrag. Och ett utkast kan redan vara bokfört, till exempel när en SIE-import senare tog in verifikationen. Tryck `1` i Utkast:

```text
Möjliga par i 2026/27 (samma belopp, högst 4 dagar emellan):
  1  utkast   #13           2026-09-26       1308.75  Loopia webhotell (händelse)
              #12           2026-09-28      -1308.75  1920  5343-2795 | LOOPIA AB (WEBSUPPORT) | … (kontohändelse)
              2 dagar emellan, delar orden: loopia, webhotell
  2  bokfört  #22           2026-09-01       -311.25  1920  5343-2795 | LOOPIA AB (WEBSUPPORT) | … (kontohändelse)
              D6            2026-08-31        311.25  Loopia Faktura
              1 dag emellan, delar orden: loopia
  < Föregående sida
  > Nästa sida
Cratchit > … > Utkast > Samma?>
```

Varje par har tre rader: det äldre utkastet, det andra utkastet eller verifikationen, och vad som talar för paret. *utkast* är två utkast och *bokfört* ett utkast och en verifikation. Ett utkast kan finnas i flera par. Hittar cratchit inga står det `Inga möjliga par i 2026/27 (samma belopp, högst 4 dagar emellan).`

Två utkast är ett par bara när de kompletterar varandra:

* **Högst det ena har en händelse.** Två händelser med samma belopp slås aldrig ihop, eftersom den ena då skulle försvinna. Är det samma faktura beskriven två gånger glömmer du den ena i Utkast.
* **Har båda konteringar måste de balansera tillsammans:** ut från ett konto och in på ett annat, som en överföring. Två kontohändelser åt samma håll, eller en in och en ut på samma konto (som momsen och utbetalningen på skattekontot), är två händelser.
* **Är det ena bara en händelse** har det andra konteringarna, till exempel betalningen på kontoutdraget. Tecknen jämförs inte: händelsens tecken säger om den går åt det vanliga hållet, kontohändelsens om pengarna går in eller ut. En leverantörsfaktura har plus och betalningen minus, en kundbetalning plus och plus.

Belopp och datum gör bara paret möjligt. Med fakturametoden har en faktura och dess betalning samma belopp inom några dagar, men de är två händelser. Därför avgör du. Först står paren vars texter delar flest ord (stora och små bokstäver räknas lika; tal och ord på en bokstav räknas inte), sedan de med minst dagar emellan. Ett utkast utan rubrik jämförs med konteringarnas texter. För ett utkast och en verifikation står först de som *passar*: verifikationen har alla utkastets konteringar (samma konto, datum och belopp).

Fyra dagar är en helg och en bankdag på var sida; helgdagar räknas inte. Ett par du lämnar kommer cratchit inte ihåg: det står med nästa gång.

###### Paret

<!-- hjälp: samma-par -->
Paret i sin helhet och vad som talar för det. Två utkast: tryck `0`, *Samma!*, för att slå ihop dem till ett. Ett utkast och en verifikation: tryck `9` för att glömma utkastet, eftersom det redan är bokfört, och `9` igen för att bekräfta. Tryck `-` för att gå tillbaka till Samma?.
<!-- /hjälp -->

```text
Samma händelse? 2 dagar emellan, delar orden: loopia, webhotell.
Utkast #13 (händelse), noterat 2026-11-17
  Datum:          2026-09-26
  Rubrik:         Loopia webhotell
  Belopp:         1308.75
  Konteringar:    inga
Utkast #12 (kontohändelse), noterat 2026-11-17
  Datum:          2026-09-28
  Rubrik:         
  Belopp:         
  Konteringar:
    1920  Plusgiro     -1308.75  5343-2795 | LOOPIA AB (WEBSUPPORT) | 62600010249540 | Webhotell 12/10 - 11/1
Det ena är en händelse och det andra en kontohändelse.
  0 Samma!
Cratchit > … > Utkast > Samma? > 1>
```

När båda utkasten har konteringar säger raden sist att de balanserar tillsammans: ut från ett konto och in på ett annat.

`0`, *Samma!*, slår ihop utkasten direkt. Paret du ser i sin helhet är bekräftelsen; cratchit frågar inte en gång till. Det ena utkastet får

* **händelsen** från det av dem som har en (högst det ena har det);
* **konteringarna** från båda, men en kontering från samma kontoutdragsrad bara en gång;
* **noteringsdagen** som är tidigast.

Utkastet går att dela igen med `9`, *Inte samma*, när du öppnar det från Utkast (se [Inte samma](#inte-samma)).

För ett utkast och en verifikation visar cratchit utkastets konteringar som inte finns i verifikationen. De glöms med utkastet. `9` leder till [Glöm](#glöm), som för ett utkast du öppnat. Verifikationen ändras inte.

Efteråt är du tillbaka i Samma?. cratchit söker paren igen, och under listan står vad du gjorde, till exempel `cratchit slog ihop #13 och #12 till #12: Loopia webhotell : 5343-2795 | LOOPIA.. (samma).` Numren kan ha ändrats, eftersom ett utkast har försvunnit. Utkast säger samma sak när du går tillbaka dit.

##### Läs pdf-underlag

<!-- hjälp: las-pdf-underlag -->
Skriv sökvägen till mappen med pdf-underlagen och tryck Enter. Tab fyller i mappar och visar vilka pdf-filer mappen har. Väljer du en fil, eller en mapp som inte finns, säger cratchit det och sökvägen står kvar. En mapp leder till *Pdf-underlag*, en lista över mappens pdf-filer. Tryck `-` för att gå tillbaka till Utkast.
<!-- /hjälp -->

Underlag är dokumenten som visar en affärshändelse, till exempel fakturor och kvitton som pdf. Tryck `4` i Utkast och välj mappen där de ligger. Sökvägen börjar i bokföringens mapp, eller i mappen du valde senast för kontoutdrag eller underlag.

```text
Skriv sökvägen till mappen med pdf-underlagen och tryck Enter. Tab fyller i mappar och visar mappens pdf-filer.
Cratchit > … > Utkast > Läs pdf-underlag: /Users/me/Documents/The ITfied AB/M2 Juli-Sep/
      4 pdf-filer i mappen:
        62600010249540.pdf
        Faktura50555657.pdf
        M2.pdf
        momsrapport 2026-07-01 - 2026-09-30.pdf
```

Tab fyller bara i mappar. Pdf-filerna visas som information; du väljer mappen, inte filerna. cratchit läser filerna och låter dem ligga.

###### Pdf-underlag

<!-- hjälp: pdf-underlag -->
Mappens pdf-filer, numrerade, med vad cratchit hittar i varje fil: text eller inte. Listan visar 20 filer i taget; tryck `>` för nästa sida och `<` för föregående. Tryck `0` för att gå igenom filerna en i taget och tolka dem som händelser. Tryck `-` för att gå tillbaka och välja en annan mapp.
<!-- /hjälp -->

När du trycker Enter på en mapp läser cratchit varje pdf-fil i den (inte i undermappar). Medan det pågår står det på en rad under sökvägen vilken fil cratchit läser, till exempel `Läser pdf-filer: 3 av 9 (62600010365346.pdf)`, och när det är klart `Läste 9 pdf-filer, 9 med text.` cratchit läser varje fil en gång och kommer ihåg texten tills du väljer en mapp igen. Sedan visar cratchit en rad per fil:

```text
Pdf-underlag i /Users/me/Documents/The ITfied AB/M2 Juli-Sep: 9 filer
    1  4816_13_1784701252.pdf                                  text, 1 sida
    2  62600010249540.pdf                                      text, 2 sidor
    …
Cratchit > … > Utkast > Läs pdf-underlag > Pdf-underlag> 
```

| Står det | Betyder |
|---|---|
| `text, 2 sidor` | Filen har text som cratchit kan läsa |
| `text, 1 sida, 4 tecken oläsliga` | Filen har text, men några tecken gick inte att läsa; kontrollera texten innan du litar på den |
| `ingen text, 1 sida (inskannad?)` | Sidorna har ingen text, till exempel en inskannad bild |
| `inga sidor hittade` | cratchit hittade inga sidor i filen, till exempel för att den är skadad |
| `krypterad med lösenord` | Filen behöver ett lösenord för att öppnas |
| `krypterad (AES), går inte att läsa` | Filen är krypterad på ett sätt som cratchit inte kan läsa |
| `textkopiering spärrad i filen` | Den som skapade filen har spärrat kopiering av texten, och cratchit respekterar det |
| `inte en pdf` | Filen heter `.pdf` men är ingen pdf |
| `går inte att läsa` | Filen gick inte att öppna |

cratchit läser texten med koden från walk_pdf.

###### Underlag

<!-- hjälp: underlag -->
Ett underlag i taget, med dess text. Tryck `>` för nästa och `<` för föregående; vid första och sista gör de ingenting. Tryck `1` för att se hela texten och tolka underlaget. `0 Glöm` finns inte än. Tryck `-` för att gå tillbaka till listan.
<!-- /hjälp -->

Tryck `0 Tolka som händelser` i Pdf-underlag. cratchit visar den första filen:

```text
Underlag 5 av 9: Faktura50555657.pdf (text, 2 sidor)

=========================== sida 1 av 2 ============================
Faktura
Fakturadatum: 2026-08-07
OCR-nummer: 574512366662450
…
====================================================================
… 96 rader till

  < Föregående
  0 Glöm
  1 Tolka
  > Nästa
Cratchit > … > Pdf-underlag > Underlag 5> 
```

* Överst står filens nummer i listan, namnet och vad cratchit hittade i den.
* Underlagets text står mellan två dubbla linjer, `====`, med en tom rad före och efter. Den övre linjen säger vilken sida texten börjar på, och en enkel linje, `----`, skiljer sidorna åt.
* Högst 20 rader visas. Finns det fler står det under linjen hur många.
* En fil utan text visas med sin status och `Ingen text att visa.`, så att du kan gå vidare till nästa.
* `0 Glöm` ska ta bort underlaget ur listan, till exempel när filen inte är ett underlag. Det finns inte än.

###### Tolka

<!-- hjälp: tolka -->
Hela texten i underlaget. Att göra en händelse av underlaget finns inte än. Tryck `-` för att gå tillbaka till underlaget.
<!-- /hjälp -->

Tryck `1` i Underlag. cratchit visar hela texten, alla sidor, utan val:

```text
Tolka Faktura50555657.pdf (text, 2 sidor, 115 rader)

=========================== sida 1 av 2 ============================
Faktura
Fakturadatum: 2026-08-07
…
--------------------------- sida 2 av 2 ----------------------------
…
====================================================================

Att göra en händelse av underlaget finns inte än.
Cratchit > … > Pdf-underlag > Underlag 5 > Tolka> 
```

Tryck `-` för att gå tillbaka till samma underlag och fortsätta med `<` och `>`. Att skapa ett utkast av underlaget härifrån kommer senare.

#### Bokfört

<!-- hjälp: bokfort -->
Räkenskapsårets bokförda verifikationer, den senaste överst, 20 i taget. `…` överst betyder att det finns nyare, och `…` nederst att det finns äldre. Tryck `>` för nästa sida och `<` för föregående. Vid första och sista sidan gör de ingenting. Skriv en verifikations id som ett ord, till exempel `A12`, och tryck Enter för att öppna den. Tryck `0` för att göra den sista verifikationen i en serie till ett utkast igen. Tryck `-` för att gå tillbaka till Årsflöde.
<!-- /hjälp -->

Tryck `1` i Årsflöde för att se räkenskapsårets bokförda verifikationer. De visas en sida i taget, den senaste överst:

```text
Bokfört 2025/26: verifikationer 21–40 av 120
  …
  A49           2026-04-10        449.90  Utlägg Privat - Kjell&Co
  A50           2026-04-07          7.40  Account:NORDEA Text:AVGIFTER NORDEA
  …
  A30           2026-01-12       1250.00  Telia
  …
  0 Gör till utkast
  < Föregående sida
  > Nästa sida
Cratchit > Samlingsmapp > The ITfied AB > 2025/26 > Årsflöde > Bokfört> 
```

* En sida har högst 20 verifikationer.
* `…` överst betyder att det finns nyare verifikationer före sidan, och `…` nederst att det finns äldre efter den.
* `>` visar nästa sida, med äldre verifikationer, och `<` föregående sida. Båda valen finns alltid; på första sidan gör `<` ingenting, och på sista sidan gör `>` ingenting.
* Skriv en verifikations id och tryck Enter för att öppna den (se nedan).
* `0` gör den sista verifikationen i en serie till ett utkast igen (se nedan).

#### Gör till utkast

<!-- hjälp: gor-till-utkast -->
Den sista verifikationen i varje serie. Skriv seriens bokstav och tryck Enter för att göra den verifikationen till ett utkast igen. Kom den från ett annat program med en SIE-fil, eller har du exporterat den dit, finns den kvar där. Tryck `-` för att gå tillbaka.
<!-- /hjälp -->

En bokförd verifikation går inte att ändra. Den sista i en serie kan du ändå göra till ett utkast igen, till exempel för att rätta något du nyss bokförde. Tryck `0` i Bokfört eller `5` i Utkast:

```text
Den sista verifikationen i varje serie i 2026/27:
  A  A11           2026-07-03          3.70  AVGIFTER NORDEA  registrerad 2026-11-18
  D  D3            2026-06-12       1233.75  Websupport  registrerad 2026-07-15
  …
Kom verifikationen från ett annat program med en SIE-fil, eller har du exporterat den dit, finns den kvar där. Då skiljer sig bokföringarna tills du har rättat där också.
Skriv seriens bokstav och tryck Enter för att göra verifikationen till ett utkast.
Cratchit > … > Bokfört > Gör till utkast>
```

Skriv seriens bokstav, stor eller liten, och tryck Enter. Verifikationen blir direkt ett utkast; cratchit frågar inte först, eftersom du kan bokföra utkastet igen. Bokför du det i samma serie får det samma nummer. Du kommer tillbaka dit du var, där det står till exempel `cratchit gjorde A11 till utkast #1: AVGIFTER NORDEA.` Loggen får hela verifikationen.

Utkastet får verifikationens datum, text och belopp, och dess konteringar med datum och texter. En struken rad kommer inte med, och en tillagd rad blir en vanlig kontering. Utkastet noteras i dag och är *klart*, så det har `2 Bokför` i sin vy och i [Fullborda utkast](#fullborda-utkast). Det som går förlorat:

* **tecknet**: verifikationen har beloppet utan tecken, så en kreditfaktura kommer tillbaka med plus;
* **kontoutdragsraden** en kontering kom från, som verifikationen inte sparade;
* **registreringsdagen och signaturen**: bokför du igen blir registreringsdagen ny.

cratchit vet inte vilka verifikationer som har kommit från eller gått till ett annat program, så varningen står alltid med. Bokföringslagen tillåter inte att en bokförd verifikation bara försvinner; använd det till att rätta det du nyss bokförde i cratchit.

#### Öppna en verifikation

En verifikation har ett id: serien och numret som ett ord, till exempel `A12` eller `D1`. Skriv id:t i *Bokfört* och tryck Enter:

```text
Cratchit > Samlingsmapp > The ITfied AB > 2025/26 > Årsflöde > Bokfört: D19
```

* Skriv id:t utan mellanslag. Serien skrivs exakt, med versal: `a12` hittar inte `A12`.
* Numret jämförs som ett tal, så `D1` är aldrig `D11`.
* Finns verifikationen inte i räkenskapsåret, eller är id:t skrivet på annat sätt, säger cratchit det under prompten, och texten står kvar så att du kan rätta den.

<!-- hjälp: verifikation -->
Verifikationen i tre delar. Överst hur den är bokförd: serien med seriens namn, när den registrerades och signaturen när den finns. Sedan *Händelse*: datum, rubrik och belopp. Sist *Konteringar*: konto, kontots namn och belopp, och radens egen text och datum när den har dem. En struken rad är märkt `(struken)` och en tillagd rad `(tillagd)`. Tryck `0` för händelsen och `1` för konteringarna. Tryck `2` för att skapa en [ändringsverifikation](#ändringsverifikation) som vänder verifikationen. Verifikationen själv går inte att ändra. Tryck `-` för att gå tillbaka till Bokfört.
<!-- /hjälp -->

```text
Verifikation D19
  Serie:          D  Leverantörsfakturor
  Registrerad:    2026-04-17
Händelse:
  Datum:          2026-02-27
  Rubrik:         Faktura Websupport - webbhotell
  Belopp:         1233.75
Konteringar:
  2440  Leverantörsskulder           -1233.75
  6210  Telekommunikation              987.00
  2641  Debiterad ingående moms        246.75
  0 Händelse
  1 Konteringar
Cratchit > Samlingsmapp > The ITfied AB > 2025/26 > Årsflöde > Bokfört > D19> 
```

En verifikation har två delar (beslut 2026-11-16):

* **Händelse** är vad som hände, när och för hur mycket: datum, rubrik och belopp.
* **Konteringar** är hur händelsen bokförs: raderna med konto och belopp.

Serien, registreringsdagen och signaturen står för sig, överst. De säger hur verifikationen är bokförd, inte vad som hände.

<!-- hjälp: handelse -->
Händelsen i verifikationen: datum, rubrik och belopp. I en bokförd verifikation är händelsen låst: valen leder till *Låst*, som säger vad du kan göra i stället. Tryck `-` för att gå tillbaka till verifikationen.
<!-- /hjälp -->

```text
Händelsen i D19
  0 Datum: 2026-02-27
  1 Rubrik: Faktura Websupport - webbhotell
  2 Belopp: 1233.75
Cratchit > … > Bokfört > D19 > Händelse> 
```

Händelsen i en bokförd verifikation går inte att ändra (beslut 2026-11-16): datum, rubrik och belopp är låsta. Väljer du något av dem kommer du till *Låst*:

```text
Det går inte att ändra datumet i en bokförd verifikation.
Bara den sista verifikationen i en serie går att göra om till ett utkast och ändra fritt. I serie D är det D23. Är händelsen i D19 fel rättar du den med en ny verifikation.
Cratchit > … > Bokfört > D19 > Händelse > Låst (Datum)> 
```

<!-- hjälp: last -->
Det du valde går inte att ändra i en bokförd verifikation. Är verifikationen den sista i sin serie kan du göra om den till ett utkast, där du kan ändra händelsen fritt och bokföra den igen: tryck `0` i Bokfört (se [Gör till utkast](#gör-till-utkast)). Annars rättar du en felaktig händelse med en ny verifikation. Tryck `-` för att gå tillbaka.
<!-- /hjälp -->

<!-- hjälp: konteringar -->
Verifikationens eller utkastets konteringar, numrerade `0`–`9`. Tryck en rads siffra för att öppna den. Finns det fler än tio rader bläddrar `<` och `>` en sida. Strukna rader är med, märkta `(struken)`. Tryck `-` för att gå tillbaka.
<!-- /hjälp -->

```text
Konteringar i D19
  0 2440  Leverantörsskulder           -1233.75
  1 6210  Telekommunikation              987.00
  2 2641  Debiterad ingående moms        246.75
Cratchit > … > Bokfört > D19 > Konteringar> 
```

<!-- hjälp: kontering -->
En kontering: konto med kontots namn, beskrivning och belopp, och radens datum när det skiljer sig från händelsens. Står det att raden är struken eller tillagd är den en del av en rättelse. En rad från ett kontoutdrag visar vilken fil och rad den kom ifrån. Än så länge kan du bara se raden: valen leder till *Inte än*. Tryck `-` för att gå tillbaka till konteringarna.
<!-- /hjälp -->

```text
Kontering 1 i D19
  0 Konto: 6210  Telekommunikation
  1 Beskrivning:
  2 Belopp: 987.00
Cratchit > … > Bokfört > D19 > Konteringar > 1> 
```

Än så länge visar cratchit bara verifikationen. Att rätta den, med strukna och tillagda rader, kommer senare.

**Serier.** Varje verifikation hör till en serie, en bokstav. En SIE-fil har inga namn på serierna, så bokföringen har sina egna. Från början har den Fortnox serier: A Redovisning, B Kundfakturor, C Inbetalningar från kunder, D Leverantörsfakturor, E Utbetalningar till leverantörer, F Kassa, G Avskrivning, H Periodisering, I Bokslut, J Kontantfaktura, K Kvitto, L Lön, M Momsrapport och N Lager. En serie som en SIE-fil använder men som inte finns där läggs till utan namn, och visas då bara med sin bokstav. Att byta namn på en serie eller lägga till en kommer senare.

#### Ändringsverifikation

En bokförd verifikation går inte att ändra, men du kan boka bort den med en ändringsverifikation: en ny verifikation som vänder den, så att de tillsammans blir 0. Det använder du till exempel när en faktura blivit bokförd två gånger; den andra fanns aldrig i verkligheten. Det är du som avgör när det speglar verkligheten, och det går för alla verifikationer, inte bara den sista i en serie. Tryck `2` i verifikationens vy:

```text
Verifikation D19
  …
  0 Händelse
  1 Konteringar
  2 Ändringsverifikation
Cratchit > … > Bokfört > D19> 2
cratchit skapade utkast #5: Ändring av D19, daterat 2025-03-10 som D19.
Se till att datumet inte ligger i ett år eller en momsperiod som redan är rapporterad; då krävs en ny rapport. Datumet ändrar du med 0 i utkastets vy innan du bokför.
```

cratchit skapar ändringen som ett utkast, inte som en bokförd verifikation, och du står kvar i verifikationens vy. Utkastet har

* **rubriken** `Ändring av D19`;
* **beloppet** med minus, `-1233.75`: händelsen går åt andra hållet än vanligt;
* **konteringarna** med vänt tecken och samma texter, men utan egna datum; en struken rad kommer inte med, och en tillagd rad vänds som de andra;
* **datumet** från verifikationen, så att ändringen hamnar i samma period som det den vänder.

Utkastet är klart. Bokför det med `2 Bokför` i utkastets vy eller i [Fullborda utkast](#fullborda-utkast), där du väljer serien. Ligger verifikationens datum i ett år eller en momsperiod som redan är rapporterad skulle ändringen kräva en ny rapport; ändra då datumet till en period som inte är rapporterad, med `0` i utkastets vy, innan du bokför. Vill du inte ha ändringen glömmer du utkastet.

#### Bokslut

<!-- hjälp: bokslut -->
Bokslutet avslutar räkenskapsåret. Tryck `0` för avstämning mot kontoutdrag. Den finns inte än. Tryck `-` för att gå tillbaka till Årsflöde.
<!-- /hjälp -->

#### Momsrapport

<!-- hjälp: momsrapport -->
Räkenskapsårets momskvartal, det senaste först, vart och ett med momsen i tre steg: *Period* (momsen på momskontona i kvartalets verifikationer), *Avräknat* (det som en momsverifikation har flyttat till 2650 eller 1650) och *Debiterat* (det Skatteverket har debiterat eller krediterat skattekontot för kvartalet). Tryck kvartalets siffra för att se dess momsrapport; `0` är det senaste kvartalet som har tagit slut. Kvartalet som pågår i dag har `+`. Tryck `-` för att gå tillbaka till Årsflöde.
<!-- /hjälp -->

Tryck `2` i Årsflöde:

```text
Momskvartal som slutar i 2026/27 (i dag 2026-11-18):
                   Period    Avräknat   Debiterat
  + 2026 Q4          0.00        0.00        0.00  hittills
  0 2026 Q3      -8467.20        0.00        0.00  nästa att rapportera  1 kontroll
  1 2026 Q2       2292.68     2291.00        0.00  avräknat av M5, M1
Cratchit > … > Årsflöde > Momsrapport>
```

Ett kvartal hör till räkenskapsåret när det slutar i det, så med brutet räkenskapsår kan första kvartalet börja i förra året. Minus är moms att betala och plus moms att få tillbaka, som på momskontona. *nästa att rapportera* står vid kvartalet före det som pågår, och *kontroll* vid ett kvartal där kontrollerna har hittat något. En debitering som inget kvartal tar står under listan.

##### Kvartalets momsrapport

<!-- hjälp: momskvartal -->
Kvartalets momsrapport: varje ruta med sitt belopp på blanketten, rutans konton och verifikationerna bakom, sedan stegen och ruta 49. Rapporten visas en sida i taget; tryck `>` och `<` för att bläddra. Under den står kontrollerna, förslaget till momstömning och eSKD-filens fält. Tryck `0` för att granska rutorna, `1` för att göra momstömningen och bokföra den, och `2` för att skriva rapporten och eSKD-filen. Tryck `-` för att gå tillbaka till kvartalen.
<!-- /hjälp -->

Rapporten är den som kvartalets verifikationer ger, utan kvartalets egen momstömning. Den blir alltså densamma före och efter att du har bokfört tömningen, och efteråt är den den du lämnade in. Under rapporten:

```text
Kontroller av 2026 Q3 (de varnar bara):
  ! …

Momstömning (förslag): M 2026-09-30 "Momsrapport 2026-07-01 - 2026-09-30"
  2611  Utgående moms 25%                       8500.00
  2641  Debiterad ingående moms                  -32.80
  2650  Redovisningskonto för moms             -8468.00
  3740  Öres- och kronutjämning                     0.80

eSKD-fil moms_202609.eskd:
  MomsUtgHog                    8500
  MomsIngAvdr                     32
  MomsBetala                    8468
```

Kontrollerna varnar för det som kan göra rapporten fel: utkast som är daterade i kvartalet, 2650 eller 1650 i en verifikation som inte är en momsverifikation, omvänd skattskyldighet som inte går jämnt upp, en verifikation som registrerades efter kvartalets momstömning, och konton 2600–2699 som inte redovisas i någon ruta.

* `1 Momstömning` finns för ett kvartal som har tagit slut, har något att avräkna och inte redan är avräknat. cratchit gör tömningen till ett utkast i serie M, daterat kvartalets sista dag, och går direkt till [Bokför](#bokför), där `M` står ifyllt. Enter bokför. Trycker du `-` ligger tömningen kvar som utkast, och vyn säger vilket; nästa `1` använder samma utkast. Har bokföringen ändrats sedan dess byter cratchit ut utkastets konteringar. Efter bokföringen står det till exempel `2026 Q3 är redan avräknat av M3.`
* `2 Skriv rapport och eSKD-fil` skriver mappen `moms_<första dag>_<sista dag>` i bokföringens mapp, till exempel `moms_2026-07-01_2026-09-30`, med `momsrapport.txt` (rapporten och kontrollerna), `verifikation.txt` (tömningen, eller att kvartalet redan är avräknat) och eSKD-filen `moms_202609.eskd`. Rapport och verifikation skrivs om varje gång. En eSKD-fil skrivs aldrig över; nästa får ett nummer, till exempel `moms_202609_2.eskd`, så filen du laddade upp finns kvar. Under vyn står varje fil med hela sökvägen, så att du hittar eSKD-filen när du laddar upp den i Skatteverkets e-tjänst *Lämna momsdeklaration*:

```text
cratchit skrev:
  /Users/du/Documents/The ITfied AB/moms_2026-07-01_2026-09-30/momsrapport.txt
  /Users/du/Documents/The ITfied AB/moms_2026-07-01_2026-09-30/verifikation.txt
  /Users/du/Documents/The ITfied AB/moms_2026-07-01_2026-09-30/moms_202609.eskd
Ladda upp moms_202609.eskd i Skatteverkets e-tjänst Lämna momsdeklaration.
```

För kvartalet som pågår skriver cratchit bara rapporten.

Ordningen mellan `1` och `2` spelar ingen roll. eSKD-filen behöver organisationsnumret; saknar bokföringen det säger cratchit det.

###### Granska rutorna

<!-- hjälp: granska-rutor -->
Blankettens alla rutor under sina avsnitt, med rubrik och hur många konteringar kvartalet har i varje; också rutor utan konteringar, så att en ruta som saknas syns. Skriv rutans nummer och tryck Enter för att se den. Tryck `-` för att gå tillbaka till kvartalet.
<!-- /hjälp -->

###### Ruta

<!-- hjälp: ruta -->
En ruta: dess rubrik och avsnitt, beloppet på blanketten, kontona och verifikationerna bakom, andra konton som redovisas i rutan, och Skatteverkets beskrivning av rutan. Tryck `>` för nästa ruta och `<` för föregående. Tryck `-` för att gå tillbaka till rutorna.
<!-- /hjälp -->

Momsrapporten kan du också göra från kommandoraden med `cratchit --sie-to-vat` (se [Kommandoraden](#kommandoraden)).

#### Årsredovisning

<!-- hjälp: arsredovisning -->
Årsredovisningen gör cratchit från kommandoraden, med `cratchit --sie-to-ar`. Ovanför prompten står stegen med kommandona för den här bokföringen och det här räkenskapsåret: exportera året till en SIE-fil, läs hjälpen, avsluta cratchit och kör kommandot i bokföringens mapp. Tryck `-` för att gå tillbaka till Årsflöde.
<!-- /hjälp -->

Tryck `4` i Årsflöde. cratchit visar till exempel:

```text
Årsredovisningen gör cratchit från kommandoraden, av en SIE-fil för räkenskapsåret.

1. Exportera räkenskapsåret till en SIE-fil: 1 Exportera till SIE > 1 Allt i räkenskapsåret.
   Senast exporterad: sie_2025-26_allt.se. Exportera igen om du har bokfört något sedan dess.

2. Läs hjälpen, bland annat om --bas-year:
     cratchit --sie-to-ar --help | less

3. Avsluta cratchit (qq) och kör i bokföringens mapp:
     cd /Users/me/Documents/itfied
     cratchit --sie-to-ar sie_2025-26_allt.se
   cratchit skapar mappen 556782-8172_2025-05-01_2026-04-30 med årsredovisningen, en rapport och en kopia av SIE-filen.
```

* SIE-filen är den senaste exporten av räkenskapsåret i bokföringens mapp. Finns ingen står det `<SIE-fil>`; exportera året först.
* Mappen och organisationsnumret är bokföringens. En sökväg med mellanslag står inom citattecken.
* På Windows står `cd /d` och `| more` i stället för `cd` och `| less`.

#### Deklaration till SKV

<!-- hjälp: deklaration -->
Inkomstdeklaration 2 (INK2) gör cratchit från kommandoraden, med `cratchit --sie-to-ink2`. Ovanför prompten står stegen med kommandona för den här bokföringen och det här räkenskapsåret, som för årsredovisningen. Kommandot behöver också förra årets deklaration, för underskottet, eller förra årets underskott i kronor. Tryck `-` för att gå tillbaka till Årsflöde.
<!-- /hjälp -->

Tryck `7` i Årsflöde. Stegen är desamma som för årsredovisningen. Har du gjort förra årets deklaration med `cratchit --sie-to-ink2` i bokföringens mapp, och den fick sina SRU-filer, föreslår cratchit den mappen:

```text
     cratchit --sie-to-ink2 sie_2025-26_allt.se 556782-8172_2024-05-01_2025-04-30
```

Annars står det var förra årets `BLANKETTER.SRU` ska anges, och hur du anger förra årets underskott i stället:

```text
     cratchit --sie-to-ink2 sie_2025-26_allt.se --previous-deficit 0
```

### Importera från SIE

<!-- hjälp: importera-sie -->
Skriv sökvägen till en SIE-fil för det här räkenskapsåret och tryck Enter. Tab fyller i mappar och SIE-filer. cratchit jämför filen med bokföringen och visar om importen går att göra, innan något importeras. Så länge du inte har ändrat sökvägen går `-` tillbaka.
<!-- /hjälp -->

I ett räkenskapsår tar du in en SIE-fil för samma år från ett annat bokföringsprogram, till exempel nya verifikationer. Filen måste gälla just det räkenskapsåret. Gäller den ett annat år säger cratchit det och importerar ingenting.

1. Tryck `2` (*Importera från SIE*) i räkenskapsåret.
2. Skriv sökvägen till SIE-filen. Sökvägen börjar i bokföringens mapp, och Tab fyller i mappar och SIE-filer.
3. Tryck Enter. cratchit jämför filen med bokföringen, serie för serie, och säger om importen går att göra.
4. Kontrollera det som står. Tryck `J` (*Ja*) för att importera, eller `N` (*Nej*) för att välja en annan fil.

```text
SIE-fil: /Users/anna/Företaget/SIE-filer/TheITfiedAB20261112_102636.se
  Räkenskapsår:   2025-05-01 – 2026-04-30, fullbordat
  Skapad:         2026-11-12 (Fortnox 3.62.17)
  Verifikationer: 120 t.o.m. 2026-04-30
Jämfört med bokföringen (2025/26):
  A  lika, t.o.m. A59
  D  lika, t.o.m. D26
  I  bokföringen t.o.m. I1, filen t.o.m. I5, nya: I2–5
  Konton: 0 nya, 0 med ändrat namn eller ändrade uppgifter
Importen går att göra: 4 nya verifikationer.
Importera SIE-filen?
  J Ja
  N Nej
Cratchit > Samlingsmapp > The ITfied AB > 2025/26 > Importera från SIE > Bekräfta> 
```

Går importen inte att göra står det varför, och verifikationerna i bokföringen som filen skulle ta bort eller byta ut står under sin serie. Är det just de verifikationerna som hindrar importen kan du i stället ersätta räkenskapsåret med filens innehåll (se [Ersätta ett räkenskapsår med en SIE-fil](#ersätta-ett-räkenskapsår-med-en-sie-fil)):

```text
Jämfört med bokföringen (2025/26):
  D  bokföringen t.o.m. D28, filen t.o.m. D26
      D27  2026-04-12  Loopia Faktura  311.25  finns inte i filen
      D28  2026-04-30  Telia  1250.00  finns inte i filen
Importen går inte att göra. Bokföringen har 2 verifikationer som filen skulle ta bort eller byta ut.
Ersätt året med SIE-filens innehåll?
  J Ja
  N Nej
```

Hindrar något annat importen, till exempel en lucka i en serie, finns bara `N` (*Tillbaka*).

<!-- hjälp: bekrafta-import -->
Ovanför prompten står SIE-filen och hur den förhåller sig till bokföringen, serie för serie: hur långt bokföringen och filen räcker, och vilka verifikationer som är nya. Sist står om importen går att göra, och annars varför. Ändrade verifikationer listas tio i taget, och `2` (*Mer…*) visar fler. Tryck `J` för att importera, eller, när cratchit frågar om att ersätta året, för att se vad en ersättning tar bort. Tryck `N` eller `-` för att gå tillbaka och välja en annan fil; sökvägen börjar då i mappen där du valde filen.
<!-- /hjälp -->

När importen är klar går cratchit tillbaka till räkenskapsåret, som säger vad som importerades. SIE-filen ligger kvar där den låg (se [Dina filer](#dina-filer)).

cratchit säger direkt under prompten att filen inte går att använda, och visar ingen jämförelse, om:

* filen inte är en SIE-fil, har fel eller saknar räkenskapsår,
* filen gäller ett annat räkenskapsår än det du står i (se nedan om år som ansluter),
* filens räkenskapsår är nytt men inte ansluter till bokföringen. Ett nytt år måste börja dagen efter det senaste bokförda årets sista dag, eller sluta dagen före det äldsta årets första dag. Bokföringen får inga luckor mellan åren.

*Bekräfta* visar jämförelsen, men säger att importen inte går att göra, om:

* filen gäller ett annat organisationsnummer än bokföringen,
* en serie i filen har en lucka, till exempel `Serie A i filen har en lucka: A12 saknas.` Filen ska ansluta till bokföringen utan luckor,
* allt i filen redan finns i bokföringen,
* importen skulle ta bort eller byta ut verifikationer som redan finns i bokföringen,
* en verifikation som redan finns har ett annat datum eller en annan text i filen.

**Ett år med andra datum.** I ett år där inget är bokfört tar cratchit emot en SIE-fil för ett år med andra datum, om filens år ansluter till bokföringen och är högst 18 månader:

* i det första året efter det senaste bokförda året, om filens år börjar dagen efter det årets sista dag. Så tar du in en omläggning av räkenskapsåret.
* i året före det äldsta bokförda året, om filens år slutar dagen före det årets första dag. Så tar du in ett första år som är kortare eller längre än tolv månader.

*Bekräfta* visar då vilket år cratchit räknade med:

```text
  Räkenskapsår:   2026-01-01 – 2026-04-30, ofullbordat (cratchit räknade med 2026-01-01 – 2026-12-31)
  Ansvar:         cratchit kontrollerar inte Skatteverkets godkännande av en omläggning. Du ansvarar för att räkenskapsåret är korrekt.
```

Efter importen har året filens datum, och de följande åren räknas från det, tolv månader i taget. I alla andra år måste filen ha exakt samma datum som året.

**Ändrade verifikationer.** En verifikation som redan är bokförd får enligt SIE bara ändras genom att rader stryks (`#BTRANS`) och nya rader läggs till (`#RTRANS`). Det som redan står får inte ändras. Sådana ändringar tar cratchit emot. *Bekräfta* listar då varje ändrad verifikation, till exempel `A12  2025-06-03  Loopia: 1 rad struken, 1 rad tillagd`, tio i taget, och `2` (*Mer…*) visar fler. Ändringarna skrivs också i loggen.

### Ersätta ett räkenskapsår med en SIE-fil

När en import inte går att göra, därför att bokföringen har verifikationer som filen skulle ta bort eller byta ut, frågar cratchit om du vill ersätta räkenskapsåret med filens innehåll. Tryck `J`, så visar cratchit vad som försvinner:

```text
Räkenskapsåret 2025/26 blir exakt som SIE-filen.
Det här försvinner ur bokföringen:
  D27           2026-04-12  Loopia Faktura  311.25  finns inte i filen
  D28           2026-04-30  Telia  1250.00  finns inte i filen
Det här kommer in från filen:
  I2–5
Ingående balanser i följande räkenskapsår räknas om.
Ersätt räkenskapsåret med SIE-filen?
  J Ja
  N Nej
Cratchit > … > Importera från SIE > Bekräfta > Ersätt> 
```

<!-- hjälp: ersatt-sie -->
Ovanför prompten står vad som försvinner ur bokföringen och vad som kommer in från SIE-filen. Tryck `J` för att ersätta räkenskapsåret med filens innehåll. Det som försvinner är borta; bokför det igen från underlag eller kontoutdrag om det behövs. Tryck `N` för att välja en annan fil, eller `-` för att gå tillbaka ett steg.
<!-- /hjälp -->

Att ersätta är att bränna broarna och göra om:

* Räkenskapsårets verifikationer blir exakt filens. Det som skilde sig är borta ur bokföringen. cratchit sparar ingen kopia.
* Årets utkast ligger kvar, utom de som motsvarar en verifikation i filen.
* Konton och företagsuppgifter tas från filen som vid en import, och ingående balanser i följande år räknas om.
* Är året stängt ersätts det ändå, och förblir stängt.
* cratchit skriver i sin logg vad som försvann, med alla rader, och för in filen i förteckningen.

Efteråt går cratchit tillbaka till räkenskapsåret, som visar vad som försvann. Bokför det igen från underlag eller kontoutdrag om det behövs.

cratchit erbjuder inte att ersätta när filen har ett annat organisationsnummer, när en serie i filen har en lucka eller när allt redan finns i bokföringen.

### Bygga upp bokföringen från SIE-filer

Det här är ett och samma sätt att arbeta när du

* **flyttar in** bokföringen från ett annat program och vill ha historiken med,
* **håller cratchit i takt** med ett annat program som du fortfarande använder,
* **lagar** en bokföring som inte längre stämmer med programmet som har rätt bokföring.

Du bygger år för år, från det senaste räkenskapsåret bakåt. Så ansluter varje år till det som redan finns, bokföringen får inga luckor, och året du arbetar i är på plats först.

**Förbered.** Exportera en SIE-fil per räkenskapsår från programmet och lägg dem i en mapp. När du letar upp filerna med Tab visar cratchit år och status för varje fil (se [Räkenskapsårets status i en SIE-fil](#räkenskapsårets-status-i-en-sie-fil)). Välj filer som är *fullbordat* för passerade år. Det innevarande året är *pågående*, och förra året kan vara *ofullbordat* om bokslutet inte är gjort.

**Steg 1, bara när bokföringen inte finns än.** Starta cratchit i företagets mapp och välj `1` (*Ny bokföring här*) och `1` (*Från SIE*) med det senaste räkenskapsårets fil (se [Ny bokföring här](#ny-bokföring-här)).

**Steg 2, ett år i taget, från det senaste bakåt.** Öppna året i listan över räkenskapsår och välj `2` (*Importera från SIE*) med årets fil. För ett äldre år som inte finns än öppnar du det tomma året sist i listan.

* Går importen att göra: tryck `J`.
* Går den inte att göra på grund av konflikter: svara `J` på *Ersätt året med SIE-filens innehåll?*, se vad som försvinner, och tryck `J` igen (se [Ersätta ett räkenskapsår med en SIE-fil](#ersätta-ett-räkenskapsår-med-en-sie-fil)). Bokför det som försvann igen från underlag eller kontoutdrag, om det behövs.

**Gränser.** cratchit hanterar det aktuella räkenskapsåret och högst 9 äldre; med 2026/27 som aktuellt år kan du gå tillbaka till 2017/18. Varje år måste ansluta till de som finns, och en fil med en lucka i en serie tar cratchit inte emot.

Så här blir det för de tre fallen:

| Fall | Gör så här |
| --- | --- |
| Flytta in | Steg 1, sedan steg 2 för de äldre åren. |
| Hålla i takt | Steg 2 för de år som har ändrats i det andra programmet, oftast bara det innevarande. |
| Laga | Steg 2 för de år som är fel. *Ersätt* lagar på plats, i samma bokföring, utan att något behöver tas bort. |

### Exportera till SIE

<!-- hjälp: exportera-sie -->
Tryck `1` *Allt* för att skriva hela räkenskapsåret, med balanser och alla verifikationer, till en SIE-fil. Tryck `2` *Skillnad* för att jämföra året med en SIE-fil från ett annat program och skriva bara de verifikationer som filen saknar. Filen hamnar i bokföringens mapp.
<!-- /hjälp -->

Med en SIE-fil kan du lämna över ett räkenskapsår till ett annat bokföringsprogram. Tryck `1` (*Exportera till SIE*) i räkenskapsåret. Där finns två val:

* `1` *Allt*: hela räkenskapsåret, med kontoplan, ingående och utgående balanser, resultat och alla verifikationer. Filen heter till exempel `sie_2025-26_allt.se`.
* `2` *Skillnad*: bara de verifikationer som finns i räkenskapsåret men inte i en SIE-fil som du väljer, utan balanser. Filen heter till exempel `sie_2025-26_skillnad.se`.

Filen hamnar i bokföringens mapp, och cratchit skriver aldrig över en tidigare export: finns filen redan får den nya filen ett nummer, till exempel `sie_2025-26_allt_2.se`. När filen är skriven går cratchit tillbaka till räkenskapsåret och säger vad som skrevs:

```text
Räkenskapsår 2025-05-01 – 2026-04-30
cratchit skrev sie_2025-26_allt.se i /Users/anna/Documents/cratchit/The ITfied AB: 115 verifikationer och balanser.
  1 Exportera till SIE
Cratchit > Samlingsmapp > The ITfied AB > 2025/26> 
```

<!-- hjälp: skillnad-sie -->
Skriv sökvägen till SIE-filen att jämföra med och tryck Enter. Filen ska vara en export som du just har gjort från programmet du ska föra över till, till exempel Fortnox, så att den visar hur bokföringen ser ut där nu. Är filen äldre kan verifikationer som redan finns i programmet komma med igen, och då läggs de till en gång till. Sökvägen börjar i bokföringens mapp, och Tab fyller i mappar och SIE-filer.
<!-- /hjälp -->

*Skillnad* är till för när du bokför i cratchit och vill föra över det nya till ett annat program. Exportera först året från det andra programmet till en SIE-fil, och välj sedan den filen här. cratchit jämför verifikationerna efter datum och konton och belopp, inte efter nummer.

**Jämför alltid med en färsk export.** Skillnaden räknas mot SIE-filen du väljer, inte mot programmet självt. Den blir därför bara rätt om filen visar hur bokföringen i programmet ser ut just nu. Gör så här varje gång:

1. Exportera räkenskapsåret från programmet du ska föra över till.
2. Välj den filen i *Skillnad* och skriv skillnaden.
3. Läs in skillnadsfilen i programmet.

Nästa gång börjar du om från steg 1. Den gamla exporten saknar ju det du just läste in, så en jämförelse med den skulle skriva samma verifikationer igen. Är filen inte skapad i dag, eller saknar den datum, frågar cratchit i jämförelsen om det är den senaste exporten:

```text
Jämfört med TheITfiedAB.se (Fortnox 3.0, skapad 2026-10-19):
Filen skapades 2026-10-19. Är det den senaste exporten från programmet?
  I räkenskapsåret, inte i filen (2):
    A116    2026-04-28       1250.00  Loopia
    A117    2026-04-30        310.00  Bankavgift
  I filen, inte i räkenskapsåret (0):
  1 Skriv skillnaden till en SIE-fil
Cratchit > Samlingsmapp > The ITfied AB > 2025/26 > Exportera till SIE > Skillnad > TheITfiedAB.se> 
```

<!-- hjälp: jamforelse-sie -->
Ovanför prompten står vilka verifikationer som bara finns i räkenskapsåret och vilka som bara finns i SIE-filen. Tryck `1` för att skriva de verifikationer som bara finns i räkenskapsåret till en SIE-fil. Har filen verifikationer som inte finns i räkenskapsåret, eller som ser annorlunda ut där, skriver cratchit ingen fil. Tryck `-` för att gå tillbaka.
<!-- /hjälp -->

Ska du föra över till Fortnox eller ett liknande program, använd *Skillnad* och inte *Allt*. Program som Fortnox lägger till alla verifikationer i filen, med nya nummer i samma serie, men kan inte ändra eller ta bort de verifikationer som redan finns. En fil med hela året skulle därför dubblera verifikationerna i programmet. Exportera först året från programmet, och jämför med den filen.

Har SIE-filen verifikationer som inte finns i räkenskapsåret, eller som ser annorlunda ut där, skriver cratchit ingen fil. Program som Fortnox kan bara lägga till verifikationer, så de måste stämma först: gör ändringen i programmet, eller ta in den i cratchit. Finns alla verifikationer redan i filen säger cratchit att det inte finns något att exportera.

### Lägg till

<!-- hjälp: lagg-till -->
Tryck `1` *Befintlig* för att lägga till en bokföring som redan finns men inte står i samlingsmappens lista, till exempel en som du har flyttat, kopierat eller tagit med från en annan dator. Tryck `2` *Ny* för att skapa en ny bokföring, från en SIE-fil eller från uppgifter om företaget.
<!-- /hjälp -->

Du kommer hit med `+` i samlingsmappen.

### Lägga till en befintlig bokföring

<!-- hjälp: oppna-befintlig -->
Skriv sökvägen till mappen som bokföringen ligger i och tryck Enter. Tab fyller i mappnamn. cratchit lägger till bokföringen i samlingsmappens lista och går tillbaka dit. Så länge du inte har ändrat sökvägen går `-` tillbaka. Annars tömmer Esc raden först.
<!-- /hjälp -->

Har du en bokföring som samlingsmappen inte listar tar du in den så här:

1. Tryck `+` (*Lägg till bokföring*) i samlingsmappen.
2. Tryck `1` (*Befintlig*).
3. Skriv sökvägen till mappen som bokföringen ligger i. Sökvägen börjar i samlingsmappen, och Tab fyller i mappnamnen (se [Skriva en sökväg](#skriva-en-sökväg)).
4. Tryck Enter.

```text
Cratchit > Samlingsmapp > Lägg till > Befintlig: /Users/anna/Documents/Bokföringar/The ITfied AB/
Samlingsmappen kommer ihåg The ITfied AB.
/Users/anna/Documents/cratchit
    Namn           Org.nr       Skapad      Mapp
  1 The ITfied AB  556782-8172  2026-10-23  ~/Documents/Bokföringar/The ITfied AB
  + Lägg till bokföring
Cratchit > Samlingsmapp> 
```

cratchit går tillbaka till samlingsmappen, där bokföringen nu finns i listan. Där öppnar du den med dess siffra, som alla andra bokföringar.

Ligger det ingen bokföring i mappen säger cratchit det, och sökvägen står kvar så att du kan rätta den.

### Flytta en bokföring

Allt som hör till en bokföring ligger i dess mapp. Flytta eller kopiera mappen dit du vill ha den, och lägg sedan till den på det nya stället med `+` och `1` (*Befintlig*). cratchit säger då "Samlingsmappen kommer ihåg … på det nya stället.", och listan visar den nya platsen.

### Ny bokföring

<!-- hjälp: ny-bokforing -->
Välj hur den nya bokföringen ska skapas: `1` från en SIE-fil eller `2` från namn, organisationsnummer och räkenskapsår.
<!-- /hjälp -->

Tryck `+` och sedan `2` (*Ny*). Där finns två val:

* `1` *Från SIE*: skapa bokföringen från en SIE-fil (se nedan).
* `2` *Från uppgifter*: skapa bokföringen från namn, organisationsnummer och räkenskapsår (se [Ny bokföring från uppgifter](#ny-bokföring-från-uppgifter)).

### Ny bokföring från en SIE-fil

<!-- hjälp: fran-sie -->
Skriv sökvägen till SIE-filen och tryck Enter. Tab fyller i mappar och SIE-filer. Bokföringen skapas i den mapp där SIE-filen ligger, eller under *Ny bokföring här* i mappen du startade i. cratchit läser filen och visar vad den innehåller innan något skapas. Så länge du inte har ändrat sökvägen går `-` tillbaka.
<!-- /hjälp -->

En SIE-fil är en fil med bokföring som du har hämtat från ett annat bokföringsprogram. Bokföringen skapas i den mapp där SIE-filen ligger. Lägg därför först SIE-filen i den mapp du vill ha bokföringen i, till exempel en ny mapp för företaget eller en mapp du redan har för det. Så skapar du bokföringen:

1. Tryck `+`, `2` (*Ny*) och `1` (*Från SIE*).
2. Skriv sökvägen till SIE-filen. Sökvägen börjar i samlingsmappen, och Tab fyller i mappar och SIE-filer.
3. Tryck Enter. cratchit läser filen och visar vad den innehåller.
4. Tryck `J` (*Ja*) för att skapa bokföringen, eller `N` (*Nej*) för att välja en annan fil.

```text
SIE-fil: /Users/anna/Företaget/TheITfiedAB.se
  Mapp:           /Users/anna/Företaget
  Företag:        The ITfied AB (556782-8172)
  Räkenskapsår:   2026-05-01 – 2027-04-30, pågående
  Skapad:         2026-10-19 (Fortnox 3.62.17)
  Verifikationer: 42 t.o.m. 2026-10-12  A21 · D11 · M10
  Konton:         501
Skapa bokföringen från SIE-filen?
  J Ja
  N Nej
Cratchit > Samlingsmapp > Lägg till > Ny > Från SIE > Bekräfta> 
```

<!-- hjälp: bekrafta-sie -->
Ovanför prompten står mappen som bokföringen skapas i, alltså SIE-filens mapp, och vad cratchit hittade i SIE-filen: företaget, räkenskapsåret och dess status (pågående, ofullbordat, fullbordat eller misstämmer), när och med vilket program filen skapades, verifikationerna och kontona. En rad *Varning* säger till om ett äldre år är ofullbordat eller om filen misstämmer. Tryck `J` för att skapa bokföringen. Tryck `N` eller `-` för att gå tillbaka och välja en annan fil; sökvägen börjar då i mappen där du valde filen.
<!-- /hjälp -->

cratchit skapar bokföringen i SIE-filens mapp och tar in hela filen. cratchit skapar ingen ny mapp, bara en `.cratchit`-mapp. Övriga filer i mappen och SIE-filen ligger kvar som de låg (se [Dina filer](#dina-filer)). Sedan går cratchit tillbaka till samlingsmappen, där den nya bokföringen nu finns i listan. (Med [Ny bokföring här](#ny-bokföring-här) hamnar bokföringen i stället i mappen du startade i, var SIE-filen än ligger.)

Ligger filens räkenskapsår mer än 9 år före det aktuella står det en varning i *Bekräfta*: året kommer inte att synas i listan över räkenskapsår, eftersom cratchit för närvarande hanterar det aktuella året och högst 9 äldre. Bokföringen skapas ändå om du trycker `J`.

cratchit skapar ingenting om filen inte går att använda, till exempel om den inte är en SIE-fil, har fel eller saknar företagets namn eller räkenskapsår. cratchit skapar inte heller någon bokföring om SIE-filens mapp redan har en `.cratchit`-mapp, ligger inuti en annan bokföring, är din hemkatalog eller är samlingsmappen. Orsaken står då under prompten, och sökvägen står kvar så att du kan rätta den.

### Ny bokföring från uppgifter

<!-- hjälp: fran-uppgifter -->
Välj en uppgift med dess siffra och fyll i den. När namn, organisationsnummer och räkenskapsår är ifyllda skapar Enter bokföringen. Du kan ändra en uppgift så många gånger du vill innan dess.
<!-- /hjälp -->

Har du ingen SIE-fil skapar du bokföringen från uppgifter om företaget. Tryck `+`, `2` (*Ny*) och `2` (*Från uppgifter*). Där fyller du i ett formulär:

```text
  1 Namn: <ange organisationens namn>
  2 Organisationsnummer: <ange organisationens organisationsnummer>
  3 Räkenskapsår: <ange räkenskapsårets första månad>
  4 Första årets längd: 12 månader
Cratchit > Samlingsmapp > Lägg till > Ny > Från uppgifter> 
```

1. Tryck en uppgifts siffra. Raden är tom om uppgiften inte är ifylld, och visar det du skrev om den är det.
2. Skriv uppgiften och tryck Enter. Går den att använda kommer du tillbaka till formuläret, där den nu står. Går den inte att använda visar cratchit varför, och texten står kvar så att du kan rätta den.
3. Vill du inte ändra något trycker du `-` innan du har ändrat raden. Har du börjat ändra tömmer Esc raden först. Uppgiften är då som den var.
4. När namn, organisationsnummer och räkenskapsår är ifyllda trycker du Enter i formuläret. cratchit skapar då bokföringen och går tillbaka till samlingsmappen, där den finns i listan.

```text
Tryck Enter för att skapa bokföringen i /Users/anna/Documents/cratchit/Exempel AB.
Bokföringen får ingen kontoplan än.
  1 Namn: Exempel AB
  2 Organisationsnummer: 556677-8899
  3 Räkenskapsår: 2026-05-01 – 2027-04-30
  4 Första årets längd: 12 månader
Cratchit > Samlingsmapp > Lägg till > Ny > Från uppgifter> 
```

Bokföringen får en mapp med företagets namn i samlingsmappen. Den får än så länge ingen kontoplan (se [Kommer senare](#kommer-senare)).

<!-- hjälp: uppgifter-namn -->
Skriv organisationens namn och tryck Enter. Bokföringen får en mapp med det namnet i samlingsmappen, så det får inte redan finnas en mapp med samma namn där.
<!-- /hjälp -->

<!-- hjälp: uppgifter-orgnr -->
Skriv organisationsnumret och tryck Enter, med eller utan bindestreck, till exempel 556677-8899. cratchit kontrollerar att kontrollsiffran stämmer.
<!-- /hjälp -->

<!-- hjälp: uppgifter-rakenskapsar -->
Skriv räkenskapsårets första månad och tryck Enter, till exempel 2026-05. Räkenskapsåret börjar den första dagen i den månaden. cratchit hanterar det aktuella räkenskapsåret och högst 9 äldre, så ett tidigare första år tar cratchit inte emot.
<!-- /hjälp -->

<!-- hjälp: uppgifter-langd -->
Skriv hur många månader det första räkenskapsåret är och tryck Enter, 1 till 18. Oftast är det 12, men ett nystartat företags första räkenskapsår kan vara kortare eller längre, högst 18 månader.
<!-- /hjälp -->

Ett nystartat företag kan ha ett första räkenskapsår som är kortare eller längre än tolv månader. Startar företaget till exempel i maj och ska ha kalenderår, blir det första räkenskapsåret 8 månader: maj till december.

### Ny bokföring här

<!-- hjälp: ny-bokforing-har -->
Bokföringen skapas i mappen du startade cratchit i. Välj `1` för att skapa den från en SIE-fil, eller `2` från namn, organisationsnummer och räkenskapsår. Andra filer i mappen rör cratchit inte.
<!-- /hjälp -->

Vill du ha bokföringen i en mapp som redan finns, till exempel företagets mapp med avtal och fakturor, startar du cratchit i den mappen:

```sh
cd ~/Företaget
cratchit
```

Har mappen ingen bokföring finns valet `1 Ny bokföring här`. Det har samma två val som [Ny bokföring](#ny-bokföring) i samlingsmappen:

```text
Bokföringen hamnar i /Users/anna/Företaget.
  1 Från SIE
  2 Från uppgifter
Cratchit > Ny bokföring här> 
```

* `1 Från SIE` fungerar som [Ny bokföring från en SIE-fil](#ny-bokföring-från-en-sie-fil), men sökvägen börjar i mappen du startade i, och bokföringen skapas i den mappen, inte i SIE-filens. SIE-filen kan ligga var som helst, till exempel i en undermapp `SIE-filer`, och ligger kvar där den låg.
* `2 Från uppgifter` är formuläret i [Ny bokföring från uppgifter](#ny-bokföring-från-uppgifter). Namnet är bara organisationens namn, inget mappnamn.

Bokföringen hamnar i mappen du startade i, i en `.cratchit`-mapp. När bokföringen är skapad hamnar du i samlingsmappen, som kommer ihåg bokföringen och listar den:

```text
cratchit skapade bokföringen The ITfied AB.
/Users/anna/Documents/cratchit
    Namn           Org.nr       Skapad      Mapp
  1 The ITfied AB  556782-8172  2026-11-15  ~/Företaget
  + Lägg till bokföring
Cratchit > Samlingsmapp> 
```

Går du tillbaka utan att skapa något kommer du till början som vanligt. Nästa gång du startar cratchit i mappen öppnas bokföringen direkt (se [Starta i en bokföring](#starta-i-en-bokföring)).

cratchit skapar ingen bokföring i en mapp som redan har en `.cratchit`-mapp, i din hemkatalog, i samlingsmappen eller i en mapp som ligger inuti en annan bokföring. Då leder `1` till *Fel*, som säger varför.

<!-- hjälp: har-namn -->
Skriv organisationens namn och tryck Enter, till exempel The ITfied AB. Namnet är bara namnet; bokföringen hamnar i mappen du startade cratchit i.
<!-- /hjälp -->

## Dina filer

cratchit verkar utan att märkas. cratchit flyttar, döper om, ändrar och tar aldrig bort dina filer. cratchit skriver i sina egna `.cratchit`-mappar, och skapar synliga filer och mappar bara när du ber om just det, till exempel en export till SIE. En ny bokföring från SIE får bara en `.cratchit`-mapp, i SIE-filens mapp eller i den mapp du startade i.

När du skapar en bokföring från en SIE-fil, importerar en eller läser ett kontoutdrag, ligger filen kvar där den låg. cratchit för i stället en *förteckning* över de filer bokföringen har fått något från: vad filen hette, var den låg, när den lästes in och vad den gav. Förteckningen känner igen en fil på dess innehåll, så den vet vilken fil det var även om du senare flyttar filen eller byter namn på den.

Förteckningen ligger i bokföringens `.cratchit`-mapp, en för hela bokföringen och en för varje räkenskapsår. cratchit visar den inte än.

Har en bokföring en mapp `importerade_sie_filer` från en tidigare version av cratchit, rör cratchit inte den. Du kan flytta tillbaka filerna eller ta bort mappen när du vill.

## Om bokföringen har blivit fel

Börja alltid med att hämta en färsk SIE-fil för hela året från programmet som har rätt bokföring.

**Bokföringen stämmer inte med det andra programmet**, till exempel därför att en import inte går att göra, eller därför att fel SIE-fil är importerad i ett år. Gör steg 2 i [Bygga upp bokföringen från SIE-filer](#bygga-upp-bokföringen-från-sie-filer) för de år som är fel. Går importen inte att göra ersätter du året med filens innehåll.

**En SIE-fil har en lucka i en serie.** cratchit tar inte emot den, varken som import eller ersättning. Hämta en ny export från programmet. Tab-listan visar serierna och räkenskapsårets status för varje fil, så att du kan välja en som är hel.

**Ett nytt räkenskapsår ansluter inte till bokföringen.** Importera först åren emellan, ett i taget, så att bokföringen inte får några luckor.

**En testbokföring ska börja om från sitt frö.** Ta bort `.cratchit`-mappen i testmappen, starta cratchit där och välj `1` (*Ny bokföring här*) och `1` (*Från SIE*). Samlingsmappen behåller en rad för den gamla, som du tar bort med *Glöm*. Gör aldrig så med din riktiga bokföring.

## Kommer senare

* Valen i Årsflöde: utkast som du skapar, ändrar och bokför, arbete med årets verifikationer, momsrapport, årsredovisning, årsstämma, rapport till Bolagsverket och deklaration till Skatteverket.
* Avstämning mot kontoutdrag i bokslutet, till exempel 1920 mot plusgirot, 1930 mot sparkontot och 1630 mot skattekontot.

* En kontoplan för en ny bokföring från uppgifter. cratchit kommer att erbjuda BAS-kontoplanen.
* Import av en SIE-fil som tar bort eller byter ut verifikationer som redan finns i bokföringen, eller ändrar deras datum eller text.

## Ordlista

| Ord | Betyder |
|---|---|
| bokföring | Bokföringen för en juridisk person, i en egen mapp |
| samlingsmapp | Där cratchit håller reda på dina bokföringar |
| sökväg | Var en mapp ligger på datorn, till exempel `/Users/anna/Documents` |
| prompt | Raden där cratchit väntar på din tangent eller text |
| organisationsnummer | Företagets nummer hos Bolagsverket, tio siffror, till exempel 556677-8899 |
| kontoplan | Listan över bokföringens konton, till exempel BAS-kontoplanen |
| SIE-fil | En fil med bokföring i ett standardformat, som bokföringsprogram använder för att lämna över bokföring till varandra |
| räkenskapsår | Den period, oftast tolv månader, som bokföringen räknas och avslutas för |
| verifikation | En bokförd affärshändelse, med serie och nummer, till exempel `A12` |
| utkast | En affärshändelse som ännu inte är bokförd. Allt i den kan ändras |
