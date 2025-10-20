#include "TaggedAmountFramework.hpp"
#include "../../logger/log.hpp"
#include <iostream> // ,std::cout
#include <numeric> // std::accumulate,
#include <sstream> // std::ostringstream, std::istringstream
#include <algorithm> // std::all_of,
#include <ranges> // std::ranges::subrange,ranges::find,
#include <fstream> // std::ifstream

// BEGIN class TaggedAmount

TaggedAmount::TaggedAmount(Date const &date, CentsAmount const &cents_amount, Tags &&tags)
    : m_date{date},m_cents_amount{cents_amount}, m_tags{tags} {}

std::string TaggedAmount::to_string(TaggedAmount::ValueId value_id) {
  std::ostringstream os{};
  os << std::setw(sizeof(std::size_t) * 2) << std::setfill('0') << std::hex
     << value_id << std::dec;
  return os.str();
}

bool TaggedAmount::operator==(TaggedAmount const &other) const {
  auto result =
      this->date() == other.date() and
      this->cents_amount() == other.cents_amount() and
      std::all_of(m_tags.begin(), m_tags.end(),
                  [&other](Tags::value_type const &entry) {
                    return ((entry.first.starts_with("_")) or
                            (other.tags().contains(entry.first) and
                             other.tags().at(entry.first) == entry.second));
                  });

  // std::cout << "\nTaggedAmountClass::operator== ";
  // if (result) std::cout << "TRUE"; else std::cout << "FALSE";
  return result;
}

// END class TaggedAmount

// TaggedAmount to Value Id
TaggedAmount::ValueId to_value_id(TaggedAmount const &ta) {
  return std::hash<TaggedAmount>{}(ta);
}

std::ostream& operator<<(std::ostream &os, TaggedAmount const &ta) {
  // os << TaggedAmount::to_string(to_value_id(ta));
  os << " " << ::to_string(ta.date());
  os << " " << ::to_string(to_units_and_cents(ta.cents_amount()));
  for (auto const &tag : ta.tags()) {
    os << "\n\t|--> \"" << tag.first << "=" << tag.second << "\"";
  }
  return os;
}

// Hex string to Value Id (for parsing tags that encodes references as valude Ids)
TaggedAmount::OptionalValueId to_value_id(std::string const& sid) {
  // std::cout << "\nto_value_id()" << std::flush;
  TaggedAmount::OptionalValueId result{};
  TaggedAmount::ValueId value_id{};
  std::istringstream is{sid};
  try {
    is >> std::hex >> value_id;
    result = value_id;
  } catch (...) {
    std::cout << "\nto_value_id(" << std::quoted(sid)
              << ") failed. General Exception caught." << std::flush;
  }
  return result;
}

// Hex listing string to Value Ids (for parsing tags that encodes references as valude Ids)
TaggedAmount::OptionalValueIds to_value_ids(Key::Path const &sids) {
  // std::cout << "\nto_value_ids()" << std::flush;
  TaggedAmount::OptionalValueIds result{};
  TaggedAmount::ValueIds value_ids{};
  for (auto const &sid : sids) {
    if (auto value_id = to_value_id(sid)) {
      // std::cout << "\n\tA valid instance id sid=" << std::quoted(sid);
      value_ids.push_back(*value_id);
    } else {
      std::cout
          << "\nDESIGN_INSUFFICIENCY: to_value_ids: Not a valid instance id string sid="
          << std::quoted(sid) << std::flush;
    }
  }
  if (value_ids.size() == sids.size()) {
    result = value_ids;
  } else {
    std::cout << "\nDESIGN_INSUFFICIENCY: to_value_ids(Key::Path const& "
              << sids.to_string() << ") Failed. Created" << value_ids.size()
              << " out of " << sids.size() << " possible.";
  }
  return result;
}

// BEGIN class DateOrderedTaggedAmountsContainer

TaggedAmounts DateOrderedTaggedAmountsContainer::tagged_amounts() {
  // Note: For now generate a container and return. This ensures
  //       this will work also when we refactor the Tagged Amounts CAS that allows / detects branching paths 
  return TaggedAmounts{m_date_ordered_tagged_amounts.begin(),m_date_ordered_tagged_amounts.end()};
}


