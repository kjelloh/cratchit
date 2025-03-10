﻿# Cratchit
An SIE based book keeping helper console in modern C++. SIE is a File Encoding format for exchange of book keeping data between applications.
The name Cratchit is a reference to Bob Cratchit that is the poor clerk of Scrooge in Charles Dickens novel "A Christmas Carol".

cratchit © 2025 by Kjell-Olov Högdal is licensed under Creative Commons Attribution 4.0 International. To view a copy of this license, visit https://creativecommons.org/licenses/by/4.0/

# Build

This project supports Cmake and Conan package manager.

If you have those installed - the following is the proposed build method.

```
conan install . --build=missing
cmake --preset conan-release
cmake --build build/Release
```

NOTE: Also see init_tool_chain.zsh if applicable to your platform

# Tree (bare git repository)

```
.
├── CMakeLists.txt
├── Development_Journal.txt
├── LICENSE
├── README.md
├── conanfile.py
├── init_tool_chain.zsh
├── sie_specs
│   └── doc
│       ├── SIE_filformat_ver_4B_080930.pdf
│       ├── SIE_filformat_ver_4B_ENGLISH.pdf
│       └── URL.txt
├── skv_specs
│   ├── README.TXT
│   ├── _Nyheter_from_beskattningsperiod_2024P4 (2)
│   │   ├── Filexempel_20XX.xls
│   │   ├── Fritext_FO.xls
│   │   ├── INK1_SKV2000-34-02-0024-02.xls
│   │   ├── INK2R_SKV2002-33-01-24-04.xls
│   │   ├── INK2S_SKV2002-33-01-24-04.xls
│   │   ├── INK2_SKV2002-33-01-24-04.xls
│   │   ├── INK3K_SKV2003-32-01-24-01.xls
│   │   ├── INK3R_SKV2003-32-02-24-07.xls
│   │   ├── INK3SUF_SKV2003-32-01-24-01.xls
│   │   ├── INK3SUS_SKV2003-32-01-24-01.xls
│   │   ├── INK3S_SKV2003-32-01-24-07.xls
│   │   ├── INK4DU_SKV2004-32-01-24-01.xls
│   │   ├── INK4R_SKV2004-32-02-24-01.xls
│   │   ├── INK4S_SKV2004-32-01-24-06.xls
│   │   ├── K10A_SKV2110a-19-02-24-01.xls
│   │   ├── K10_SKV2110-37-04-24-01.xls
│   │   ├── K11_SKV2111-03-01-18-02.xls
│   │   ├── K12_SKV2112-23-01-23-01.xls
│   │   ├── K13_SKV2113-08-02-15-01.xls
│   │   ├── K15A_SKV2115-15-01-17-01.xls
│   │   ├── K15B_SKV2116-17-01-17-01.xls
│   │   ├── K2_SKV2102-18-02-24-02.xls
│   │   ├── K4_SKV2104-27-02-19-01.xls
│   │   ├── K5_SKV2105-33-02-22-01.xls
│   │   ├── K6_SKV2106-33-02-22-01.xls
│   │   ├── K7_SKV2107-27-01-22-01.xls
│   │   ├── K8_SKV2108-12-01-22-01.xls
│   │   ├── K9_SKV2119-05-02-22-01.xls
│   │   ├── Landkoder_EES_2022.xls
│   │   ├── Lokala_Skattekontor_for_anstan.xls
│   │   ├── Medieleverantorsinfo_2022.xls
│   │   ├── N3A_SKV2153-34-03-24-01.xls
│   │   ├── N3B_SKV2167-23-01-18-01.xls
│   │   ├── N4_SKV2154-17-01-15-01.xls
│   │   ├── N7_SKV2170-11-01-19-02.xls
│   │   ├── N8_SKV2155-10-01-19-01.xls
│   │   ├── N9_SKV2158-04-06-23-03.xls
│   │   ├── NEA_SKV2164-10-02-17-01.xls
│   │   ├── NE_SKV2161-12-02-23-02.xls
│   │   ├── T1_SKV2050-34-01-24-02.xls
│   │   ├── T2_SKV2051-35-02-24-02.xls
│   │   ├── Vardeforrad_2025 (002).xls
│   │   ├── _Nyheter_from_beskattningsperiod_2024P4.pdf
│   │   └── _SKV269 blankettbilder med fÑltnamn 2024P4.pdf
│   └── fiscal_year_2022
│       ├── INK1_SKV2000-32-02-0022-02_SKV269.csv
│       ├── INK1_SKV2000-32-02-0022-02_SKV269.xls
│       ├── K10_SKV2110-35-04-22-01.csv
│       └── K10_SKV2110-35-04-22-01.xls
├── src
│   ├── cratchit.cpp
│   ├── cratchit.h
│   ├── main.cpp
│   └── zeroth
│       └── main.cpp
└── test_package
    └── conanfile.py
```

