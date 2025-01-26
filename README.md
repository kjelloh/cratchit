# Cratchit
An SIE based book keeping helper console in modern C++. SIE is a File Encoding format for exchange of book keeping data between applications.
The name Cratchit is a reference to Bob Cratchit that is the poor clerk of Scrooge in Charles Dickens novel "A Christmas Carol".

## License
This project is licensed under the [Creative Commons Attribution-ShareAlike 4.0 International License (CC BY-SA 4.0)](https://creativecommons.org/licenses/by-sa/4.0/).

## version 0.6

* Now uses BY-SA 4.0 license.
* Now comes with Cmake support
* Now comes with conan package support 

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