std::size_t DateOrderedTaggedAmountsContainer::size() const { return m_date_ordered_tagged_amounts.size(); }
DateOrderedTaggedAmountsContainer::const_iterator DateOrderedTaggedAmountsContainer::begin() const { return m_date_ordered_tagged_amounts.begin(); }
DateOrderedTaggedAmountsContainer::const_iterator DateOrderedTaggedAmountsContainer::end() const { return m_date_ordered_tagged_amounts.end(); }

DateOrderedTaggedAmountsContainer::const_subrange DateOrderedTaggedAmountsContainer::in_date_range(zeroth::DateRange const &date_period) {
  auto first = std::find_if(this->begin(), this->end(),
                            [&date_period](auto const &ta) {
                              return (ta.date() >= date_period.begin());
                            });
  auto last = std::find_if(this->begin(), this->end(),
                           [&date_period](auto const &ta) {
                             return (ta.date() > date_period.end());
                           });
  return std::ranges::subrange(first, last);
}

OptionalTaggedAmount DateOrderedTaggedAmountsContainer::at(ValueId const &value_id) {
  std::cout << "\nDateOrderedTaggedAmountsContainer::at("
            << TaggedAmount::to_string(value_id) << ")" << std::flush;
  OptionalTaggedAmount result{};
  if (m_tagged_amount_cas_repository.contains(value_id)) {
    result = m_tagged_amount_cas_repository.at(value_id);
  } else {
    std::cout << "\nDateOrderedTaggedAmountsContainer::at could not find a "
                 "mapping to value_id="
              << TaggedAmount::to_string(value_id) << std::flush;
  }
  return result;
}

OptionalTaggedAmount DateOrderedTaggedAmountsContainer::operator[](ValueId const &value_id) {
  std::cout << "\nDateOrderedTaggedAmountsContainer::operator[]("
            << TaggedAmount::to_string(value_id) << ")" << std::flush;
  OptionalTaggedAmount result{};
  if (auto o_ptr = this->at(value_id)) {
    result = o_ptr;
  } else {
    std::cout << "\nDateOrderedTaggedAmountsContainer::operator[] could not "
                 "find a mapping to value_id="
              << TaggedAmount::to_string(value_id) << std::flush;
  }
  return result;
}

OptionalTaggedAmounts DateOrderedTaggedAmountsContainer::to_tagged_amounts(ValueIds const &value_ids) {
  std::cout << "\nDateOrderedTaggedAmountsContainer::to_tagged_amounts()"
            << std::flush;
  OptionalTaggedAmounts result{};
  TaggedAmounts tas{};
  for (auto const &value_id : value_ids) {
    if (auto o_ta = (*this)[value_id]) {
      tas.push_back(*o_ta);
    } else {
      std::cout << "\nDateOrderedTaggedAmountsContainer::to_tagged_amounts() "
                   "failed. No instance found for value_id="
                << TaggedAmount::to_string(value_id) << std::flush;
    }
  }
  if (tas.size() == value_ids.size()) {
    result = tas;
  } else {
    std::cout << "\nto_tagged_amounts() Failed. tas.size() = " << tas.size()
              << " IS NOT provided value_ids.size() = " << value_ids.size()
              << std::flush;
  }
  return result;
}

DateOrderedTaggedAmountsContainer&  DateOrderedTaggedAmountsContainer::clear() {
  m_tagged_amount_cas_repository.clear();
  m_date_ordered_tagged_amounts.clear();
  return *this;
}

DateOrderedTaggedAmountsContainer& DateOrderedTaggedAmountsContainer::reset(DateOrderedTaggedAmountsContainer const &other) {
  this->m_date_ordered_tagged_amounts = other.m_date_ordered_tagged_amounts;
  this->m_tagged_amount_cas_repository = other.m_tagged_amount_cas_repository;
  return *this;
}

void DateOrderedTaggedAmountsContainer::date_ordered_tagged_amounts_insert(TaggedAmount const& ta) {
  auto value_id = to_value_id(ta);
  if (m_tagged_amount_cas_repository.contains(value_id) == false) {
    if (false) {
      std::cout << "\nthis:" << this << " Inserted new " << ta;
    }
    // Find the last element with a date less than the date of ta
    auto prev = std::upper_bound(
        m_date_ordered_tagged_amounts.begin(),
        m_date_ordered_tagged_amounts.end(), ta,
        [](TaggedAmount const &ta1, TaggedAmount const &ta2) {
          return ta1.date() < ta2.date();
        });

    m_tagged_amount_cas_repository.cas_repository_insert({value_id, ta});
    m_date_ordered_tagged_amounts.insert(prev, ta); // place after all with date less than the one of ta
  } 
  else {
    // No op - ta already in container (and CAS)
  }
}

