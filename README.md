# Cratchit
An SIE based book keeping helper console in modern C++. SIE is a File Encoding format for exchange of book keeping data between applications.
The name Cratchit is a reference to Bob Cratchit that is the poor clerk of Scrooge in Charles Dickens novel "A Christmas Carol".

## Operation

```
C:\Users\kjell-olovhogdahl\Documents\GitHub\cratchit\build\build_vs\Debug>Cratchit.exe

Cratchit>Enter "quit" to exit
Cratchit>Telia

        "#FNR"  "ITFIED"
        "#FNAMN"        "The ITfied AB"
        "#ORGNR"        "556782-8172"
        "#RAR"  "0"     "20150501"      "20160430"
        ""
        "#VER"  "4"     "42"    "20151116"      "FA407/Telia"   "20160711"
        "{"
        "#TRANS"        "2440"  "{}"    "-1330,00"      ""      "FA407/Telia"
        "#TRANS"        "2640"  "{}"    "265,94"        ""      "FA407/Telia"
        "#TRANS"        "6212"  "{}"    "1064,06"       ""      "FA407/Telia"
        "}"
Cratchit>quit

Done!

C:\Users\kjell-olovhogdahl\Documents\GitHub\cratchit\build\build_vs\Debug>
```

## Version 0.15

* Restructured source to build/src structure
* Added fcb (Front-end, Core, Back-end idiom)
* Adapted Console Front-end to be extended with Cratchit Console Fronted (console command execute and help overloads)
* Does not build on MSYS2 tool-chain

## Version 0.1

* Created initial version (empty code)
* Created Visual Studio and MSYS build environment generators

## Project
```

C:\Users\kjell-olovhogdahl\Documents\GitHub\cratchit>tree /F
Folder PATH listing
Volume serial number is D49B-BB89
C:.
│   .gitignore
│   LICENSE
│   README.md
│
└───build
    │   CmakeLists.txt      // CMake project
    │   msys2_me.sh         // MINGW shell script: Generate MSYS2 build environment sub-folder
    │   vs_me.cmd           // Windows shell: Generate Visual Studio build environment sub-folder
    │
    │
    └───src
        │   main.cpp
        │
        ├───fcb             // Fronend, Core, Backend idiom
        │       Active.cpp
        │       Active.h
        │       BackEnd.cpp
        │       BackEnd.h
        │       Core.cpp
        │       Core.h
        │       FrontEnd.cpp
        │       FrontEnd.h
        │
        └───sie            // Open SIE source
                SIE.cpp
                SIE.h
```