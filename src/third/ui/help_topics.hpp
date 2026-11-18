#pragma once

#include "pyroceph/tui/error_state.hpp"
#include "pyroceph/tui/quit_state.hpp"

#include <array>
#include <string_view>

namespace cratchit::ui {

// The help topics of cratchit's states (see State::help_topic). Each one marks
// a help paragraph in the user manual, docs/anvandarmanual.md, as
// `<!-- hjälp: <topic> -->`.
namespace help_topic {
inline constexpr std::string_view root = "start";
inline constexpr std::string_view workspace = "samlingsmapp";
inline constexpr std::string_view add_project = "lagg-till";
inline constexpr std::string_view open_existing = "oppna-befintlig";
inline constexpr std::string_view new_project = "ny-bokforing";
inline constexpr std::string_view from_sie = "fran-sie";
inline constexpr std::string_view confirm_sie_project = "bekrafta-sie";
inline constexpr std::string_view from_details = "fran-uppgifter";
inline constexpr std::string_view details_name = "uppgifter-namn";
inline constexpr std::string_view details_orgnr = "uppgifter-orgnr";
inline constexpr std::string_view details_year = "uppgifter-rakenskapsar";
inline constexpr std::string_view details_months = "uppgifter-langd";
inline constexpr std::string_view here = "ny-bokforing-har";
inline constexpr std::string_view here_name = "har-namn";
inline constexpr std::string_view project = "bokforing";
inline constexpr std::string_view forget_project = "glom";
inline constexpr std::string_view fiscal_year = "rakenskapsar";
inline constexpr std::string_view year_flow = "arsflode";
inline constexpr std::string_view closing = "bokslut";
inline constexpr std::string_view vat_return = "momsrapport";
inline constexpr std::string_view vat_quarter = "momskvartal";
inline constexpr std::string_view vat_review = "granska-rutor";
inline constexpr std::string_view vat_box = "ruta";
inline constexpr std::string_view annual_report = "arsredovisning";
inline constexpr std::string_view tax_return = "deklaration";
inline constexpr std::string_view drafts = "utkast";
inline constexpr std::string_view read_statement = "las-kontoutdrag";
inline constexpr std::string_view statement_account = "kontoutdrag-konto";
inline constexpr std::string_view confirm_statement = "bekrafta-kontoutdrag";
inline constexpr std::string_view describe_event = "beskriv-handelse";
inline constexpr std::string_view same = "samma";
inline constexpr std::string_view same_pair = "samma-par";
inline constexpr std::string_view complete = "fullborda";
inline constexpr std::string_view post = "kontera";
inline constexpr std::string_view complete_event = "kontera-handelse";
inline constexpr std::string_view book = "bokfor-utkast";
inline constexpr std::string_view use_template = "anvand-mall";
inline constexpr std::string_view unbook = "gor-till-utkast";
inline constexpr std::string_view read_pdf = "las-pdf-underlag";
inline constexpr std::string_view pdf_list = "pdf-underlag";
inline constexpr std::string_view document = "underlag";
inline constexpr std::string_view interpret = "tolka";
inline constexpr std::string_view amount_choice = "belopp-otydligt";
inline constexpr std::string_view booked = "bokfort";
inline constexpr std::string_view voucher = "verifikation";
inline constexpr std::string_view draft = "utkastet";
inline constexpr std::string_view forget_draft = "glom-utkast";
inline constexpr std::string_view event = "handelse";
inline constexpr std::string_view postings = "konteringar";
inline constexpr std::string_view posting = "kontering";
inline constexpr std::string_view locked = "last";
inline constexpr std::string_view import_sie = "importera-sie";
inline constexpr std::string_view confirm_import = "bekrafta-import";
inline constexpr std::string_view replace_sie = "ersatt-sie";
inline constexpr std::string_view export_sie = "exportera-sie";
inline constexpr std::string_view sie_difference = "skillnad-sie";
inline constexpr std::string_view sie_comparison = "jamforelse-sie";
inline constexpr std::string_view design_insufficiency = "inte-an";
inline constexpr std::string_view error = pyroceph::tui::error_help_topic;
inline constexpr std::string_view quit = pyroceph::tui::quit_help_topic;
} // namespace help_topic

// All of them, so that a test can check that the manual has each one
inline constexpr std::array all_help_topics{
    help_topic::root,    help_topic::workspace,           help_topic::add_project, help_topic::open_existing,
    help_topic::new_project, help_topic::from_sie, help_topic::confirm_sie_project, help_topic::from_details,
    help_topic::details_name, help_topic::details_orgnr, help_topic::details_year, help_topic::details_months, help_topic::here, help_topic::here_name, help_topic::project, help_topic::forget_project, help_topic::fiscal_year, help_topic::year_flow, help_topic::drafts, help_topic::read_statement, help_topic::statement_account, help_topic::confirm_statement, help_topic::describe_event, help_topic::same, help_topic::same_pair, help_topic::complete, help_topic::post, help_topic::complete_event, help_topic::book, help_topic::use_template, help_topic::unbook, help_topic::read_pdf, help_topic::pdf_list, help_topic::document, help_topic::interpret, help_topic::amount_choice, help_topic::booked, help_topic::voucher, help_topic::draft, help_topic::forget_draft, help_topic::event, help_topic::postings, help_topic::posting, help_topic::locked, help_topic::closing, help_topic::vat_return, help_topic::vat_quarter, help_topic::vat_review, help_topic::vat_box, help_topic::annual_report, help_topic::tax_return, help_topic::import_sie, help_topic::confirm_import, help_topic::replace_sie, help_topic::export_sie,
    help_topic::sie_difference, help_topic::sie_comparison, help_topic::design_insufficiency,
    help_topic::error, help_topic::quit,
};

} // namespace cratchit::ui