DateOrderedTaggedAmountsContainer& DateOrderedTaggedAmountsContainer::erase(ValueId const &value_id) {
  if (auto o_ptr = this->at(value_id)) {
    // m_tagged_amount_cas_repository.the_map().erase(value_id);
    m_tagged_amount_cas_repository.erase(value_id);    
    auto iter = std::ranges::find(m_date_ordered_tagged_amounts, *o_ptr);
    if (iter != m_date_ordered_tagged_amounts.end()) {
      m_date_ordered_tagged_amounts.erase(iter);
    } else {
      std::cout << "\nDESIGN INSUFFICIENCY: Failed to erase tagged amount in "
                   "map but not in date-ordered-vector, value_id "
                << value_id;
    }
  } else {
    std::cout
        << "nDESIGN INSUFFICIENCY: DateOrderedTaggedAmountsContainer::at "
           "failed to find value_id "
        << value_id;
  }
  return *this;
}

DateOrderedTaggedAmountsContainer& DateOrderedTaggedAmountsContainer::merge(DateOrderedTaggedAmountsContainer const &other) {
  // other.for_each([this](TaggedAmount const &ta) {
  //   // TODO 240217: Consider a way to ensure that SIE entries in SIE file has
  //   // preceedence (overwrite any existing tagged amounts reflecting the same
  //   // events) Hm...Maybe this is NOT the convenient place to do this?
  //   this->date_ordered_tagged_amounts_insert(ta);
  // });
  std::ranges::for_each(other,[this](TaggedAmount const &ta) {
    // TODO 240217: Consider a way to ensure that SIE entries in SIE file has
    // preceedence (overwrite any existing tagged amounts reflecting the same
    // events) Hm...Maybe this is NOT the convenient place to do this?
    this->date_ordered_tagged_amounts_insert(ta);
  });

  return *this;
}

DateOrderedTaggedAmountsContainer& DateOrderedTaggedAmountsContainer::merge(TaggedAmounts const &tas) {
  for (auto const &ta : tas)
    this->date_ordered_tagged_amounts_insert(ta);
  return *this;
}
DateOrderedTaggedAmountsContainer& DateOrderedTaggedAmountsContainer::reset(TaggedAmounts const &tas) {
  this->clear();
  // *this += tas;
  this->merge(tas);
  return *this;
}

// END class DateOrderedTaggedAmountsContainer

TaggedAmount::ValueId TaggedAmountHasher::operator()(TaggedAmount const& ta) const {
  return to_value_id(ta);
}

namespace first {


  TaggedAmountsCasRepository::MaybeValue date_ordered_prev(
     TaggedAmountsCasRepository::Value const& value
    ,TaggedAmountsCasRepository const& container) {
    TaggedAmountsCasRepository::MaybeValue result{};

    return result;
  }

  DateOrderedTaggedAmountsContainer::DateOrderedTaggedAmountsContainer() 
    : m_repo{date_ordered_prev} {
  }

  TaggedAmountsCasRepository::const_iterator DateOrderedTaggedAmountsContainer::begin() const {return m_repo.begin();}
  TaggedAmountsCasRepository::const_iterator DateOrderedTaggedAmountsContainer::end() const {return m_repo.end();}

}