## version 0.6

* Refactored to conan package manager directory structure and scaffolding
* Changed License to CC BY 4.0 (Creative Commons Attribution 4.0 International)

## version 0.5

* Cratchit operates in states as defined by the following prompts
  * ":>"
  * ":tas>"
  * ":tas:accept>"
  * ":had>"
  * ":had:vat>"
  * ":had:je>"
  * ":had:aggregate:gross 0+or->"
  * ":had:aggregate:counter>"
  * ":had:aggregate:counter:gross>"
  * ":had:aggregate:counter:net+vat>"
  * ":had:je:1*at>"
  * ":had:je:ha>"
  * ":had:je:at>"
  * ":had:je:at:edit>"
  * "had:je:cat>"
  * ":skv>"
  * ":skv:tax_return:period>"
  * ":skv:tax_return>"
  * ":skv:to_tax>"
  * ":skv:ink2>"
  * ":skv:income?>"
  * ":skv:dividend?>"
  * ":contact>"
  * ":employee>"
  * ":??>"
* Now supports the following commands
    * "-version" or  "-v"
    * "-tas" (Tagged Amounts)
    * "-has_tag"
    * "-has_not_tag"
    * "-is_tagged"
    * "-is_not_tagged"
    * "-to_bas_account"
    * "-amount_trails"
    * "-aggregates"
    * "-to_hads" - Creates a had from each currently selected tagged amount
    * "-todo"
    * "-bas"
    * "-sie"
    * "-types"
    * "-balance"
    * "-had"
    * "-meta"
    * "-sru"
    * "-sru -bas"
    * "-gross"
    * "-net"
    * "-vat"
    * "-t2"
    * "-skv"
    * "-csv -had <path>"
    * "-csv -sru <path>"
    * "-"
    * "-omslutning"
    * "-plain_ar"
    * "-plain_ink2"
    * "-doc_ar"

## Version 0.4

* Now supports Heading + Date + Amount input for input of transactions to Journal
    * Parsing expects the last word to be a float amount.
    * Parsing expects the second last to be a date on the form YYYYMMDD
    * Parsing expects all other words to be part of the heading for the transaction event entered.
* Now supports command -sie <sie file path>
    * Parsing expects the -sie command to be followed by a path to an existing sie-file.
    * If found parsing expects the SIE-file to contain "#VER" entries with "#TRANS" subentries (See project document ./cratchit/src/sie/doc/SIE_filformat_ver_4B_ENGLISH.pdf)
    * Parsing will read in #VER entries into internal model of Journal entries.

* Removed CMake support (currently builds on macOS with g++11 in C++20 mode and I have so far failed to figure out a viable CMake configuration for cross platform build)
* Made project single file (main.cpp) again.
* Currently builds on macOS in VSCode with the following tasks.json configuration

```
        {
            "type": "cppbuild",
            "label": "macOS C/C++: g++-11 build active file",
            "command": "/usr/local/bin/g++-11",
            "args": [
                "-fdiagnostics-color=always",
                "-std=c++20",
                "-g",
                "${file}",
                "-o",
                "${workspaceFolder}/cratchit.out"
            ],
            "options": {
                "cwd": "${fileDirname}"
            },
            "problemMatcher": [
                "$gcc"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "detail": "compiler: /usr/local/bin/g++-11"
        }  
```
   
* Current macOS g++11 based build command is:
   
```
> g++-11 -fdiagnostics-color=always -std=c++20 -g cratchit/src/main.cpp -o cratchit/cratchit.out
```

## Version 0.3
* Refactored into "proper" Cmake folder structure (user creates a build dir under CMakeLists.txt and parallel to src directory)
```
.
├── CmakeLists.txt
├── LICENSE
├── README.md
├── cratchit.out
└── src
    ├── main.cpp
    ├── sie
    │   ├── SIE.cpp
    │   ├── SIE.h
    │   ├── doc
    │   │   ├── SIE_filformat_ver_4B_080930.pdf
    │   │   ├── SIE_filformat_ver_4B_ENGLISH.pdf
    │   │   └── URL.txt
    │   └── test
    │       ├── events1.txt
    │       └── sie1.se
    └── tris
        ├── Active.cpp
        ├── Active.h
        ├── BackEnd.cpp
        ├── BackEnd.h
        ├── Core.cpp
        ├── Core.h
        ├── FrontEnd.cpp
        └── FrontEnd.h

```
* Now builds in VSCode (macOS, g++-11,C++20)

## Version 0.2

* Made Frontend, Core Backend idiom into Tris library (and namespace)
* Added seed for parsing SIE-files using Boost Qi (No actual functionality yet)
* Added SIE format specifications
* Builds in Visual Studio 2015
* Builds with GCC in MSYS2 Console
* Fails with Clang in MSYS2 Console (Undefined reference to std::__once_call and std::once_callable)

