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

* 2026-11-18: Step A's workflow ran green on GitHub on all three platforms, as reported by the user.
* 2026-11-18: Step B, the wiring, is done. `cratchit` is one program with the variants zeroth, first, second and third.
  * `src/third/main.cpp` has `third::main` (declared in `src/third/main.hpp`). It got a `return 0;`, which the global `main` it once was didn't need.
  * `src/third/CMakeLists.txt` no longer makes a program:
    * It makes the library `third_main` (`main.cpp` and the build stamp), which links third's libraries PRIVATE.
    * It copies the framework next to `cratchit` and installs it.
    * Its project is renamed `cratchit_third`, and `include(CTest)` moved to the top level.
    * `src/third/conanfile.py` is removed. Third can no longer be built on its own from `src/third/`.
  * The top-level `CMakeLists.txt`:
    * It requires CMake 3.25.
    * It has the option `CRATCHIT_WITH_LEGACY` (default `${APPLE}`). When it's on, everything legacy (the `find_package` calls, sources, links, the font copy and `src/second/test`) is built. When it's off, `cratchit` is `src/main.cpp` only.
    * It always has `add_subdirectory(src/third)`, links `third_main`, and installs `cratchit` into `bin/` with GNUInstallDirs.
  * `conanfile.py` has the option `with_legacy`: default `True`, but `False` on anything but macOS. It's passed to CMake as `CRATCHIT_WITH_LEGACY`. `zlib` and `gtest` are always required, and the legacy packages only with `with_legacy`.
  * `src/main.cpp`:
    * The legacy `main` is now `legacy_main`, compiled only with `CRATCHIT_WITH_LEGACY`.
    * The new `main` checks the first argument for `--third` before anything else, and calls `third::main` with `--third` removed.
    * Without legacy, `main` always runs third.
    * With legacy and no `--third`, `main` runs `legacy_main`, as before (third is not the default yet, see step D).
  * The workflow builds the top level with `-o "&:with_legacy=False"`. Its sparse checkout is `CMakeLists.txt`, `conanfile.py`, `src/main.cpp` and `src/third/`. It also runs `cratchit --third` and `cratchit --third --sie-to-ar --help`.
  * `src/third/README.md` says to build from the root folder, and explains `with_legacy` and `--third`.
  * Tried locally on macOS (Debug):
    * **Without legacy** (a separate build folder, Conan with `with_legacy=False`): only `zlib` and `gtest` are pulled in.
      * Build works.
      * `ctest`: 970 tests pass.
      * No arguments or `--third` start the UI.
      * `--sie-to-ar` and `--sie-to-vat` work with and without `--third`.
      * `--zeroth` is rejected by third (exit code 2).
      * `cmake --install --prefix <temp>` installs `bin/cratchit` and `bin/.cratchit/`, and the installed program finds its framework.
    * **With legacy** (the usual `build/Debug`, `conan install` as in `init_tool_chain.zsh`):
      * Build works, with all four variants linked into one `cratchit`, without duplicate-symbol errors.
      * `./run.zsh --nop` works.
      * Legacy `--test`: 364 tests pass.
      * `ctest`: 971 tests pass.
      * `--third` starts the UI, and `--third --sie-to-ar --help` works.
  * Not yet run on GitHub: that happens on the next push.

## Next steps

* **C. Check and install.**
  * Check that no symbols overlap: build both, then compare the symbols they define with `nm`.
  * Install with `cmake --install`, putting `cratchit`, `.cratchit/` (third's framework) and `resources/` (second's font) in place.
  * `run.zsh` also copies `.cratchit/`. Until then, `./run.zsh --third` (in `workspace/`) runs third's commands, but its UI doesn't find its framework there.
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