namespace tas {
  TaggedAmounts to_bas_omslutning(DateOrderedTaggedAmountsContainer::const_subrange const& tas) {
    logger::cout_proxy << "\nto_bas_omslutning";
    TaggedAmounts result{};
    using BASBuckets = std::map<BAS::AccountNo, TaggedAmounts>;
    BASBuckets bas_buckets{};
    auto is_valid_bas_account_transaction = [](TaggedAmount const &ta) {
      if (ta.tags().contains("BAS") and
          !(BAS::to_account_no(ta.tags().at("BAS")))) {
        // Whine about invalid tagging of 'BAS' tag!
        // It is vital we do NOT have any badly tagged BAS account transactions
        // as this will screw up the saldo calculation!
        logger::cout_proxy << "\nDESIGN_INSUFFICIENCY: tas::to_bas_omslutning failed to "
                     "create a valid BAS account no from tag 'BAS' with value "
                  << std::quoted(ta.tags().at("BAS"));
        return false;
      } else
        return (     (ta.tags().contains("BAS")) 
                 and (BAS::to_account_no(ta.tags().at("BAS")))
                 and (ta.tags().contains("IB") == false)
                 and (    ((ta.tags().contains("type") == false)) 
                       or (     (ta.tags().contains("type") == true) 
                            and (ta.tags().at("type") != "saldo"))));
    };
    for (auto const &ta : tas) {
      if (is_valid_bas_account_transaction(ta)) {
        bas_buckets[*BAS::to_account_no(ta.tags().at("BAS"))].push_back(ta);
      }
    }
    for (auto const &[bas_account_no, tas] : bas_buckets) {
      Date period_end_date{};
      logger::cout_proxy << "\n" << std::dec << bas_account_no;
      auto cents_saldo = std::accumulate(
          tas.begin(), tas.end(), CentsAmount{0},
          [&period_end_date](auto acc, auto const &ta) {
            period_end_date =
                std::max(period_end_date,
                         ta.date()); // Ensure we keep the latest date. NOTE: We
                                     // expect they are in growing date order.
                                     // But just in case...
            acc += ta.cents_amount();
            logger::cout_proxy << "\n\t" << period_end_date << " "
                      << to_string(to_units_and_cents(ta.cents_amount()))
                      << " ackumulerat:" << to_string(to_units_and_cents(acc));
            return acc;
          });

      TaggedAmount saldo_ta{period_end_date, cents_saldo};
      saldo_ta.tags()["BAS"] = std::to_string(bas_account_no);
      saldo_ta.tags()["type"] = "saldo";
      result.push_back(saldo_ta);
    }
    std::ranges::sort(result,[](auto const& lhs,auto const& rhs){
      return (lhs.tag_value("BAS").value() < rhs.tag_value("BAS").value());
    });
    return result;
  }
} // namespace tas

namespace CSV {
  namespace NORDEA {

  OptionalTaggedAmount to_tagged_amount(FieldRow const& field_row, Table::Heading const& heading) {
    OptionalTaggedAmount result{};
    if (field_row.size() >= 10) {
      auto sDate = field_row[element::Bokforingsdag];
      if (auto date = to_date(sDate)) {
        auto sAmount = field_row[element::Belopp];
        if (auto amount = to_amount(sAmount)) {
          auto cents_amount = to_cents_amount(*amount);
          TaggedAmount ta{*date,cents_amount};
          ta.tags()["Account"] = "NORDEA";
          if (field_row[element::Namn].size() > 0) {
            ta.tags()["Text"] = field_row[element::Namn];
          }
          if (field_row[element::Avsandare].size() > 0) {
            ta.tags()["From"] = field_row[element::Avsandare];
          }
          if (field_row[element::Mottagare].size() > 0) {
            ta.tags()["To"] = field_row[element::Mottagare];
          }
          if (auto saldo = to_amount(field_row[element::Saldo])) {
            ta.tags()["Saldo"] = to_string(to_cents_amount(*saldo));
          }
          if (field_row[element::Meddelande].size() > 0) {
            ta.tags()["Message"] = field_row[element::Meddelande];
          }
          if (field_row[element::Egna_anteckningar].size() > 0) {
            ta.tags()["Notes"] = field_row[element::Egna_anteckningar];
          }
          // Tag with column names as defined by heading
          if (heading.size() > 0 and heading.size() == field_row.size()) {
            for (int i=0;i<heading.size();++i) {
              auto key = heading[i];
              auto value = field_row[i];
              if (ta.tags().contains(key) == false) {
                if (value.size() > 0) ta.tags()[key] = field_row[i]; // add column value tagged with column name
              }
              else {
                logger::cout_proxy << "\nCSV::NORDEA::to_tagged_amountWarning, to_tagged_amount - Skipped conflicting column name:" << std::quoted(key) << ". Already tagged with value:" << std::quoted(ta.tags()[key]);
              }
            }
          }
          else {
            logger::cout_proxy << "\nCSV::NORDEA::to_tagged_amountError, to_tagged_amount failed to tag amount with heading defined tags and values. No secure mapping from heading column count:" << heading.size() << " to entry column count:" << field_row.size();
          }

          result = ta;
        }
        else {
          logger::cout_proxy << "\nCSV::NORDEA::to_tagged_amountNot a valid NORDEA amount: " << std::quoted(sAmount); 
        }
      }
      else {
        if (sDate.size() > 0) logger::cout_proxy << "\nCSV::NORDEA::to_tagged_amountNot a valid date: " << std::quoted(sDate);
      }
    }
    return result;
  }

  } // namespace NORDEA

