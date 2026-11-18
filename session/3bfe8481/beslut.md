# Decisions

Decisions that apply in this session. The goal is in [session.md](session.md), and progress is in [status.md](status.md).

## 2026-11-18

* **Names.**
  * *Playground cratchit* is the cratchit in plaiground `session/029b12e4/cratchit/`.
  * *Legacy cratchit* is this repo's cratchit, with its variants `zeroth`, `first` and `second`.
* **The work is done on the branch `merge-playground-cratchit`.** The user makes all commits.
* **Legacy cratchit stays macOS-only.** It won't be made cross-platform for now. Playground cratchit is the cross-platform one. Which cratchit becomes "the one" is left open.
* **Playground cratchit comes without its git history.** The history stays in the public plaiground repo. The copy is taken from plaiground commit `1a0eedb`.
* **The plaiground session notes stay in plaiground.** These are `session.md`, `beslut.md`, `status.md` and `_dev_home/`. This session keeps its own notes.
* **Playground cratchit becomes the variant `third` of the one `cratchit` executable**, next to `zeroth`, `first` and `second`.
  * Its sources go in `src/third/`, as `second` has `src/second/`. Its own layout stays as it is (`charset/`, `core/`, `pyroceph/`, `ui/`, `walk_pdf/`), and `src/third/` is the include root for that code.
  * Only its startup code changes: `main` becomes `third::main`, declared in `src/third/main.hpp`. The rest keeps its own namespaces (`cratchit::…`, `pyroceph::tui`, `walk_pdf`, `charset`).
  * Its tests go in `src/third/test/`. They stay a separate `cratchit_tests` program, run with `ctest`.
  * Its CMake goes in `src/third/CMakeLists.txt`, which is pulled in with `add_subdirectory`. Its `cmake/`, `docs/` and `resources/` go in `src/third/` too.
  * Its `zlib` dependency goes into the one top-level `conanfile.py`.
* **What gets compiled depends on the platform.** A CMake option, `CRATCHIT_WITH_LEGACY`, is on by default on macOS.
  * When it's on, `cratchit` has all four variants.
  * When it's off, only `third` is compiled in, and the legacy Conan packages aren't required.
  * The GitHub workflow builds with it off, on Windows, Linux and macOS.
* **`--third` is checked first in `main`.** This happens before legacy cratchit prints or logs anything, so playground cratchit's commands (`--walk-pdf`, `--sie-to-ar`, `--sie-to-ink2`, `--sie-to-vat`) still print nothing else.
* **Details of the wiring (step B):**
  * `--third` counts only as the first argument, and is removed before `third::main` gets the arguments. Legacy's own flags (`--zeroth`, …) are still read anywhere in the arguments, as before.
  * Conan's option `with_legacy` decides what is built, and is passed to CMake's `CRATCHIT_WITH_LEGACY`. CMake's own default (`${APPLE}`) applies only without Conan.
  * Third no longer builds as a program of its own from `src/third/`. The top level is the one way to build, with or without legacy.
* **`third` will be the default variant when the merge is done.** That means `cratchit` with no variant flag runs `third`, and legacy cratchit is reached with `--zeroth`, `--first` or `--second`.
