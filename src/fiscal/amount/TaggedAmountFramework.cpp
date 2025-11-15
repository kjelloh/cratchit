#include "TaggedAmountFramework.hpp"
#include "../../logger/log.hpp"
#include "text/format.hpp"
#include "functional/ranges.hpp" // adjacent_value_pairs,...
#include "csv/functional.hpp" // CSV::functional::Result<T>,...
#include "persistent/in/maybe.hpp" // in::MaybeIStream,...
#include <iostream> // ,std::cout
#include <sstream> // std::ostringstream, std::istringstream
#include <algorithm> // std::all_of,
#include <ranges> // std::ranges::subrange,ranges::find,
#include <fstream> // std::ifstream

// BEGIN class TaggedAmount

// TaggedAmount::TaggedAmount(Date const& date, CentsAmount const& cents_amount, Tags&& tags)
//     : m_date{date},m_cents_amount{cents_amount}, m_tags{tags} {}

// lvalue/rvalue constructor
// An lvalue will be copied + moved.
// An rvalue will be 'perfactly' moved
TaggedAmount::TaggedAmount(Date date, CentsAmount cents_amount,Tags tags)
    : m_date{std::move(date)},m_cents_amount{std::move(cents_amount)}, m_tags{std::move(tags)} {}

// Replaced with text::format::to_hex_string
// std::string TaggedAmount::to_string(TaggedAmount::ValueId value_id) {
//   // TODO: Consider a safe way to ensure ALL value ids gets converted to the same string
//   //       to ernsure consistent environment value encoding / 20251021
//   //       Also see to_cas_environment(indexed_environment)

//   // std::ostringstream os{};
//   // os << std::setw(sizeof(std::size_t) * 2) << std::setfill('0') << std::hex
//   //    << value_id << std::dec;
//   // return os.str();
//   return std::format("{:x}",value_id);
// }

bool TaggedAmount::operator==(TaggedAmount const& other) const {
  auto result =
      this->date() == other.date()
      and this->cents_amount() == other.cents_amount()
  //  and std::all_of(m_tags.begin(), m_tags.end(),
  //                 [&other](Tags::value_type const& entry) {
  //                   return ((entry.first.starts_with("_")) or
  //                           (other.tags().contains(entry.first) and
  //                            other.tags().at(entry.first) == entry.second));
  //                 });

      // Changed 20251026 - Now equal is all-tags-equal (no meta '_xxx' tag excepion)
      and this->m_tags == other.m_tags;

  // std::cout << "\nTaggedAmountClass::operator== ";
  // if (result) std::cout << "TRUE"; else std::cout << "FALSE";
  return result;
}

bool TaggedAmount::operator<(TaggedAmount const& other) const {
  // Tagged Amounts are implicitally ordered by date
  // DateOrderedTaggedAmountsContainer orders samet date values by upper_bound (insertion order for same date)
  return this->date() < other.date();
}

// END class TaggedAmount

// TaggedAmount to Value Id
TaggedAmount::ValueId to_value_id(TaggedAmount const& ta) {
  return std::hash<TaggedAmount>{}(ta);
}

std::ostream& operator<<(std::ostream &os, TaggedAmount const& ta) {
  // os <<  text::format::to_hex_string(to_value_id(ta));
  os << " " << ::to_string(ta.date());
  os << " " << ::to_string(to_units_and_cents(ta.cents_amount()));
  for (auto const& tag : ta.tags()) {
    os << "\n\t|--> \"" << tag.first << "=" << tag.second << "\"";
  }
  return os;
}

// Hex listing string to Value Ids (for parsing tags that encodes references as valude Ids)
TaggedAmount::OptionalValueIds to_maybe_value_ids(Key::Sequence const& sids) {
  // std::cout << "\nto_maybe_value_ids()" << std::flush;
  TaggedAmount::OptionalValueIds result{};
  TaggedAmount::ValueIds value_ids{};
  for (auto const& sid : sids) {
    if (auto maybe_value_id = to_maybe_value_id(sid)) {
      // std::cout << "\n\tA valid instance id sid=" << std::quoted(sid);
      value_ids.push_back(maybe_value_id.value());
    } else {
      std::cout
          << "\nDESIGN_INSUFFICIENCY: to_maybe_value_ids: Not a valid instance id string sid="
          << std::quoted(sid) << std::flush;
    }
  }
  if (value_ids.size() == sids.size()) {
    result = value_ids;
  } else {
    std::cout << "\nDESIGN_INSUFFICIENCY: to_maybe_value_ids(Key::Sequence const& "
              << sids.to_string() << ") Failed. Created" << value_ids.size()
              << " out of " << sids.size() << " possible.";
  }
  return result;
}