  namespace SKV {
    OptionalTaggedAmount to_tagged_amount(FieldRow const& field_row, Table::Heading const& heading) {
      OptionalTaggedAmount result{};
      if (field_row.size() == 5) {
        // NOTE: The SKV comma separated file is in fact a end-with-';' field file (so we get five separations where the file only contains four fields...)
        //       I.e., field index 0..3 contains values
        // NOTE! skv-files seems to be ISO_8859_1 encoded! (E.g., 'å' is ASCII 229 etc...)
        // Assume the client calling this function has already transcoded the text into internal character set and encoding (on macOS UNICODE in UTF-8)
        auto sDate = field_row[element::OptionalDate];
        if (auto date = to_date(sDate)) {
          auto sAmount = field_row[element::Belopp];
          if (auto amount = to_amount(sAmount)) {
            auto cents_amount = to_cents_amount(*amount);
            TaggedAmount ta{*date,cents_amount};
            ta.tags()["Account"] = "SKV";
            ta.tags()["Text"] = field_row[element::Text];

            // Tag with column names as defined by heading
            if (heading.size() > 0 and heading.size() == field_row.size()) {
              for (int i=0;i<heading.size();++i) {
                auto key = heading[i];
                if (ta.tags().contains(key) == false) {
                  // Note: The SKV file may contain both CR (0x0D) and LF (0x0A) even when downloaded to macOS that expects only CR.
                  // We can trim on the value to get rid of any residue LF
                  // TODO: Make reading of text from file robust against control characters that does not match the expectation of the runtime
                  auto value = tokenize::trim(field_row[i]);
                  if (value.size() > 0) ta.tags()[key] = field_row[i]; // add column value tagged with column name
                }
                else {
                  logger::cout_proxy << "\nCSV::SKV::to_tagged_amountWarning, to_tagged_amount - Skipped conflicting column name:" << std::quoted(key) << ". Already tagged with value:" << std::quoted(ta.tags()[key]);
                }
              }
            }
            else {
              logger::cout_proxy << "\nCSV::SKV::to_tagged_amountError, to_tagged_amount failed to tag amount with heading defined tags and values. No secure mapping from heading column count:" << heading.size() << " to entry column count:" << field_row.size();
            }

            result = ta;
          }
          else {
            logger::cout_proxy << "\nCSV::SKV::to_tagged_amountNot a valid amount: " << std::quoted(sAmount); 
          }
        }
        else {
          // It may be a saldo entry
          /*
              skv-file entry	";Ingående saldo 2022-06-05;625;0;"
              field_row				""  "Ingående saldo 2022-06-05" "625" "0" ""
              index:           0                           1     2   3   4
          */
          auto sOptionalZero = field_row[element::OptionalZero]; // index 3
          if (auto zero_amount = to_amount(sOptionalZero)) {
            // No date and the optional zero is present ==> Assume a Saldo entry
            // Pick the date from Text
            auto words = tokenize::splits(field_row[element::Text],' ');
            if (auto saldo_date = to_date(words.back())) {
              // Success
              auto sSaldo = field_row[element::Saldo];
              if (auto saldo = to_amount(sSaldo)) {
                auto cents_saldo = to_cents_amount(*saldo);
                TaggedAmount ta{*saldo_date,cents_saldo};
                ta.tags()["Account"] = "SKV";
                ta.tags()["Text"] = field_row[element::Text];
                ta.tags()["type"] = "saldo";
                // NOTE! skv-files seems to be ISO_8859_1 encoded! (E.g., 'å' is ASCII 229 etc...)
                // TODO: Re-encode into UTF-8 if/when we add parsing of text into tagged amount (See namespace charset::ISO_8859_1 ...)
                result = ta;
              }
              else {
                logger::cout_proxy << "\nCSV::SKV::to_tagged_amountNot a valid SKV Saldo: " << std::quoted(sSaldo); 
              }
            }
            else {
                logger::cout_proxy << "\nCSV::SKV::to_tagged_amountNot a valid SKV Saldo Date in entry: " << std::quoted(field_row[element::Text]); 
            }
          }
          if (sDate.size() > 0) logger::cout_proxy << "\nCSV::SKV::to_tagged_amountNot a valid SKV date: " << std::quoted(sDate);
        }
      }
      return result;
    } // to_tagged_amount
  } // SKV
} // CSV

