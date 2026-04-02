#include "SIEArchive.hpp"

// ------------------------------------------
// SIEArchive

std::pair<bool,std::string> SIEArchive::remove(DatedJournalEntryMeta const& key) {
  std::pair<bool,std::string> result{false,std::format("Could not find entry to remove")};

  // TODO: Consider to refactor the whole SIE storage API to use a key:{series,verno}.
  //       Then we can just look up ('A',11) plain and simple.
  //       We can then define 'views' in date order, series order or what have you
  //       using key vectors?
  //       For now, I suppose I have to live with the mess below (indirection hell...) 

  for (auto& [year_id,sie_env] : this->m_map) {
    if (sie_env.fiscal_year().contains(key.m_date) == false) continue;
    for (auto& [series,journal] : sie_env.journals()) {
      if (series != key.m_jem.series) continue;

      // TODO: Consider to rely on 'journal' storing entries in verno order low-to-high?
      //       In that case we can just use the rbegin() to access the last element...
      //       For now, the code both searches and exhaust all elements to
      //       also find largest verno to check if remove is allowed.
      BAS::VerNo max_verno{0};
      auto found_iter = journal.end(); 
      for (auto iter = journal.begin();iter != journal.end();++iter) {
        max_verno = std::max(max_verno,iter->first);
        if (key.m_jem.verno.value() == iter->first) found_iter = iter;
      }
      if (found_iter != journal.end()) {
        // Found it
        if (key.m_jem.verno.value() == max_verno) {
          // Ok to remove last in series
          journal.erase(found_iter);
          result.first = true;
        }
        else {
          result.second = std::format(
            "Failed to remove {}{}. Only last verno:{} can be removed."
            ,key.m_jem.series
            ,key.m_jem.verno.value()
            ,max_verno);
        }
      }

    }
  }

  return result;
}

UpdateFromPostedResult SIEArchive::update_from_posted_and_staged_sie_docs(
    sie::RelativeYearKey year_id
  ,SIEDocument const& posted_doc
  ,SIEDocument const& staged_doc) {

  logger::scope_logger log_raii{
      logger::development_trace
    ,std::format(
        "update_posted_from_sie_env: year_id:{}, posted sie year:{} staged sie year:{}"
      ,year_id
      ,posted_doc.fiscal_year().to_string()
      ,staged_doc.fiscal_year().to_string())
  };

  UpdateFromPostedResult result{};

  auto const& [iter,was_inserted] = this->m_map.insert_or_assign(year_id,std::move(posted_doc));

  result = iter->second.stage_sie_(staged_doc); // insert_or_assign never fails (true=inserted, false = assigned)

  if (was_inserted) {
    logger::development_trace(
      "update_from_posted_and_staged_sie_docs: Update to *NEW* posted id: {}"
      ,year_id);
  }
  else {
    // Was assigned
    logger::development_trace(
      "update_from_posted_and_staged_sie_docs: Update to EXISTING posted id: {}"
      ,year_id);
  }

  return result;
}

MaybeSIEDocumentRef SIEArchive::at(sie::RelativeYearKey key) {
    auto it =this->m_map.find(key);
    if (it ==this->m_map.end()) return {};
    return MaybeSIEDocumentRef::from(it->second);
}

MaybeSIEDocumentRef SIEArchive::at(Date date) {
  auto iter = std::ranges::find_if(this->m_map,[date](auto const& entry){
    return entry.second.fiscal_year().contains(date);
  });
  if (iter == this->m_map.end()) return {};
  return MaybeSIEDocumentRef::from(iter->second);
}

BAS::MaybeJournalEntryRef SIEArchive::at(DatedJournalEntryMeta key) {
  return this->at(key.m_date)
    .and_then([&key](auto& sie_env){
      return sie_env.at(key);
    });
}

SIEDocument& SIEArchive::operator[](sie::RelativeYearKey key) {
  auto iter =this->m_map.find(key);
  if (iter !=this->m_map.end()) {return iter->second;}

  // No environment in map for key!
  static SIEDocument dummy{FiscalYear{Date{}.year(),Date{}.month()}};
  // TODO: Consider a design to not default construct SIEDocument as
  logger::design_insufficiency(
    "SIEArchive:operator['{}'] does not exist - Returns dummy / empty for fiscal year:{}"
    ,dummy.fiscal_year().to_string()
    ,key);
  return dummy;
}

SIEDocumentChangeResult SIEArchive::stage(BAS::MDJournalEntry const& mdje) {
  SIEDocumentChangeResult result{mdje,SIEDocumentChangeResult::Status::Undefined};

  // TODO: Refctor this 'mess' *sigh* (to many optionals...)

  if (this->m_map.contains("current")) {
    if (auto financial_year = (*this)["current"].financial_year_date_range()) {
      if (financial_year->contains(mdje.defacto.date)) {
        return (*this)["current"].stage_entry_(mdje);
      }
    }
  }

  if (this->m_map.contains("-1")) {
    if (auto financial_year = (*this)["-1"].financial_year_date_range()) {
      if (financial_year->contains(mdje.defacto.date)) {
        return (*this)["-1"].stage_entry_(mdje);
      }
    }
  }
  return result;
}

// private

std::expected<ActualYearKey, std::string> SIEArchive::to_actual_year_key(
    sie::RelativeYearKey relative_year_key
  ,FiscalYear current_fiscal_year) {

  // Old mechanism: use relative key as-is
  return relative_year_key;

  // New mechanism - Use Date with the value of first day of financial year as key
  // try {
  //     auto relative_fiscal_year_index = std::stoi(relative_year_key);
  //     if (relative_fiscal_year_index <= 0 && relative_fiscal_year_index >= -10) {
  //         return current_fiscal_year.to_relative_fiscal_year(relative_fiscal_year_index).start();
  //     } else {
  //         return std::unexpected(std::format(
  //             "Relative year index {} is out of bounds (>0 or < -10)",
  //             relative_fiscal_year_index
  //         ));
  //     }
  // } catch (...) {
  //     return std::unexpected(std::format(
  //         "Failed to interpret relative year index {}", relative_year_key
  //     ));
  // }
}



// SIEArchive
// ------------------------------------------
