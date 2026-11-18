cmake_minimum_required(VERSION 3.25)

# cratchit's variant third, pulled in by the top-level CMakeLists.txt with
# add_subdirectory. That file makes the program, cratchit, which links third_main
# below. The settings here (C++23, warnings as errors) apply to third's targets only.
project(cratchit_third VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The Swedish UI texts are UTF-8 in the source; MSVC must be told so
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

# Warnings are errors for all targets (CMake's own setting, so it applies to
# every compiler). Override once with: cmake --compile-no-warning-as-error ...
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

# Warning flags shared by all targets
add_library(cratchit_warnings INTERFACE)
if(MSVC)
  target_compile_options(cratchit_warnings INTERFACE /W4 /permissive-)
else()
  target_compile_options(cratchit_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

# Text transcoding: Unicode, UTF-8, UTF-16, CP437, WinAnsi, MacRoman, Adobe Standard and detection
# (from walk_pdf, session 9fe36d28),
# and Transcoder that pairs a decoder with an encoder
add_library(charset
  charset/adobe_standard.cpp
  charset/cp437.cpp
  charset/decoder.cpp
  charset/detect.cpp
  charset/macroman.cpp
  charset/transcoder.cpp
  charset/utf16.cpp
  charset/utf8.cpp
  charset/winansi.cpp
)
target_include_directories(charset PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(charset PRIVATE cratchit_warnings)

find_package(ZLIB REQUIRED)

# walk_pdf (session 9fe36d28): the text of a PDF, namespace walk_pdf. Its own
# sources unchanged in walk_pdf, its main.cpp as cli.cpp (cratchit --walk_pdf)
# and its manual as docs/walk_pdf.md; the charsets it needs are in charset.
# See walk_pdf/walk_pdf.hpp.
# docs/walk_pdf.md into the program as bytes, for cratchit --walk_pdf --help;
# CMake re-runs when it changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS docs/walk_pdf.md)
file(READ docs/walk_pdf.md walk_pdf_manual_hex HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," manual_bytes "${walk_pdf_manual_hex}")
configure_file(walk_pdf/manual_text.cpp.in generated/walk_pdf_manual_text.cpp @ONLY)

add_library(walk_pdf
  walk_pdf/cli.cpp
  walk_pdf/cmap.cpp
  walk_pdf/crypto/md5.cpp
  walk_pdf/crypto/rc4.cpp
  walk_pdf/document.cpp
  walk_pdf/filters.cpp
  walk_pdf/font.cpp
  walk_pdf/glyph_names.cpp
  walk_pdf/path.cpp
  walk_pdf/peek.cpp
  walk_pdf/plain_text.cpp
  walk_pdf/scanner.cpp
  walk_pdf/security.cpp
  walk_pdf/syntax.cpp
  walk_pdf/text_string.cpp
  walk_pdf/walk_pdf.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/walk_pdf_manual_text.cpp
)
target_include_directories(walk_pdf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/walk_pdf)
target_link_libraries(walk_pdf PUBLIC charset PRIVATE ZLIB::ZLIB cratchit_warnings)

# The terminal UI library pyroceph, namespace pyroceph::tui. Knows nothing of
# cratchit, so that it can become a library of its own.
add_library(pyroceph
  pyroceph/tui/error_state.cpp
  pyroceph/tui/input_buffer.cpp
  pyroceph/tui/key_parser.cpp
  pyroceph/tui/key_reader.cpp
  pyroceph/tui/pager.cpp
  pyroceph/tui/path_completion.cpp
  pyroceph/tui/prompt_loop.cpp
  pyroceph/tui/quit_state.cpp
  pyroceph/tui/raw_mode.cpp
  pyroceph/tui/state.cpp
  pyroceph/tui/terminal_width.cpp
)
target_include_directories(pyroceph PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pyroceph PUBLIC charset PRIVATE cratchit_warnings)

# Application logic (from take 3, session 60cae9fe)
# cratchit_ar (session 5b35b2b9): the årsredovisning of an SIE file, run as
# cratchit --sie-to-ar. Its manual, docs/sie_to_ar.md, and each BAS to ÅR
# mapping, resources/ar/bas_to_ar_<year>.csv, are built into the program as
# bytes, with a list of the mappings. A new mapping is a new CSV and its year here.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS docs/sie_to_ar.md)
file(READ docs/sie_to_ar.md ar_manual_hex HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," ar_manual_bytes "${ar_manual_hex}")
configure_file(core/ar/manual_text.cpp.in generated/ar_manual_text.cpp @ONLY)
set(BAS_TO_AR_YEARS 2024 2026)
set(layout_declarations "")
set(layout_entries "")
set(layout_sources "")
foreach(year IN LISTS BAS_TO_AR_YEARS)
  set(layout_file resources/ar/bas_to_ar_${year}.csv)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${layout_file})
  file(READ ${layout_file} layout_hex HEX)
  string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," layout_bytes "${layout_hex}")
  configure_file(core/ar/layout_text.cpp.in generated/layout_text_${year}.cpp @ONLY)
  string(APPEND layout_declarations "std::string_view layout_text_${year}();\n")
  string(APPEND layout_entries "      {${year}, \"${layout_file}\", layout_text_${year}()},\n")
  list(APPEND layout_sources ${CMAKE_CURRENT_BINARY_DIR}/generated/layout_text_${year}.cpp)
endforeach()
configure_file(core/ar/layouts.cpp.in generated/layouts.cpp @ONLY)

# cratchit_ink2 (session 7d4475fb): the inkomstdeklaration 2 of an SIE file, run
# as cratchit --sie-to-ink2. Its manual, docs/sie_to_ink2.md, the BAS to INK2
# mapping and SKV's fields of INK2, INK2R and INK2S are built into the program as bytes.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS docs/sie_to_ink2.md)
file(READ docs/sie_to_ink2.md ink2_manual_hex HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," ink2_manual_bytes "${ink2_manual_hex}")
configure_file(core/ink2/manual_text.cpp.in generated/ink2_manual_text.cpp @ONLY)
set(INK2_MAPPING_FILE resources/ink2/bas_2023_to_ink2_2026.csv)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${INK2_MAPPING_FILE})
file(READ ${INK2_MAPPING_FILE} mapping_hex HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," mapping_bytes "${mapping_hex}")
configure_file(core/ink2/mapping_text.cpp.in generated/mapping_text.cpp @ONLY)
set(SKV_FIELDS_FILE resources/ink2/skv_ink2_2026P4.csv)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${SKV_FIELDS_FILE})
file(READ ${SKV_FIELDS_FILE} skv_hex HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," skv_bytes "${skv_hex}")
configure_file(core/ink2/skv_text.cpp.in generated/skv_text.cpp @ONLY)

# The VAT return of take 3 (session 60cae9fe), run as cratchit --sie-to-vat: its
# manual, docs/sie_to_vat.md, and SKV's tables (resources/momsrapport) built into
# the program as bytes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS docs/sie_to_vat.md)
file(READ docs/sie_to_vat.md vat_manual_hex HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," vat_manual_bytes "${vat_manual_hex}")
configure_file(core/skv/vat_return/manual_text.cpp.in generated/vat_manual_text.cpp @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS resources/momsrapport/bas_to_momsrapport.csv)
file(READ resources/momsrapport/bas_to_momsrapport.csv account_boxes_hex HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," account_boxes_bytes "${account_boxes_hex}")
configure_file(core/skv/vat_return/account_boxes.cpp.in generated/vat_account_boxes.cpp @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS resources/momsrapport/momsrapport_box_to_eskd.csv)
file(READ resources/momsrapport/momsrapport_box_to_eskd.csv box_tags_hex HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," box_tags_bytes "${box_tags_hex}")
configure_file(core/skv/vat_return/box_tags.cpp.in generated/vat_box_tags.cpp @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS resources/momsrapport/momsrapport.csv)
file(READ resources/momsrapport/momsrapport.csv box_texts_hex HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," box_texts_bytes "${box_texts_hex}")
configure_file(core/skv/vat_return/box_texts.cpp.in generated/vat_box_texts.cpp @ONLY)

add_library(cratchit_core
  core/amount.cpp
  core/balances.cpp
  core/closing.cpp
  core/ink2/accounts.cpp
  core/ink2/arguments.cpp
  core/ink2/check.cpp
  core/ink2/cli.cpp
  core/ink2/form.cpp
  core/ink2/mapping.cpp
  core/ink2/output.cpp
  core/ink2/previous.cpp
  core/ink2/report.cpp
  core/ink2/skv.cpp
  core/ink2/sru_output.cpp
  core/ink2/tax.cpp
  core/ink2/trace.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/ink2_manual_text.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/mapping_text.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/skv_text.cpp
  core/sru.cpp
  core/skv/vat_return/box.cpp
  core/skv/vat_return/box_texts.cpp
  core/skv/vat_return/checks.cpp
  core/skv/vat_return/clearing.cpp
  core/skv/vat_return/cli.cpp
  core/skv/vat_return/files.cpp
  core/skv/vat_return/document.cpp
  core/skv/vat_return/eskd.cpp
  core/skv/vat_return/report.cpp
  core/skv/vat_return/review.cpp
  core/skv/vat_return/steps.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/vat_manual_text.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/vat_account_boxes.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/vat_box_tags.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/vat_box_texts.cpp
  core/ar/arguments.cpp
  core/ar/cli.cpp
  core/ar/disposition.cpp
  core/ar/equity.cpp
  core/ar/layout.cpp
  core/ar/notes.cpp
  core/ar/output.cpp
  core/ar/overview.cpp
  core/ar/requirements.cpp
  core/ar/statement.cpp
  core/ar/text.cpp
  core/ar/trace.cpp
  ${layout_sources}
  ${CMAKE_CURRENT_BINARY_DIR}/generated/layouts.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/ar_manual_text.cpp
  core/bas.cpp
  core/books/books.cpp
  core/books/export.cpp
  core/books/import.cpp
  core/books/report.cpp
  core/csv.cpp
  core/date.cpp
  core/details_project.cpp
  core/dot_folder.cpp
  core/event_draft.cpp
  core/same_event.cpp
  core/templates.cpp
  core/fiscal_year.cpp
  core/inventory.cpp
  core/log.cpp
  core/manual.cpp
  core/path_value.cpp
  core/pdf_documents.cpp
  core/project.cpp
  core/sie/check.cpp
  core/sie/document.cpp
  core/sie/file.cpp
  core/sie/lexer.cpp
  core/sie/writer.cpp
  core/sie_export.cpp
  core/sie_import.cpp
  core/sie_project.cpp
  core/sha256.cpp
  core/sie_summary.cpp
  core/statement/statement.cpp
  core/statement_drafts.cpp
  core/terminal_encoding.cpp
  core/version.cpp
)
target_include_directories(cratchit_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cratchit_core PUBLIC charset PRIVATE walk_pdf cratchit_warnings)
target_compile_definitions(cratchit_core PRIVATE CRATCHIT_VERSION="${PROJECT_VERSION}")
if(WIN32)
  # SHGetKnownFolderPath and CoTaskMemFree, for the Documents folder
  target_link_libraries(cratchit_core PRIVATE shell32 ole32)
endif()

# LOG_DEVELOPMENT_TRACE is compiled in when CRATCHIT_DEVELOPMENT_TRACE is ON,
# left out when OFF, and DEBUG (the default) compiles it into Debug builds only
set(CRATCHIT_DEVELOPMENT_TRACE DEBUG CACHE STRING "Compile in LOG_DEVELOPMENT_TRACE: ON, OFF or DEBUG")
set_property(CACHE CRATCHIT_DEVELOPMENT_TRACE PROPERTY STRINGS ON OFF DEBUG)
if(CRATCHIT_DEVELOPMENT_TRACE STREQUAL "DEBUG")
  set(development_trace "$<CONFIG:Debug>")
elseif(CRATCHIT_DEVELOPMENT_TRACE)
  set(development_trace 1)
else()
  set(development_trace 0)
endif()
target_compile_definitions(cratchit_core PUBLIC CRATCHIT_DEVELOPMENT_TRACE=${development_trace})

# cratchit's own UI states, built on pyroceph::tui::State. A library of its own
# so that the tests can reach the states.
add_library(cratchit_ui
  ui/add_project_menu.cpp
  ui/booked_menu.cpp
  ui/unbook_menu.cpp
  ui/vat_return_menu.cpp
  ui/command_hint_menu.cpp
  ui/design_insufficiency.cpp
  ui/details_project_menu.cpp
  ui/draft_menu.cpp
  ui/fiscal_year_menu.cpp
  ui/forget_project_menu.cpp
  ui/project_menu.cpp
  ui/reports_menu.cpp
  ui/root_menu.cpp
  ui/sie_import_menu.cpp
  ui/sie_project_menu.cpp
  ui/voucher_menu.cpp
  ui/start.cpp
  ui/test_menu.cpp
  ui/workspace_menu.cpp
  ui/year_flow_menu.cpp
  ui/year_view.cpp
)
target_include_directories(cratchit_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cratchit_ui PUBLIC pyroceph cratchit_core PRIVATE cratchit_warnings)

# The time and type of the build, shown when cratchit starts. The target runs on
# every build and writes build_stamp.cpp anew, so only that file is compiled and
# cratchit relinked. The file is written once at configure time too, so that it
# exists before the first build.
set(build_stamp_file ${CMAKE_CURRENT_BINARY_DIR}/generated/build_stamp.cpp)
if(NOT EXISTS ${build_stamp_file})
  execute_process(COMMAND ${CMAKE_COMMAND} -DOUT=${build_stamp_file} -DCONFIG=unknown
                  -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_stamp.cmake)
endif()
add_custom_target(cratchit_build_stamp
  COMMAND ${CMAKE_COMMAND} -DOUT=${build_stamp_file} -DCONFIG=$<CONFIG>
          -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_stamp.cmake
  BYPRODUCTS ${build_stamp_file}
  COMMENT "Stamping the build time into cratchit"
)

# third::main (main.hpp), which the program cratchit calls for variant third. A
# library of its own that links third's libraries PRIVATE, so that their include
# folders (e.g. walk_pdf/ with bare names like path.hpp) never reach the sources
# of the other variants in cratchit.
add_library(third_main STATIC
  main.cpp
  ${build_stamp_file}
)
add_dependencies(third_main cratchit_build_stamp)
target_link_libraries(third_main PRIVATE cratchit_core cratchit_ui pyroceph walk_pdf cratchit_warnings)

# The framework comes with the program: a .cratchit folder next to it, which
# cratchit only reads. resources/ holds its sources; framework.txt gets the
# project version. It is put together in the build folder at configure time.
set(framework_folder ${CMAKE_CURRENT_BINARY_DIR}/framework/.cratchit)
configure_file(resources/framework.txt.in ${framework_folder}/framework.txt @ONLY)
# The user manual comes with the framework, so that ? can show its help paragraphs
configure_file(docs/anvandarmanual.md ${framework_folder}/anvandarmanual.md COPYONLY)

# Copies the framework next to the built program (the target cratchit, made by the
# top-level CMakeLists.txt) on every build, so that a development build finds it
# just like an installed one
add_custom_target(cratchit_framework ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${framework_folder} $<TARGET_FILE_DIR:cratchit>/.cratchit
  COMMENT "Copying the framework next to cratchit"
)
add_dependencies(cratchit_framework cratchit)

# Installs the framework into <prefix>/bin, next to the program: bin/.cratchit/.
# The top-level CMakeLists.txt installs the program itself, bin/cratchit.
include(GNUInstallDirs)
install(DIRECTORY ${framework_folder} DESTINATION ${CMAKE_INSTALL_BINDIR})

# BUILD_TESTING comes from include(CTest) in the top-level CMakeLists.txt
if(BUILD_TESTING)
  add_subdirectory(test)
endif()