namespace CSV {
  namespace project {

    ToTaggedAmountProjection make_tagged_amount_projection(
       HeadingId const& csv_heading_id
      ,CSV::TableHeading const& table_heading) {
      switch (csv_heading_id) {
        case HeadingId::Undefined: {
          return [table_heading](CSV::FieldRow const& field_row) -> OptionalTaggedAmount {
            return std::nullopt;
          };
        } break;
        case HeadingId::NORDEA: {
          return [table_heading](CSV::FieldRow const& field_row) -> OptionalTaggedAmount {
            return CSV::NORDEA::to_tagged_amount(field_row,table_heading);
          };
        } break;
        case HeadingId::SKV: {
          return [table_heading](CSV::FieldRow const& field_row) -> OptionalTaggedAmount {
            return CSV::SKV::to_tagged_amount(field_row,table_heading);
          };
        } break;
        case HeadingId::unknown: {
          return [table_heading](CSV::FieldRow const& field_row) -> OptionalTaggedAmount {
            return std::nullopt;
          };
        } break;
      }
    }

    OptionalDateOrderedTaggedAmounts to_dotas(
       CSV::project::HeadingId const& csv_heading_id
      ,CSV::OptionalTable const& maybe_csv_table) {
      OptionalDateOrderedTaggedAmounts result{};
      if (maybe_csv_table) {
        auto to_tagged_amount = make_tagged_amount_projection(csv_heading_id,maybe_csv_table->heading);
        DateOrderedTaggedAmountsContainer dotas{};
        for (auto const& field_row : maybe_csv_table->rows) {
          if (auto o_ta = to_tagged_amount(field_row)) {
            dotas.date_ordered_tagged_amounts_insert(*o_ta);
          }
          else {
            logger::cout_proxy << "\nCSV::project::to_dota: Sorry, Failed to create tagged amount from field_row " << std::quoted(to_string(field_row));
          }
        }
        result = dotas;
      }
      else {
        logger::development_trace("CSV::project::to_dota - Null table -> nullopt result");
      }
      return result;
    } // to_dota
  } // project
} // CSV

/**
* Return a list of tagged amounts if provided statement_file_path is to a file with amount values (e.g., a bank account csv statements file)
*/
OptionalDateOrderedTaggedAmounts to_dotas(std::filesystem::path const& statement_file_path) {
  if (true) {
    std::cout << "\nto_tagged_amounts(" << statement_file_path << ")";
  }
  OptionalDateOrderedTaggedAmounts result{};
  CSV::OptionalFieldRows field_rows{};
  std::ifstream ifs{statement_file_path};
  // NOTE: The mechanism implemented to apply correct decoding and parsing of different files is a mess!
  //       For one, The runtime on Mac uses UTF-8 through the console by default (which Windows and Unix may or may not do).
  //       Also, The NORDEA CSV-file as downloaded through Safary from NORDEA web bank is also UTF-8 encoded (although I am not sure it will be using another browser on another platform?).
  //       Finally, The SKV-file from Swedish Tax Agency web interface gets encoded in ISO8859-1 on Mac using Safari.
  //       For now the whole thing is a patch-work that may or may not continue to work on Mac and will very unlikelly work on Linux or Windows?
  //       TODO 20240527 - Try to refactor this into something more stable and cross-platform at some point in time?!
  if (statement_file_path.extension() == ".csv") {
    encoding::UTF8::istream utf8_in{ifs};
    field_rows = CSV::to_field_rows(utf8_in,';'); // Call to_field_rows overload for "UTF8" input (assuming a CSV-file is ISO8859-1 encoded, as NORDEA csv-file is)
  }
  else if (statement_file_path.extension() == ".skv") {
    encoding::ISO_8859_1::istream iso8859_in{ifs};
    field_rows = CSV::to_field_rows(iso8859_in,';'); // Call to_field_rows overload for "ISO8859-1" input (assuming a SKV-file is ISO8859-1 encoded)
  }
  if (field_rows) {
    // The file is some form of 'comma separated value' file using ';' as separators
    // NOTE: Both Nordea csv-files (with bank account transaction statements) and Swedish Tax Agency skv-files (with tax account transactions statements)
    // uses ';' as value separators
    if (field_rows->size() > 0) {
      auto csv_heading_id = CSV::project::to_csv_heading_id(field_rows->at(0));
      auto heading_projection = CSV::project::make_heading_projection(csv_heading_id);
      result =  CSV::project::to_dotas(csv_heading_id,CSV::to_table(field_rows,heading_projection));
    }
  }
  return result;
}


