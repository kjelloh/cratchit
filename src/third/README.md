# cratchit (variant third)

A Swedish bookkeeping app for the terminal.

This is cratchit's variant *third*, next to *zeroth*, *first* and *second* in this repo. It comes from plaiground `session/029b12e4/cratchit/` (commit `1a0eedb`), without its git history, and it is the cross-platform cratchit. For now it still builds as a program of its own from this folder. The merge is tracked in [session/3bfe8481](../../session/3bfe8481/status.md).

* The user interface is in Swedish. cratchit is made for Swedish bookkeeping (SIE files, *bokslut* — year-end closing — and *årsredovisning* — annual report).
* The code and the developer documentation are in English.
* Early work in progress: so far cratchit opens its framework and its settings folder and lets you navigate a small demo menu.

## Requirements

* CMake 3.25 or later
* [Conan 2](https://conan.io) (and Python, which Conan runs on)
* A C++23 compiler: Apple clang (Xcode), GCC 14 or MSVC (Visual Studio 2022)

Conan needs a profile once per machine: `conan profile detect`.

## Build and install

### Step by step

From this folder (`src/third/`):

```sh
# 1. Fetch the dependencies for a Release build and generate the CMake setup.
#    Compiles nothing of cratchit. Only needed again when dependencies or settings change.
conan install . --build=missing -s build_type=Release -s compiler.cppstd=23

# 2. Configure: read CMakeLists.txt and write the build files to build/Release/.
#    Rarely needed again; step 3 reconfigures by itself when CMakeLists.txt changes.
cmake --preset conan-release

# 3. Compile and link the libraries, cratchit and the tests. Only what changed is rebuilt.
cmake --build --preset conan-release

# 4. Run the tests (optional, but a good check before installing).
ctest --preset conan-release

# 5. Copy the program and its framework to <prefix>/bin for the whole computer:
#    /usr/local/bin/cratchit and /usr/local/bin/.cratchit/. Builds nothing.
sudo cmake --install build/Release
```

cratchit is installed for the whole computer, in CMake's default place for the platform (`CMAKE_INSTALL_PREFIX`): `/usr/local` on macOS and Linux, `C:\Program Files\cratchit` on Windows. That needs `sudo`, or an administrator terminal on Windows. `/usr/local/bin` is in `PATH` already on macOS and most Linux systems. Your own files stay your own: `~/.cratchit` and the workspace are per user.

To install somewhere else, add `--prefix <dir>`; the program then goes in `<dir>/bin`, which must be in your `PATH`. After a code change, run steps 3 to 5 again; otherwise the old program stays installed.

The program file and its framework folder (see [Where cratchit keeps its files](#where-cratchit-keeps-its-files)) are installed. The libraries are static and built into the program.

### Windows

Visual Studio is a multi-config generator, so steps 2 and 5 differ:

```sh
cmake --preset conan-default
cmake --install build --config Release
```

Run the install in a terminal started as administrator. `C:\Program Files\cratchit\bin` is not in `PATH`; add it yourself, once. GitHub Actions installs a Debug build with `--prefix` on Windows, Linux and macOS and runs it via `PATH`; the install without `--prefix` is only tried on macOS.

## Build for development

A Debug build sits next to the Release build, in `build/Debug/`, and does not affect it:

```sh
conan install . --build=missing -s build_type=Debug -s compiler.cppstd=23
cmake --preset conan-debug
cmake --build --preset conan-debug
ctest --preset conan-debug
```

It finds its framework in `build/Debug/.cratchit/`. To run it without touching your real `~/.cratchit`, point `HOME` to a folder of its own for that run:

```sh
mkdir -p ../_dev_home   # once
HOME="$PWD/../_dev_home" ./build/Debug/cratchit
```

cratchit's first lines tell which build runs, one per line: build time (local) and build type, program file and version, e.g. `Byggd: 14:32:05 2026-11-15 (Debug)`, `Från: …/build/Debug/cratchit`, `Version: 0.1.0`. The target `cratchit_build_stamp` writes `build/<type>/generated/build_stamp.cpp` (from `cmake/build_stamp.cmake`) on every build, so every build relinks cratchit, even when nothing else changed.

In Debug builds `LOG_DEVELOPMENT_TRACE` is compiled in; in Release builds it is left out (CMake option `CRATCHIT_DEVELOPMENT_TRACE`: `ON`, `OFF` or `DEBUG`, the default).

## Folder layout

| Folder or file | Holds | In Git |
| --- | --- | --- |
| `charset/` | Text encodings: Unicode, UTF-8, UTF-16, CP437, and `Transcoder` | yes |
| `pyroceph/tui/` | The terminal UI library `pyroceph`: keys, states and the prompt loop. Knows nothing of cratchit. | yes |
| `core/` | cratchit's own logic: settings folders, log, terminal encoding | yes |
| `ui/` | cratchit's own UI states (library `cratchit_ui`): the root menu, the workspace and its projects, and the test menu kept to remember how states and navigation work | yes |
| `main.cpp` | The program: starts up and runs the prompt loop from the root menu | yes |
| `test/` | The tests (GoogleTest) | yes |
| `docs/` | The user manual, in Swedish (`anvandarmanual.md`) | yes |
| `resources/` | The sources of the framework that is installed next to the program (see [Where cratchit keeps its files](#where-cratchit-keeps-its-files)). So far `framework.txt.in`; later the rules for Swedish bookkeeping. | yes |
| `CMakeLists.txt`, `conanfile.py` | The build | yes |
| `build/` | Everything built, per build type (`build/Debug/`, `build/Release/`). Safe to delete. | no |
| `CMakeUserPresets.json` | Generated by `conan install` | no |

## Where cratchit keeps its files

cratchit has two `.cratchit` folders:

| Folder | Holds | Written by |
| --- | --- | --- |
| *Framework*: `.cratchit` next to the program, e.g. `/usr/local/bin/.cratchit/` | What comes with the installation. Later the rules for Swedish bookkeeping, tax, *bokslut* and *årsredovisning*. | The installation only. cratchit only reads it. |
| *Runtime*: `.cratchit` in your home folder (`HOME`, on Windows `USERPROFILE`) | Your own files. Created on the first run. | cratchit |

| File | Holds |
| --- | --- |
| `<program folder>/.cratchit/framework.txt` | `version`, the cratchit version the framework belongs to |
| `<program folder>/.cratchit/anvandarmanual.md` | A copy of `docs/anvandarmanual.md`. `?` shows its help texts (see below). |
| `~/.cratchit/runtime.txt` | `created`, the day it was created |
| `~/.cratchit/log.txt` | The log, next to `runtime.txt` |

When the framework is missing, cratchit stops with an error: the installation is not whole.

The workspace (*samlingsmapp*) and each project (*bokföring*, the books of one juridical person) have a `.cratchit` folder of their own:

| File | Holds |
| --- | --- |
| `<Documents>/cratchit/.cratchit/workspace.txt` | `created`, and one `project.<uuid> = <folder>` per project the workspace remembers. A project folder may be anywhere. |
| `<project folder>/.cratchit/project.txt` | `created`, `uuid`, `name` and `orgnr`, in the format of take 3 (which may add more). |
| `<project folder>/.cratchit/books/series.csv` | The voucher series and their names (`series, name`), by series. SIE gives no names to series. Books without the file have Fortnox's series A–N (`default_series` in `core/books`); a new project gets them written, and each series its vouchers use is listed, unnamed when new. |
| `<project folder>/.cratchit/inventory.csv` | The inventory (*förteckning*) of the project: one row per use of a file, known by the SHA-256 of its content (`core/inventory`). Nothing writes here yet. |
| `<project folder>/.cratchit/books/<year start>/inventory.csv` | The fiscal year's inventory, overriding the project's for the same file. Each SIE file read into the books gets a row: `sha256, kind, name, path, size, read, use, generated, program, year_start, year_end, status, vouchers_added, vouchers_changed`, with `path` relative to the project folder (generic `/`), so the folder can move. |
| `<project folder>/.cratchit/books/books.txt` | `chart_type`, `company_id`, `address.*` and one `year.<start> = <end>` per fiscal year |
| `<project folder>/.cratchit/books/accounts.csv`, `dimensions.csv`, `objects.csv` | The chart of accounts, dimensions and objects |
| `<project folder>/.cratchit/books/<year start>/opening.csv`, `vouchers.csv`, `postings.csv` | A fiscal year's reported opening balances, vouchers and their postings |

The books are kept as take 3 keeps them (`core/books`, ported from take 3, as is the SIE reader `core/sie`). *Ny bokföring > Från SIE* plans the import of a whole SIE file into empty books (`plan_project_from_sie`), and on the user's confirmation creates `<the SIE file's folder>/<#FNAMN>/` (or, with *Ny bokföring här*, the working folder's `.cratchit/`) with `project.txt` and the books (`create_project_from_sie`). The SIE file is never moved, changed or copied: cratchit records it in the inventory (below). cratchit works without SIE files; SIE is only one way in: *Ny bokföring > Från uppgifter* creates `<workspace>/<name>/` from the name, organisation number (checked with its Luhn digit) and first fiscal year the user types (`core/details_project`, `ui/details_project_menu`), writing only `project.txt` and a `books.txt` holding the year. The books read a missing `accounts.csv`, `dimensions.csv` or `objects.csv`, and a year without a folder of its own, as empty. There is no chart of accounts for such books yet; a BAS chart in the framework is planned.

The workspace and its `.cratchit` are created the first time the user opens it. *Öppna befintlig* adds or replaces the project's line in `workspace.txt`, so a moved project folder is remembered at its new place (same `uuid`).

`<Documents>` is `$HOME/Documents` on macOS, `XDG_DOCUMENTS_DIR` (else `$HOME/Documents`) on Linux, and the Documents known folder on Windows (it follows a move to OneDrive). macOS may ask the user once whether the terminal may use Documents; when denied, cratchit cannot open the workspace.

The framework's sources are in `resources/`. CMake puts them together in the build folder (`framework.txt` gets the version from `project()` in `CMakeLists.txt`), copies them next to the built program on every build, and installs them next to the installed program. So a development build and an installed program find their framework the same way.

### How cratchit finds its own folder

To find the framework, cratchit needs the folder of the program file that is running. `argv[0]` cannot tell it: it is only what the starter (usually the shell) passed, and the shell passes what you typed. Started from `PATH`, `argv[0]` is the bare name `cratchit`; started via a link, it is the link's path; and the starter may pass anything at all.

So cratchit asks the platform (`program_folder()` in `core/dot_folder.cpp`):

| Platform | Asks |
| --- | --- |
| macOS | `_NSGetExecutablePath` (from dyld, the macOS loader) |
| Linux | the link `/proc/self/exe` |
| Windows | `GetModuleFileNameW` |

Then `std::filesystem::canonical` resolves links and `..` (on macOS and Linux like POSIX `realpath`, on Windows like `GetFinalPathNameByHandleW`). So a link to cratchit in another folder still leads to the framework next to the real program file. The folder of the resolved file is the program folder.

## Using cratchit

The user manual is in Swedish: [docs/anvandarmanual.md](docs/anvandarmanual.md). It describes what cratchit can do so far and is updated with every step that changes what the user sees.

`?` in the program shows the common keys. When the state has a help text, the `?` line says so, and `?` pressed again right after shows the help text from the manual. A third `?` shows the common keys again; any other key in between starts over. The manual is the only source of the help texts:

* A help text starts after a line `<!-- hjälp: <topic> -->` and ends at `<!-- /hjälp -->` (or the next comment or heading). Comments are invisible when the Markdown is shown. A help text may hold several paragraphs, lists and code blocks, and should fit on one screen.
* Each state names its topic (`State::help_topic()`); cratchit's topics are in `ui/help_topics.hpp`. A state without a topic shows only the common keys.
* The manual is copied into the framework at build time and read at start (`core/manual`). The program shows the help text as plain text, without `**`, `*`, backticks and link targets; code blocks show as they are, without their fences.
* The tests check that the manual has a help text for every topic, and no help text for an unknown topic.

## Commands without the user interface

With a command as its first argument cratchit runs it and ends, without the user interface: no startup lines, no log, no `~/.cratchit` or framework needed. Each command keeps the arguments, output, texts (English) and exit codes of the program it came from, and its manual is built into cratchit for `--help`. Any other argument is refused with exit code 2.

| Command | From | Code | Manual |
|---|---|---|---|
| `cratchit --walk-pdf <file> [<path>] [--depth N]`, `--text <file>` | walk_pdf (session 9fe36d28) | `walk_pdf/cli.cpp` | [docs/walk_pdf.md](docs/walk_pdf.md) |
| `cratchit --sie-to-ar [--trace] [--bas-year <year>] <file.se>` | cratchit_ar (session 5b35b2b9) | `core/ar/cli.cpp` | [docs/sie_to_ar.md](docs/sie_to_ar.md) |
| `cratchit --sie-to-ink2 [--trace] <file.se> <last year's BLANKETTER.SRU or folder>`, or `--previous-deficit <kronor>` | cratchit_ink2 (session 7d4475fb) | `core/ink2/cli.cpp` | [docs/sie_to_ink2.md](docs/sie_to_ink2.md) |
| `cratchit --sie-to-vat <file.se> [<last year's file.se>] [--period <YYYYQn>]` | the VAT return of take 3 (session 60cae9fe, no command line there) | `core/skv/vat_return/cli.cpp` | [docs/sie_to_vat.md](docs/sie_to_vat.md) |

`--sie-to-ar` and `--sie-to-ink2` write the folder `<orgnr>_<start>_<end>` in the working directory. Their mappings are `resources/ar/bas_to_ar_<year>.csv`, `resources/ink2/bas_2023_to_ink2_2026.csv` and `resources/ink2/skv_ink2_2026P4.csv`, built into the program. Both read the SIE file with cratchit's own SIE parser (`core/sie`), which is the one cratchit_ar and cratchit_ink2 had, under the charset names of today. INFO.SRU names the program as `cratchit <version>`. `--sie-to-vat` writes `<orgnr>_moms_<first>_<last>` with the report and voucher in Swedish (its terminal output is English); its tables are `resources/momsrapport/*.csv`.

## Trying another day

Some of cratchit depends on today's date, e.g. which fiscal year is the current one (key 0). To try another day, e.g. the turn of a fiscal year, set `CRATCHIT_TODAY`:

```sh
CRATCHIT_TODAY=2027-05-02 cratchit
```

cratchit says on stderr which day it takes as today. The tests run on a fixed day (`test/test_today.cpp`).

## Uninstall

Delete the program file and the framework next to it, e.g. `sudo rm -r /usr/local/bin/cratchit /usr/local/bin/.cratchit` (on Windows the folder `C:\Program Files\cratchit`). To remove your own files and logs as well, delete `~/.cratchit`.

## Licence

As for all of this repository: source code under GNU GPL v2, documentation under CC0 1.0. See [LICENCE.md](../../../LICENCE.md).
