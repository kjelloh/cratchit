# Survey: playground cratchit and legacy cratchit

Stage 1 of [session.md](session.md). Read-only survey, 2026-11-18, on branch `merge-playground-cratchit`.

* **Playground cratchit** is `plaiground/session/029b12e4/cratchit/`.
* **Legacy cratchit** is this repo.

Decided so far:
* Legacy cratchit stays macOS-only. It won't be made cross-platform for now.
* Playground cratchit is copied without its git history. The history stays in the public plaiground repo.

## What is there

| | Playground cratchit | Legacy cratchit |
|---|---|---|
| Size (git-tracked files) | about 400 in `cratchit/`: `src/` 281, `test/` 107, `resources/` 8, `docs/` 5 | `src/` 268, plus `resources/`, `skv_specs/` and others |
| CMake | 3.25, `project(cratchit VERSION 0.1.0)` | 3.23, `project(cratchit)` |
| C++ | C++23, warnings as errors | C++23, warnings as errors |
| Conan dependencies | `zlib/1.3.1`, `gtest/1.15.0` (test only) | `cli11`, `sol2`, `ncurses`, `pugixml`, `icu`, `immer`, `spdlog`, `gtest/1.15.0`, `raylib` |
| Targets | libraries `charset`, `walk_pdf`, `pyroceph`, `cratchit_core`, `cratchit_ui` and `cratchit_warnings`; executables `cratchit` and `cratchit_tests` | executable `cratchit` (tests built in); `test_cratchit_lib` and `cratchit_test` from `src/second/test` |
| Tests | separate `cratchit_tests`, run with `ctest` | inside the program: `cratchit --test` or `--gtest_*` |
| CI | `.github/workflows/cratchit-029b12e4.yml` builds on Ubuntu, macOS and Windows: test, run, install, run the installed program | none (no `.github/`) |
| Install | `cmake --install` puts `bin/cratchit` and `bin/.cratchit/` (framework and manual) in place | `install(TARGETS cratchit …)` exists, but in practice `run.zsh` copies the program and `resources/` to `workspace/` |
| Runtime files | `~/.cratchit/` in the user's home folder, and `.cratchit/` next to the program | `workspace/` and `resources/` next to the program |

## CLI

| Playground cratchit | Legacy cratchit |
|---|---|
| `--walk-pdf`, `--sie-to-ar`, `--sie-to-ink2`, `--sie-to-vat`. Only the first argument decides. Any other argument is an error (exit code 2). No arguments starts the UI. | `--zeroth`, `--first`, `--second` (anywhere in the arguments), then `--nop`, `--version`, `--test`, `--gtest_*` as the first argument |

* **No flags clash.**
* The PDF flag is `--walk-pdf`, not `--pdf-walk` as [session.md](session.md) has it.
* Playground cratchit rejects every argument it doesn't know, so a merged single binary would need one shared dispatcher. That stage is deferred anyway: the two stay separate executables.

## Conflicts found

1. **Same executable name.** Both build an executable target called `cratchit` and install it as `bin/cratchit`. In one CMake tree the two target names clash, and two installs into the same prefix would overwrite each other. One of them must be renamed. That is a decision for you.
2. **Legacy `.gitignore` would hide a playground file.** The pattern `*.cmake` matches `cratchit/cmake/build_stamp.cmake`, which the playground build needs. A clean checkout would then fail to configure. This needs a `!` exception, or a `.gitignore` in the playground folder. Playground cratchit's own `.gitignore` (`build/`, `CMakeUserPresets.json`, `.cratchit`) can come along.
3. **Windows-invalid file names in this repo.** There are two files with `?` in `issues/`. The playground workflow already deals with this: it uses a sparse checkout of its own folder and sets `core.protectNTFS false` on Windows. That step must stay in the workflow.
4. **One top-level Conan setup would drag legacy dependencies into the cross-platform builds.** Examples are `ncurses`, `icu` and `raylib` on Windows. So in stage 2 each cratchit keeps its own `conanfile.py`. Stage 3 needs either OS-conditional requirements, or two separate Conan installs.
5. **Close test names.** Playground has `cratchit_tests` and legacy has `cratchit_test`. These don't clash, but are easy to confuse.

## What moves easily

* The playground `CMakeLists.txt` only uses paths relative to its own folder (`CMAKE_CURRENT_SOURCE_DIR` and `CMAKE_CURRENT_BINARY_DIR`). Nothing in `cratchit/` refers to `029b12e4` or `plaiground`. It can be copied to any folder, and later used with `add_subdirectory`, without changes.
* The two cratchits keep their runtime files in different places, so they don't get in each other's way.
* Both use `gtest/1.15.0` and C++23.

## The workflow, moved

Only paths change:
* the `paths:` filter
* the `sparse-checkout:` folder
* `working-directory:`
* `hashFiles(...)` for the Conan cache key
* the workflow's file name and `name:`

The steps stay the same.

## Not part of the program

The plaiground session folder also holds:
* the session notes, in Swedish: `session.md` (549 lines), `beslut.md` (decisions, 135 lines) and `status.md` (163 lines)
* `_dev_home/.cratchit/` (`runtime.txt`, `log.txt`)

Whether these come along is a decision for you. `beslut.md` holds the design decisions behind playground cratchit.