// String conversion
std::string to_string(TaggedAmount const& ta) {
  std::ostringstream oss;
  oss << ta;
  return oss.str();
}

// Environment conversions
Environment::Value to_environment_value(TaggedAmount const& ta) {
	Environment::Value ev{};
	ev["yyyymmdd_date"] = to_string(ta.date());
	ev["cents_amount"] = to_string(ta.cents_amount());
	for (auto const& entry : ta.tags()) {
		ev[entry.first] = entry.second;
	}
	return ev;
}

OptionalTaggedAmount to_tagged_amount(Environment::Value const& ev) {
	OptionalTaggedAmount result{};
	OptionalDate date{};
	OptionalCentsAmount cents_amount{};
	TaggedAmount::OptionalValueId value_id{};
	TaggedAmount::Tags tags{};
	for (auto const& entry : ev) {
		if (entry.first == "yyyymmdd_date") date = to_date(entry.second);
		else if (entry.first == "cents_amount") cents_amount = to_cents_amount(entry.second);
		else tags[entry.first] = entry.second;
	}
	if (date and cents_amount) {
    result = TaggedAmount{*date,*cents_amount,std::move(tags)};
	}
	return result;
}

auto ev_to_maybe_ta = [](Environment::Value const &ev) -> OptionalTaggedAmount {
  return to_tagged_amount(ev);
};
auto in_period = [](TaggedAmount const &ta, FiscalPeriod const &period) -> bool {
  return period.contains(ta.date());
};  
auto id_ev_pair_to_ev = [](Environment::IdValuePair const &id_ev_pair) {
  return id_ev_pair.second;
};

TaggedAmounts to_tagged_amounts(const Environment &env) {
  static constexpr auto section = "TaggedAmount";
  if (!env.contains(section)) {
    return {}; // No entries of this type
  }
  auto const &id_ev_pairs = env.at(section);
  auto result = id_ev_pairs 
    | std::views::transform(id_ev_pair_to_ev) 
    | std::views::transform(ev_to_maybe_ta) 
    | std::views::filter([](auto const &maybe_ta) { return maybe_ta.has_value(); }) 
    | std::views::transform([](auto const &maybe_ta) { return *maybe_ta; }) 
    | std::ranges::to<std::vector>();
  
  // Sort by date for consistent ordering
  std::sort(result.begin(), result.end(), [](const TaggedAmount& a, const TaggedAmount& b) {
    return a.date() < b.date();
  });
  return result;
}

// Environment -> TaggedAmounts (filtered by fiscal period)
TaggedAmounts to_period_tagged_amounts(FiscalPeriod period, const Environment &env) {
  static constexpr auto section = "TaggedAmount";
  if (!env.contains(section)) {
    return {}; // No entries of this type
  }
  auto const &id_ev_pairs = env.at(section);
  auto result = id_ev_pairs 
    | std::views::transform(id_ev_pair_to_ev) 
    | std::views::transform(ev_to_maybe_ta) 
    | std::views::filter([](auto const &maybe_ta) { return maybe_ta.has_value(); }) 
    | std::views::transform([](auto const &maybe_ta) { return *maybe_ta; }) 
    | std::views::filter([&](auto const &ta) { return in_period(ta, period); }) 
    | std::ranges::to<std::vector>();
  
  // Sort by date for consistent ordering
  std::sort(result.begin(), result.end(), [](const TaggedAmount& a, const TaggedAmount& b) {
    return a.date() < b.date();
  });
  return result;
}
