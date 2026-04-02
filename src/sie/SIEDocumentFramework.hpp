#pragma once

#include "SIEDocument.hpp"
#include "SIEArchive.hpp"
#include "persistent/in/maybe.hpp"
#include "persistent/in/encoding_aware_read.hpp" // persistent::in::CP437::istream
#include <expected>

using MaybeSIEInStream = persistent::in::text::MaybeIStream;
MaybeSIEInStream to_maybe_sie_istream(std::filesystem::path sie_file_path);

BAS::MDJournalEntry to_md_entry(sie::io::Ver const& ver);
std::optional<SIEDocument> sie_from_utf8_sv(std::string_view utf8_sv);
std::optional<SIEDocument> sie_from_cp437_stream(persistent::in::CP437::istream& cp437_in);

inline void for_each_md_journal_entry(SIEDocument const& sie_doc,auto& f) {
	for (auto const& [series,journal] : sie_doc.journals()) {
		for (auto const& [verno,aje] : journal) {
			f(BAS::MDJournalEntry{.meta ={.series=series,.verno=verno,.unposted_flag=sie_doc.is_unposted(series,verno)},.defacto=aje});
		}
	}
}

inline void for_each_md_journal_entry(SIEArchive const& sie_archive,auto& f) {
	for (auto const& [financial_year_key,sie_doc] : sie_archive) {
		for_each_md_journal_entry(sie_doc,f);
	}
}

