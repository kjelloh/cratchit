find_package(GTest REQUIRED)

add_executable(cratchit_tests
  vat/test_eskd.cpp
  vat/test_vat_cli.cpp
  vat/test_vat_return.cpp
  test_closing.cpp
  ink2/test_ink2_accounts.cpp
  ink2/test_ink2_arguments.cpp
  ink2/test_ink2_check.cpp
  ink2/test_ink2_filed.cpp
  ink2/test_ink2_form.cpp
  ink2/test_ink2_mapping.cpp
  ink2/test_ink2_output.cpp
  ink2/test_ink2_previous.cpp
  ink2/test_ink2_skv.cpp
  ink2/test_ink2_sru_output.cpp
  ink2/test_ink2_tax.cpp
  ink2/test_ink2_trace.cpp
  ink2/test_sru.cpp
  ar/test_ar_arguments.cpp
  ar/test_ar_closing.cpp
  ar/test_ar_equity.cpp
  ar/test_ar_layout.cpp
  ar/test_ar_notes.cpp
  ar/test_ar_output.cpp
  ar/test_ar_overview.cpp
  ar/test_ar_requirements.cpp
  ar/test_ar_statement.cpp
  ar/test_ar_trace.cpp
  test_amount.cpp
  test_books.cpp
  test_report.cpp
  test_reports_menu.cpp
  test_csv.cpp
  test_date.cpp
  test_import.cpp
  test_inventory.cpp
  test_sie_check.cpp
  test_sie_document.cpp
  test_sie_export.cpp
  test_sie_file.cpp
  test_sie_import.cpp
  test_sie_lexer.cpp
  test_sie_project.cpp
  test_sha256.cpp
  test_sie_summary.cpp
  test_statement.cpp
  test_statement_drafts.cpp
  test_event_draft.cpp
  test_same_event.cpp
  test_templates.cpp
  test_pdf_documents.cpp
  test_cp437.cpp
  test_decoder.cpp
  test_design_insufficiency.cpp
  test_detect.cpp
  test_details_project.cpp
  test_error_state.cpp
  test_fiscal_year.cpp
  test_dot_folder.cpp
  test_input_buffer.cpp
  test_key_parser.cpp
  test_key_reader.cpp
  test_log.cpp
  test_manual.cpp
  test_path_completion.cpp
  test_pager.cpp
  test_path_value.cpp
  test_project.cpp
  test_prompt_loop.cpp
  test_state.cpp
  test_terminal_encoding.cpp
  test_today.cpp
  test_transcoder.cpp
  test_ui.cpp
  test_year_view.cpp
  test_utf16.cpp
  test_utf8.cpp
  test_version.cpp
)
# The manual in the source tree, so that the tests check the one that is edited
target_compile_definitions(cratchit_tests PRIVATE CRATCHIT_MANUAL_FILE="${PROJECT_SOURCE_DIR}/docs/anvandarmanual.md")
target_link_libraries(cratchit_tests PRIVATE cratchit_core cratchit_ui pyroceph cratchit_warnings GTest::gtest_main)
target_include_directories(cratchit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# cratchit_ar's tests (session 5b35b2b9): those that read filed årsredovisningar's SIE
# files skip, as cratchit's tests use no files outside the repository (the folder
# below does not exist); the amounts typed in from them are in ar/expected_ar
target_compile_definitions(cratchit_tests PRIVATE
  CRATCHIT_AR_SIE_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}/ar/no_sie_files_in_the_repository"
  CRATCHIT_AR_EXPECTED_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}/ar/expected_ar")
# Likewise cratchit_ink2's tests (session 7d4475fb) that read the SIE files and filed
# returns (SRU) skip: the folders below do not exist
target_compile_definitions(cratchit_tests PRIVATE
  CRATCHIT_INK2_SIE_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}/ink2/no_sie_files_in_the_repository"
  CRATCHIT_INK2_SRU_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}/ink2/no_sru_files_in_the_repository")

include(GoogleTest)
gtest_discover_tests(cratchit_tests)

# walk_pdf's own tests (session 9fe36d28), those that need no PDF files of the user
add_executable(walk_pdf_tests
  walk_pdf/test_api.cpp
  walk_pdf/test_cli.cpp
  walk_pdf/test_crypto.cpp
  walk_pdf/test_document.cpp
  walk_pdf/test_filters.cpp
  walk_pdf/test_fonts.cpp
  walk_pdf/test_object_streams.cpp
  walk_pdf/test_path.cpp
  walk_pdf/test_plain_text.cpp
  walk_pdf/test_references.cpp
  walk_pdf/test_security.cpp
  walk_pdf/test_single_byte.cpp
  walk_pdf/test_syntax.cpp
  walk_pdf/test_text_string.cpp
)
target_include_directories(walk_pdf_tests PRIVATE ${PROJECT_SOURCE_DIR}/walk_pdf ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(walk_pdf_tests PRIVATE walk_pdf cratchit_warnings GTest::gtest_main ZLIB::ZLIB)
gtest_discover_tests(walk_pdf_tests)