### Todo: Fix Clang in MSYS2 Build Fail
```
kjell-olovhogdahl@KJELL-OLOVHA74E MINGW64 /C/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/build/build_msys2_clang
$ make
-- Boost version: 1.60.0
-- Boost version: 1.60.0
-- Found the following Boost libraries:
--   filesystem
--   system
-- Configuring done
-- Generating done
-- Build files have been written to: C:/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/build/build_msys2_clang
[ 14%] Linking CXX executable Cratchit.exe
CMakeFiles/Cratchit.dir/objects.a(BackEnd.cpp.obj):(.text[_ZSt9call_onceIMNSt13__future_base13_State_baseV2EFvPSt8functionIFSt10unique_ptrINS0_12_Result_baseENS4_8_DeleterEEvEEPbEJPS1_S9_SA_EEvRSt9once_flagOT_DpOT0_]+0xd2): undefined reference to `std::__once_callable'
CMakeFiles/Cratchit.dir/objects.a(BackEnd.cpp.obj):(.text[_ZSt9call_onceIMNSt13__future_base13_State_baseV2EFvPSt8functionIFSt10unique_ptrINS0_12_Result_baseENS4_8_DeleterEEvEEPbEJPS1_S9_SA_EEvRSt9once_flagOT_DpOT0_]+0xe0): undefined reference to `std::__once_call'
CMakeFiles/Cratchit.dir/objects.a(BackEnd.cpp.obj):(.text[_ZSt16__once_call_implISt12_Bind_simpleIFSt7_Mem_fnIMNSt13__future_base13_State_baseV2EFvPSt8functionIFSt10unique_ptrINS2_12_Result_baseENS6_8_DeleterEEvEEPbEEPS3_SB_SC_EEEvv]+0x1c): undefined reference to `std::__once_callable'
CMakeFiles/Cratchit.dir/objects.a(BackEnd.cpp.obj):(.text[_ZSt9call_onceIMSt6threadFvvEJSt17reference_wrapperIS0_EEEvRSt9once_flagOT_DpOT0_]+0x73): undefined reference to `std::__once_callable'
CMakeFiles/Cratchit.dir/objects.a(BackEnd.cpp.obj):(.text[_ZSt9call_onceIMSt6threadFvvEJSt17reference_wrapperIS0_EEEvRSt9once_flagOT_DpOT0_]+0x81): undefined reference to `std::__once_call'
CMakeFiles/Cratchit.dir/objects.a(BackEnd.cpp.obj):(.text[_ZSt16__once_call_implISt12_Bind_simpleIFSt7_Mem_fnIMSt6threadFvvEESt17reference_wrapperIS2_EEEEvv]+0x1c): undefined reference to `std::__once_callable'
clang++.exe: error: linker command failed with exit code 1 (use -v to see invocation)
make[2]: *** [CMakeFiles/Cratchit.dir/build.make:228: Cratchit.exe] Error 1
make[1]: *** [CMakeFiles/Makefile2:68: CMakeFiles/Cratchit.dir/all] Error 2
make: *** [Makefile:84: all] Error 2

kjell-olovhogdahl@KJELL-OLOVHA74E MINGW64 /C/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/build/build_msys2_clang
$ 

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

### Raw Project (git)
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
    └───src
        │   main.cpp
        │
        ├───sie
        │   │   SIE.cpp
        │   │   SIE.h
        │   │
        │   ├───doc         // SIE documentation
        │   │       SIE_filformat_ver_4B_080930.pdf
        │   │       SIE_filformat_ver_4B_ENGLISH.pdf
        │   │       URL.txt
        │   │
        │   └───test       // SIE test files
        │           sie1.se
        │
        └───tris                // Tris library (Frontend, Code, Backend idiom)
                Active.cpp
                Active.h
                BackEnd.cpp
                BackEnd.h
                Core.cpp
                Core.h
                FrontEnd.cpp
                FrontEnd.h


```

### Generated Build Environments
```
    │   vs_me.cmd
    │
    ├───build_vs
    │   │   ALL_BUILD.vcxproj
    │   │   ALL_BUILD.vcxproj.filters
    │   │   CMakeCache.txt
    │   │   cmake_install.cmake
    │   │   Cratchit.sln
    │   │   Cratchit.VC.db
    │   │   Cratchit.vcxproj
    │   │   Cratchit.vcxproj.filters
    │   │   ZERO_CHECK.vcxproj
    │   │   ZERO_CHECK.vcxproj.filters
```

### Bult targets
```
    ├───build_vs
    │   │   ...
    │   │
    │   ├───Debug
    │   │       Cratchit.exe
    │   │       Cratchit.ilk
    │   │       Cratchit.pdb
```