// Hex string to Value Id (for parsing tags that encodes references as valude Ids)
TaggedAmount::OptionalValueId to_maybe_value_id(std::string const& sid) {
  // std::cout << "\nto_maybe_value_id()" << std::flush;
  TaggedAmount::OptionalValueId result{};
  TaggedAmount::ValueId value_id{};
  std::istringstream is{sid};
  try {
    is >> std::hex >> value_id;
    result = value_id;
  } catch (...) {
    std::cout << "\nto_maybe_value_id(" << std::quoted(sid)
              << ") failed. General Exception caught." << std::flush;
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

namespace zeroth {
  // BEGIN class DateOrderedTaggedAmountsContainer

  // Container

  std::pair<DateOrderedTaggedAmountsContainer::ValueId,bool> DateOrderedTaggedAmountsContainer::dotas_insert_auto_ordered_value(TaggedAmount const& ta) {

    if (true) {
      // 'Newer' pre-linked-encoded ordering
      logger::scope_logger scope_log_raii{logger::development_trace,"DateOrderedTaggedAmountsContainer::dotas_insert_auto_ordered_value: prev-linked-encoded ordering"};

      auto [prev_and_next_pair,transformed_ta] = this->to_prev_next_pair_and_transformed_ta(ta);
      auto [maybe_prev,maybe_next] = prev_and_next_pair;

      if (auto put_result = m_tagged_amount_cas_repository.cas_repository_put(transformed_ta);put_result.second) {
        logger::development_trace(
           "New value at:{} value:{}"
          ,text::format::to_hex_string(put_result.first)
          ,to_string(transformed_ta));
        // New linked value
        auto insert_pos = m_date_ordered_value_ids.begin(); // Default for no prev

        if (maybe_prev) {
          // Not at begin = adjust insert_pos to actual insert pos
          logger::development_trace(
             "Valid prev:{} with value:{}"
            ,text::format::to_hex_string(maybe_prev.value())
            ,to_string(this->at(maybe_prev.value()).value_or(TaggedAmount{Date{},CentsAmount{}})));

          // Find iterator to this ValueId
          insert_pos = std::ranges::find(m_date_ordered_value_ids, maybe_prev.value());
          if (insert_pos != m_date_ordered_value_ids.end()) {
              ++insert_pos; // Adjust to make std::vector::insert do what we want (inserts before iter)
          }
        }

        // If maybe_prev_vid is nullopt, insert_pos stays at begin() (insert at / before start)
        m_date_ordered_value_ids.insert(insert_pos, put_result.first);

        // Log
        if (maybe_prev) {
          logger::development_trace(
             "m_date_ordered_value_ids inserted _prev:{} -> id:{}"
            ,text::format::to_hex_string(maybe_prev.value())
            ,text::format::to_hex_string(put_result.first));
        }
        else {
          logger::development_trace(
             "m_date_ordered_value_ids inserted _prev:null -> id:{}"
            ,text::format::to_hex_string(put_result.first));
        }

        // Re-link?
        if (maybe_next) {
          logger::scope_logger scope_raii{logger::development_trace,"Re-link after insert"};

          logger::development_trace(
             "Valid next:{:x}: {}"
            ,maybe_next.value()
            ,to_string(this->at(maybe_next.value()).value_or(TaggedAmount{Date{},CentsAmount{}})));

          auto begin = std::ranges::find(m_date_ordered_value_ids,maybe_next.value());

          using namespace cratchit::functional::ranges; // adjacent_iterator_pairs
          for (auto [it, next] : adjacent_iterator_pairs(m_date_ordered_value_ids)) {
              auto lhs_id = *it;
              auto& rhs_id = *next;

              if (auto maybe_ta = this->at(rhs_id)) {
                  auto relinked_ta = to_linked_encoded_ta(lhs_id, maybe_ta.value());

                  if (relinked_ta == maybe_ta.value()) break; // The chain is unbroken from here

                  if (auto [new_id, ok] = m_tagged_amount_cas_repository.cas_repository_put(relinked_ta); ok) {

                    rhs_id = new_id; // in-place update

                    // Log
                    logger::development_trace(
                      "Re-linked as new _prev:{:x} <- relinked_ta:{}"
                      ,new_id
                      ,to_string(relinked_ta)
                    );

                    // Whine
                    logger::design_insufficiency(
                       "Orphan value left in cas {}"
                      ,to_string(maybe_ta.value()));

                  }
                  else if (auto [same_id,ok] = m_tagged_amount_cas_repository.cas_repository_mutate_transient(relinked_ta);ok) {
                    // mutate as transient ok (cas accepted relinked_ta as a value already in cas but mutated it anyways)
                    logger::development_trace(
                      "Mutated (already in CAS) as transient {:x}: transformed_ta:{}"
                      ,same_id
                      ,to_string(relinked_ta)
                    );
                  }
                  else {
                    logger::development_trace(
                      "Ignored (already in CAS) {:x}: transformed_ta:{}"
                      ,new_id
                      ,to_string(transformed_ta)
                    );
                  }
              }
          }          
            
          // std::ranges::for_each(
          //   cratchit::functional::ranges::adjacent_value_pairs(
          //     std::ranges::subrange(begin, m_date_ordered_value_ids.end())
          //   )
          //   ,[this](auto const& pair){

          //     auto relinked_ta = to_linked_encoded_ta(
          //        pair.first
          //       ,this->at(pair.second).value());

          //     if (auto put_result = m_tagged_amount_cas_repository.cas_repository_put(relinked_ta);put_result.second) {

          //       auto iter = std::ranges::find(this->m_date_ordered_value_ids,pair.second);
          //       *iter = put_result.first; // replace value_id

          //       logger::development_trace(
          //         "Re-linked {}  <- {}: {}"
          //         ,pair.first
          //         ,put_result.first
          //         ,to_string(relinked_ta));
          //     }
          //     else {
          //       logger::development_trace(
          //         "Ignored {}  <- {}: {}"
          //         ,pair.first
          //         ,put_result.first
          //         ,to_string(relinked_ta));
          //     }
          //   }
          // );

        }

        return put_result;
      }
      else {
        // No op - transformed_ta (properly linked in ta) already in container (and CAS)
        logger::development_trace(
          "DateOrderedTaggedAmountsContainer::dotas_insert_auto_ordered_value: Already in CAS at:{} '{}' = IGNORED"
          ,text::format::to_hex_string(put_result.first)
          ,to_string(transformed_ta));
        logger::development_trace(
           "                                                                                         at:{} '{}' = IN CAS"
          ,text::format::to_hex_string(put_result.first)
          ,to_string(this->at(put_result.first).value()));
        return put_result;
      }
    }
    else {
      // 'Older' no-prev-encoding ordering
      auto put_result = m_tagged_amount_cas_repository.cas_repository_put(ta);
      if (put_result.second == true) {

        // Log
        if (false) {
          std::cout << "\nthis:" << this << " Inserted new " << ta;
        }

        auto insert_pos = m_date_ordered_value_ids.begin();

        if (auto maybe_prev = to_prev(ta)) {
            // Find iterator to this ValueId
            insert_pos = std::ranges::find(m_date_ordered_value_ids, maybe_prev.value());
            if (insert_pos != m_date_ordered_value_ids.end()) {
                ++insert_pos; // Adjust to make std::vector::insert do what we want (inserts before iter)
            }
        }

        // If maybe_prev_vid is nullopt, insert_pos stays at begin() (insert at / before start)
        m_date_ordered_value_ids.insert(insert_pos, put_result.first);

        if (insert_pos != m_date_ordered_value_ids.end()) {
          logger::design_insufficiency("DateOrderedTaggedAmountsContainer::dotas_insert_auto_ordered_value: insert before end but no re-linking yet implemented");
        }

      } 
      else {
        // No op - ta already in container (and CAS)
        logger::development_trace(
           "DateOrderedTaggedAmountsContainer::dotas_insert_auto_ordered_value: Already in CAS at:{} '{}' = IGNORED"
          ,text::format::to_hex_string(put_result.first)
          ,to_string(ta));
        logger::development_trace(
           "                                                                                         at:{} '{}' = IN CAS"
          ,text::format::to_hex_string(put_result.first)
          ,to_string(this->at(put_result.first).value()));      
      }

      if (m_date_ordered_value_ids.size() > m_tagged_amount_cas_repository.size()) {
        logger::design_insufficiency("DateOrderedTaggedAmountsContainer: Unexpected m_date_ordered_value_ids.size():{} > m_tagged_amount_cas_repository.size():{}",m_date_ordered_value_ids.size(), m_tagged_amount_cas_repository.size());
      }

      return put_result;

    }
  }

  std::pair<DateOrderedTaggedAmountsContainer::ValueId,bool> DateOrderedTaggedAmountsContainer::dotas_append_value(
      DateOrderedTaggedAmountsContainer::OptionalValueId maybe_prev
    ,TaggedAmount const& ta
    ,bool auto_order_compability_mode) {

    std::pair<DateOrderedTaggedAmountsContainer::ValueId,bool> result{0,false}; // default

    logger::scope_logger scope_log_raii{logger::development_trace,"DateOrderedTaggedAmountsContainer::dotas_append_value"};

    // Assert provided prev matches 'older' date auto ordering (no behavioral change)
    if (auto_order_compability_mode) {
      logger::scope_logger scope_logger_raii{logger::development_trace,"AUTO ORDERING COMPABILITY MODE"};
      // Check consistency with 'older' auto_order design
      // We expect the order provided to match the auto-order (date ordering) we have applied so far
      auto auto_ordered_maybe_prev = to_prev(ta);
      if (auto_ordered_maybe_prev != maybe_prev) {
        logger::design_insufficiency(
           "dotas_append_value: Expetced provided _prev:{:x} to be auto-order prev:{:x}"
          ,maybe_prev.value_or(0)
          ,auto_ordered_maybe_prev.value_or(0));
        return this->dotas_insert_auto_ordered_value(ta); // force old behavior
      }
    }
    else {
      // Require pre and provided value to fullfill date order (less or equal is ok)
      auto maybe_prev_ta = maybe_prev.and_then([this](auto prev){return this->at(prev);});
      bool in_date_order = maybe_prev_ta.transform([&ta](auto const& prev_ta) {
          return (prev_ta.date() <= ta.date());
        }).value_or(true); // No prev is also 'in order'
      
      if (!in_date_order) return {0,false};
    }

    auto linked_ta = to_linked_encoded_ta(maybe_prev,ta);

    if (maybe_prev) {
      // Link to prev
      auto prev = maybe_prev.value();
      if (this->m_date_ordered_value_ids.back() == prev) {
        // _prev is last - append OK

        auto [value_id,was_inserted] = this->m_tagged_amount_cas_repository.cas_repository_put(linked_ta);

        if (was_inserted) {
          // Update ordering
          this->m_date_ordered_value_ids.push_back(value_id);
        }
        else {
          logger::design_insufficiency(
              "dotas_append_value: Failed - Can't append value that already exists in cas id:{} ta:{}"
            ,text::format::to_hex_string(value_id)
            ,to_string(ta));
        }

        result = {value_id,was_inserted};

      }
      else {
        logger::design_insufficiency(
            "dotas_append_value: Failed - Can't append value that is not last in m_date_ordered_value_ids _prev:{} ta:{}"
          ,text::format::to_hex_string(prev)
          ,to_string(ta));
      }
    }
    else {
      if (this->m_date_ordered_value_ids.size() == 0) {

        // Empty sequence - null _prev OK

        auto [value_id,was_inserted] = this->m_tagged_amount_cas_repository.cas_repository_put(linked_ta);

        if (was_inserted) {
          this->m_date_ordered_value_ids.push_back(value_id);
        }
        else {
          logger::design_insufficiency(
              "dotas_append_value: Failed - Empty m_date_ordered_value_ids but value exists in cas id:{} ta:{}"
            ,text::format::to_hex_string(value_id)
            ,to_string(ta));
        }

        result = {value_id,was_inserted};

      }
      else {
        logger::design_insufficiency(
            "dotas_append_value: Failed - Non Empty m_date_ordered_value_ids but provided _prev:null ta:{}"
          ,to_string(ta));
      }
    }

    return result;

  }

  DateOrderedTaggedAmountsContainer& DateOrderedTaggedAmountsContainer::dotas_insert_auto_ordered_container(DateOrderedTaggedAmountsContainer const& other) {
    std::ranges::for_each(other.ordered_tas_view(),[this](TaggedAmount const& ta) {
      this->dotas_insert_auto_ordered_value(ta);
    });

    return *this;
  }

  DateOrderedTaggedAmountsContainer& DateOrderedTaggedAmountsContainer::dotas_insert_auto_ordered_sequence(TaggedAmounts const& tas) {
    for (auto const& ta : tas)
      this->dotas_insert_auto_ordered_value(ta);
    return *this;
  }

  // Accessors
  bool DateOrderedTaggedAmountsContainer::contains(TaggedAmount const& ta) const {
    auto value_id = to_value_id(ta);
    return m_tagged_amount_cas_repository.contains(value_id);
  }

  OptionalTaggedAmount DateOrderedTaggedAmountsContainer::at(ValueId const& value_id) const {
    if (false) {
      logger::development_trace("DateOrderedTaggedAmountsContainer::at({})",text::format::to_hex_string(value_id));
    }
    OptionalTaggedAmount result{m_tagged_amount_cas_repository.cas_repository_get(value_id)};
    if (!result) {
      logger::development_trace("DateOrderedTaggedAmountsContainer::at({}), No value -> returns std::nullopt",text::format::to_hex_string(value_id));
    }
    return result;
  }

  // OptionalTaggedAmount DateOrderedTaggedAmountsContainer::operator[](ValueId const& value_id) const {
  //   std::cout << "\nDateOrderedTaggedAmountsContainer::operator[]("
  //             << TaggedAmount::to_string(value_id) << ")" << std::flush;
  //   OptionalTaggedAmount result{};
  //   if (auto maybe_ta = this->at(value_id)) {
  //     result = maybe_ta;
  //   } else {
  //     std::cout << "\nDateOrderedTaggedAmountsContainer::operator[] could not "
  //                 "find a mapping to value_id="
  //               << TaggedAmount::to_string(value_id) << std::flush;
  //   }
  //   return result;
  // }

  TaggedAmountsCasRepository& DateOrderedTaggedAmountsContainer::cas() {
  return m_tagged_amount_cas_repository;
  }

  TaggedAmountsCasRepository const& DateOrderedTaggedAmountsContainer::cas() const {
  return m_tagged_amount_cas_repository;
  }


  // Sequence

  std::size_t DateOrderedTaggedAmountsContainer::sequence_size() const {
    return m_date_ordered_value_ids.size();
  }

  // TaggedAmounts const& DateOrderedTaggedAmountsContainer::ordered_tas() const {
  //   return m_dotas;
  // }

  // DateOrderedTaggedAmountsContainer::const_iterator DateOrderedTaggedAmountsContainer::begin() const {
  //   return m_dotas.begin(); 
  // }

  // DateOrderedTaggedAmountsContainer::const_iterator DateOrderedTaggedAmountsContainer::end() const {
  //   return m_dotas.end(); 
  // }

  OptionalTaggedAmounts DateOrderedTaggedAmountsContainer::to_tagged_amounts(ValueIds const& value_ids) const {
    std::cout << "\nDateOrderedTaggedAmountsContainer::to_tagged_amounts()"
              << std::flush;
    OptionalTaggedAmounts result{};
    TaggedAmounts tas{};
    for (auto const& value_id : value_ids) {
      if (auto maybe_ta = this->at(value_id)) {
        tas.push_back(maybe_ta.value());
      } else {
        std::cout << "\nDateOrderedTaggedAmountsContainer::to_tagged_amounts() "
                    "failed. No instance found for value_id="
                  << text::format::to_hex_string(value_id) << std::flush;
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

  TaggedAmounts DateOrderedTaggedAmountsContainer::ordered_tagged_amounts() const {
    // value-semantics by copy (safe for now)
    return 
        ordered_tas_view()
      | std::ranges::to<TaggedAmounts>();
  }

  TaggedAmounts DateOrderedTaggedAmountsContainer::date_range_tagged_amounts(zeroth::DateRange const& date_period) const {
    // value-semantics by copy (safe for now)
    return 
      this->date_range_tas_view(date_period)
      | std::ranges::to<TaggedAmounts>();
  }

  DateOrderedTaggedAmountsContainer& DateOrderedTaggedAmountsContainer::erase(ValueId const& value_id) {
    if (auto maybe_ta = this->at(value_id)) {
      m_tagged_amount_cas_repository.erase(value_id);    
      auto iter = std::ranges::find(m_date_ordered_value_ids, value_id);
      if (iter != m_date_ordered_value_ids.end()) {
        m_date_ordered_value_ids.erase(iter);
      } else {
        logger::design_insufficiency("DateOrderedTaggedAmountsContainer::erase: Failed to erase tagged amount - in map but not in date-ordered-vector, value_id:{}",value_id);
      }
    } 
    else {
      logger::design_insufficiency("DateOrderedTaggedAmountsContainer::erase: Failed to find value_id:{}",value_id);
    }
    return *this;
  }


  DateOrderedTaggedAmountsContainer& DateOrderedTaggedAmountsContainer::reset(DateOrderedTaggedAmountsContainer const& other) {
    this->m_date_ordered_value_ids = other.m_date_ordered_value_ids;
    this->m_tagged_amount_cas_repository = other.m_tagged_amount_cas_repository;
    return *this;
  }

  DateOrderedTaggedAmountsContainer& DateOrderedTaggedAmountsContainer::reset(TaggedAmounts const& tas) {
    this->clear();
    // *this += tas;
    this->dotas_insert_auto_ordered_sequence(tas);
    return *this;
  }

  DateOrderedTaggedAmountsContainer&  DateOrderedTaggedAmountsContainer::clear() {
    m_tagged_amount_cas_repository.clear();
    m_date_ordered_value_ids.clear();
    return *this;
  }

  // Encode _prev into provided ta as defined by provided prev arg
  TaggedAmount DateOrderedTaggedAmountsContainer::to_linked_encoded_ta(
     DateOrderedTaggedAmountsContainer::OptionalValueId maybe_prev
    ,TaggedAmount const& ta) {

    auto result = ta;
    if (maybe_prev) {
      auto new_s_prev = text::format::to_hex_string(maybe_prev.value());      
      result.tags()["_prev"] = new_s_prev;
    }
    else {
      // No prev
      result.tags().erase("_prev");
    }
    return result;
  }

  DateOrderedTaggedAmountsContainer::PrevNextPair DateOrderedTaggedAmountsContainer::to_prev_and_next(TaggedAmount const& ta) {

    DateOrderedTaggedAmountsContainer::PrevNextPair result{};

    auto maybe_date_compare = [](OptionalDate maybe_lhs_date,OptionalDate maybe_rhs_date){
      if (maybe_lhs_date and maybe_rhs_date) {
        return maybe_lhs_date.value() < maybe_rhs_date.value();
      }
      return false;
    };

    auto value_id_to_ta_maybe_date = [this](ValueId value_id) -> OptionalDate {
      if (auto maybe_ta = this->m_tagged_amount_cas_repository.cas_repository_get(value_id)) {
        return maybe_ta->date();
      }
      logger::design_insufficiency("dotas_insert_auto_ordered_value: Detected corrupt m_date_ordered_value_ids. Failed to map value_id:{} to value",value_id);
      return std::nullopt;
    };

    auto next_iter = std::ranges::upper_bound(
        m_date_ordered_value_ids
        ,ta.date()
        ,maybe_date_compare
        ,value_id_to_ta_maybe_date);

    auto prev_iter = next_iter;

    if (next_iter != m_date_ordered_value_ids.begin()) {
      // For non empty m_date_ordered_value_ids decrement on non-begin is valid (including end).
      // For empty m_date_ordered_value_ids next_iter is end and begin == end, so this if is skipped (no decrement)
      --prev_iter;
    }
    else {
      // next_iter == begin --> no prev
      prev_iter = m_date_ordered_value_ids.end(); // No prev
    }
    
    if (prev_iter != m_date_ordered_value_ids.end()) {
      // prev exists
      result.first = *prev_iter;
    }
    if (next_iter != m_date_ordered_value_ids.end()) {
      // next exists
      result.second = *next_iter;
    }
    return result;
  }

  DateOrderedTaggedAmountsContainer::OptionalValueId DateOrderedTaggedAmountsContainer::to_prev(TaggedAmount const& ta) {
    auto prev_and_next = to_prev_and_next(ta);
    return prev_and_next.first;
  }

  std::pair<
     DateOrderedTaggedAmountsContainer::PrevNextPair
    ,TaggedAmount> DateOrderedTaggedAmountsContainer::to_prev_next_pair_and_transformed_ta(TaggedAmount const& ta) {

    auto ta_with_prev = ta;
    auto prev_and_next = to_prev_and_next(ta);

    // Transform meta-data prev-link reference
    if (true) {

      ta_with_prev = to_linked_encoded_ta(prev_and_next.first,ta);

      // Log
      if (true) {
        auto maybe_s_prev_lhs = ta.tag_value("_prev");
        auto maybe_s_prev_rhs = ta_with_prev.tag_value("_prev");
        if (maybe_s_prev_lhs != maybe_s_prev_rhs) {
          logger::development_trace(
            "to_prev_next_pair_and_transformed_ta: Re-linked _prev:{} to _prev:{}"
            ,maybe_s_prev_lhs.value_or("null")
            ,maybe_s_prev_rhs.value_or("null"));
        }
      }

    }


    return {prev_and_next,ta_with_prev};
  }

  // END class DateOrderedTaggedAmountsContainer

}

TaggedAmount::ValueId TaggedAmountHasher::operator()(TaggedAmount const& ta) const {
  return to_value_id(ta);
}

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

    OptionalTaggedAmounts to_tas(
       CSV::project::HeadingId const& csv_heading_id
      ,CSV::OptionalTable const& maybe_csv_table) {
      OptionalTaggedAmounts result{};
      if (maybe_csv_table) {
        auto to_tagged_amount = make_tagged_amount_projection(csv_heading_id,maybe_csv_table->heading);
        TaggedAmounts tas{};
        for (auto const& field_row : maybe_csv_table->rows) {
          if (auto maybe_ta = to_tagged_amount(field_row)) {
            tas.push_back(maybe_ta.value());
          }
          else {
            logger::cout_proxy << "\nCSV::project::to_dota: Sorry, Failed to create tagged amount from field_row " << std::quoted(to_string(field_row));
          }
        }
        result = tas;
      }
      else {
        logger::development_trace("CSV::project::to_dota - Null table -> nullopt result");
      }
      return result;
    } // to_dota
  } // project
} // CSV

template <typename T>
using CSVProcessResult = CSV::functional::CSVProcessResult<T>;

CSVProcessResult<persistent::in::MaybeIStream> file_path_to_istream(std::filesystem::path const& statement_file_path) {
  CSVProcessResult<persistent::in::MaybeIStream> result{};
  result.m_value = persistent::in::to_maybe_istream(statement_file_path);
  if (!result.m_value) result.push_message("file_path_to_istream: Failed to create istream");
  return result;
}

CSVProcessResult<std::string> istream_to_decoded_text(persistent::in::MaybeIStream const& maybe_istream) {
  CSVProcessResult<std::string> result{};
  result.push_message("istream_to_decoded_text: NOT YET IMPLEMENTED");

  return result;
}
CSVProcessResult<CSV::Table> decoded_text_to_parsed_csv(std::string const& s) {
  CSVProcessResult<CSV::Table> result{};
  result.push_message("decoded_text_to_parsed_csv: NOT YET IMPLEMENTED");
  return result;
}

struct AccountStatement { /* TBD */};
using AccountStatements = std::vector<AccountStatement>;
CSVProcessResult<AccountStatements> parsed_csv_to_account_statements(CSV::Table const& table) {
  CSVProcessResult<AccountStatements> result{};
  result.push_message("parsed_csv_to_account_statements: NOT YET IMPLEMENTED");
  return result;
}
CSVProcessResult<TaggedAmounts> account_statements_to_tas(AccountStatements const& account_statements) {
  CSVProcessResult<TaggedAmounts> result{};
  result.push_message("account_statements_to_tas: NOT YET IMPLEMENTED");
  return result;
}

/**
* Return a list of tagged amounts if provided statement_file_path is to a file with amount values (e.g., a bank account csv statements file)
*/
OptionalTaggedAmounts tas_from_statment_file(std::filesystem::path const& statement_file_path) {

  if (true) {
    std::cout << "\nto_tagged_amounts(" << statement_file_path << ")";
  }

  if (true) {
    // Refactored to pipeline

    auto result = file_path_to_istream(statement_file_path)
      .and_then(istream_to_decoded_text)
      .and_then(decoded_text_to_parsed_csv)
      .and_then(parsed_csv_to_account_statements)
      .and_then(account_statements_to_tas);

    std::ranges::for_each(result.m_messages,[](auto const& message){
      std::cout << std::format("\nTODO: {}",message);
    });
    return result.m_value;
  }
  else {
    OptionalTaggedAmounts result{};
    CSV::OptionalFieldRows field_rows{};
    std::ifstream ifs{statement_file_path};
    // NOTE: The mechanism implemented to apply correct decoding and parsing of different files is a mess!
    //       For one, The runtime on Mac uses UTF-8 through the console by default (which Windows and Unix may or may not do).
    //       Also, The NORDEA CSV-file as downloaded from NORDEA web bank through Safari browser is also UTF-8 encoded (although I am not sure it will be using another browser on another platform?).
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
        result =  CSV::project::to_tas(csv_heading_id,CSV::to_table(field_rows,heading_projection));
      }
    }
    return result;
  }
}

auto ev_to_maybe_ta = [](Environment::Value const& ev) -> OptionalTaggedAmount {
  return to_tagged_amount(ev);
};
auto in_period = [](TaggedAmount const& ta, FiscalPeriod const& period) -> bool {
  return period.contains(ta.date());
};  
auto id_ev_pair_to_ev = [](Environment::MutableIdValuePair const& id_ev_pair) {
  return id_ev_pair.second;
};

DateOrderedTaggedAmountsContainer dotas_from_environment(const Environment &env) {
  DateOrderedTaggedAmountsContainer result{};
  static constexpr auto section = "TaggedAmount";
  if (!env.contains(section)) {
    return {}; // No entries of this type
  }
  auto const& id_ev_pairs = env.at(section);
  auto tas_sequence = id_ev_pairs 
    | std::views::transform(id_ev_pair_to_ev) 
    | std::views::transform(ev_to_maybe_ta) 
    | std::views::filter([](auto const& maybe_ta) { return maybe_ta.has_value(); }) 
    | std::views::transform([](auto const& maybe_ta) { return maybe_ta.value(); }) 
    | std::ranges::to<std::vector>();

  std::ranges::for_each(tas_sequence,[&result](TaggedAmount const& ta) {
    if (true) {
      // 'Newer' respect _prev encoded order
      auto maybe_s_prev = ta.tag_value("_prev");
      auto maybe_prev = maybe_s_prev.and_then([](std::string s_prev) {
        return to_maybe_value_id(s_prev);
      });
      auto [value_id,was_inserted] = result.dotas_append_value(maybe_prev,ta,true);
    }
    else {
      // 'older' insert based on ta date
      result.dotas_insert_auto_ordered_value(ta); // put and apply auto (date) ordering
    }
  });

  return result;
}

// Environment -> TaggedAmounts (filtered by fiscal period)
DateOrderedTaggedAmountsContainer to_period_dotas_container(FiscalPeriod period, const Environment &env) {
  DateOrderedTaggedAmountsContainer result{};
  static constexpr auto section = "TaggedAmount";
  if (!env.contains(section)) {
    return {}; // No entries of this type
  }
  auto const& id_ev_pairs = env.at(section);
  auto tas_sequence = id_ev_pairs 
    | std::views::transform(id_ev_pair_to_ev) 
    | std::views::transform(ev_to_maybe_ta) 
    | std::views::filter([](auto const& maybe_ta) { return maybe_ta.has_value(); }) 
    | std::views::transform([](auto const& maybe_ta) { return maybe_ta.value(); }) 
    | std::views::filter([&](auto const& ta) { return in_period(ta, period); }) 
    | std::ranges::to<std::vector>();
  
  std::ranges::for_each(tas_sequence,[&result](TaggedAmount const& ta) {
    result.dotas_insert_auto_ordered_value(ta); // put and apply ordering
  });

  return result;
}
