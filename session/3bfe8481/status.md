# Status

The goal is in [session.md](session.md), and the decisions are in [beslut.md](beslut.md).

## Done

* 2026-11-18: Branch `merge-playground-cratchit` created and pushed, by the user.
* 2026-11-18: Stage 1, the survey of both codebases, is done. See [survey.md](survey.md). Summary:
  * Playground cratchit moves without changes: its CMake only uses paths relative to its own folder.
  * None of the CLI flags clash.
  * The two cratchits keep their runtime files in different places.
  * No relative header paths are the same in both codebases.

* 2026-11-18: Step A, the copy, is done. Playground cratchit is in `src/third/`, from plaiground `1a0eedb` (`git archive`, 407 files).
  * Its `src/` content is directly in `src/third/`. Its `test/`, `cmake/`, `docs/`, `resources/`, `CMakeLists.txt`, `conanfile.py`, `README.md`, `.gitignore` and `.gitattributes` are in `src/third/` too.
  * Changes from plaiground:
    * The `src/` prefix is removed from the paths in `CMakeLists.txt` and `test/CMakeLists.txt`.
    * The include roots are `${CMAKE_CURRENT_SOURCE_DIR}`.
    * The README paths are updated, and the README has a note at the top about variant third.
  * The legacy `.gitignore` has `!src/third/cmake/*.cmake`. Nothing in `src/third/` is ignored.
  * `.github/workflows/cratchit-third.yml` is the plaiground workflow with only the paths and names changed. It still builds `src/third/` on its own.
  * Tried locally on macOS (Debug, in `src/third/`):
    * `conan install`, configure and build work.
    * `ctest`: all 970 tests pass, including the skipped ones.
    * The program starts the UI with `HOME` in a temporary folder.
    * `--sie-to-ar --help` works.
    * `cmake --install --prefix <temp>` works, and the installed program, run via `PATH` from another folder, finds its framework.
  * Not yet run on GitHub: that happens on the first push.
  * Legacy cratchit is untouched.

## Next steps

* **B. Wire it in.**
  * `main` becomes `third::main` (`src/third/main.hpp`).
  * The top-level `CMakeLists.txt` gets `add_subdirectory(src/third)` and the option `CRATCHIT_WITH_LEGACY`.
  * `conanfile.py` gets `zlib`, and the legacy packages become conditional on the option.
  * `src/main.cpp` checks for `--third` first.
  * The workflow builds the top level with the option off.
* **C. Check and install.**
  * Check that no symbols overlap: build both, then compare the symbols they define with `nm`.
  * Install with `cmake --install`, putting `cratchit`, `.cratchit/` (third's framework) and `resources/` (second's font) in place.
  * `run.zsh` also copies `.cratchit/`.
* **D. Make `third` the default.**

## Good to know

* The namespaces `cratchit`, `charset` and `unicode` exist in both codebases. No names inside them clash so far, but that is unchecked until step C. Two classes or inline functions with the same name would not fail to link. The program would misbehave silently instead.
* This repo has file names with `?` in `issues/`, which Windows doesn't allow. The workflow uses a sparse checkout, and sets `core.protectNTFS false` on Windows, so those files are never written there.
* Playground cratchit's PDF command is `--walk-pdf` (not `--pdf-walk`).
* **For step B: keep third's include roots away from legacy sources.** The `walk_pdf` library has `src/third/walk_pdf/` as a PUBLIC include folder, so its bare header names (`path.hpp`, `document.hpp`, `font.hpp`, …) reach every target that links it directly. Today that is third's own `cratchit` executable. In the shared executable, third's `main` should therefore become a library of its own (for example `third_main`), which links third's libraries PRIVATE. Then the legacy sources never see these folders.
* The linker warns `ignoring duplicate libraries: '../libcratchit_core.a'` for `cratchit_tests`. This is harmless, and it comes with the plaiground CMake.
* Legacy cratchit runs its tests inside the program (`cratchit --test`), while third's tests are a separate `cratchit_tests` program.

## Sources

* Playground cratchit: plaiground `session/029b12e4/cratchit/`, at commit `1a0eedb` (2026-11-18). That session's notes (`session.md`, `beslut.md`, `status.md`) stay in plaiground.
* Legacy cratchit: this repo, branch `merge-playground-cratchit`, from `c3eedfe`.
