#include "env/environment.hpp"
#include "fiscal/amount/TaggedAmountFramework.hpp"
#include "tokenize.hpp"
#include "logger/log.hpp"
#include "text/format.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ranges>

namespace in {

  bool is_comment_line(std::string const& line) {
    return (line.size()>=2) and (line.substr(0,2) == R"(//)");
  }

  bool is_value_line(std::string const& line) {
      // Not a comment
      return (line.size() > 0) and !is_comment_line(line);
  }

  // Split s on ':' into <name> and <id>
  // <name> is 'type' or 'category' (e.g., 'HeadingAmountDateTransEntry',
  // 'TaggedAmount', 'contact', 'employee', 'sie_file')) <id> is basically a hash
  // of the <value> following the key (to identify duplicate values based on same
  // id)
  std::pair<std::string, std::optional<Environment::ValueId>> to_name_and_id(std::string const& s) {
    logger::scope_logger raii_log{logger::development_trace,"to_name_and_id"};
    logger::development_trace("s:'{}'",s);
    if (auto pos = s.find(':'); pos != std::string::npos) {
      auto name = s.substr(0, pos);
      auto id_string = s.substr(pos + 1);
      std::istringstream is{id_string};
      Environment::ValueId id{};
      if (is >> std::hex >> id) {
        logger::development_trace("name:{} id:{}",name,id);
        return {name, id};
      } else {
        // std::cout << "\nDESIGN_INSUFFICIENCY: Failed to parse the value id after "
        //              "':' in "
        //           << std::quoted(key);
        logger::design_insufficiency(R"(DESIGN_INSUFFICIENCY: Failed to parse the value id after ':' in "{}")", s);
        return {name, std::nullopt};
      }
    } else {
      return {s, std::nullopt};
    }
  }

  Environment::Value to_environment_value(std::string const& sev) {
    logger::scope_logger raii_logger{logger::development_trace,"to_environment_value"};
    logger::development_trace("sev:{}",sev);
    Environment::Value result{};
    auto kvps = tokenize::splits(sev, ';');
    for (auto const &kvp : kvps) {
      auto const &[name, value] = tokenize::split(kvp, '=');
      logger::development_trace("name:{} value:{}",name,value);
      result[name] = value;
    }
    return result;
  }

  Environment indexed_environment_from_file(std::filesystem::path const &p) {
    logger::scope_logger raii_log{logger::development_trace,"indexed_environment_from_file"};
    Environment result{};
    try {
      std::ifstream in{p};
      std::string line{};
      std::map<std::string, std::size_t> current_index{};
      while (std::getline(in, line)) {
        logger::development_trace("line:{}",line);
        if (is_value_line(line)) {
          std::istringstream in{line};
          std::string key{}, value_string{};
          in >> std::hex >> key >> std::quoted(value_string);
          logger::development_trace("key:{}, value_string:{}",key,value_string);
          auto const &[name, id] = to_name_and_id(key);
          if (id) {
            current_index[name] = *id;
          } else {
            current_index[name] = result[key].size(); // Auto indexing if none in file entry
          }
          // Each <name> maps to a list of pairs <index,environment value>
          // Where <index> is the index recorded in the file (to preserve order
          // to and from persisten storage in file).
          result[name].push_back(
              // EnvironmentValue is key-value-pairs as a map <key,value>
              // result is Environment, i.e.,
              Environment::MutableIdValuePair{current_index[name], to_environment_value(value_string)});
        }
      }
    } catch (std::runtime_error const &e) {
      // std::cout << "\nDESIGN_INSUFFICIENCY:ERROR - Read from " << p
      //           << " failed. Exception:" << e.what();
      logger::design_insufficiency(R"(DESIGN_INSUFFICIENCY:ERROR - Read from {} failed. Exception: {})", p.string(), e.what());
    }
    if (true) {
      // std::cout << "\nenvironment_from_file(" << p << ")";
      logger::development_trace(R"(environment_from_file("{}"))", p.string());
      for (auto const &[key, entry] : result) {
        logger::development_trace(R"(key:"{}" count:{})", key, entry.size());
      }
    }
    return result;
  }

} // in

Environment to_cas_environment(Environment const& indexed_environment) {
  logger::scope_logger raii_log{logger::development_trace,"to_cas_environment"};

  Environment result{};
  for (auto const& [name,indexed_id_value_pairs] : indexed_environment) {
    Environment::MutableIdValuePairs cas_id_value_pairs{};
    if (name == "TaggedAmount") {
      // We need to transform the indecies in the file to actual (hash based) value ids in CAS Environment
      // The pipe for the raw ev becomes environment_value -> TaggedAmount -> to_value_id(TaggedAmount).
      // TODO: Consider to use a schema to project environment value -> 'tagged amount environment value'?
      //       That is, use the schema to filter out any meta data in the environment value to be able
      //       to calculate the actual value id (hash) from correct input data?

      std::map<Environment::ValueId, Environment::ValueId> index_to_id{};
      for (auto const& [index,indexed_ev] : indexed_id_value_pairs) {
        // Trust that environment values (records as tag-value pairs) are ordered such that
        // any inter-value reference (aggregate, ordering etc.) using index/id always refers back to a value already in the container.
        // In this way we can always transform also the refernces in the value as we iterate trhough the container

        if (auto maybe_ta = to_tagged_amount(indexed_ev)) {
          // Encodes a tagged amount OK
          auto ta = maybe_ta.value();
          auto ta_ev = indexed_ev; // Default - as is (no encoded refs / meta-data that needs transform)

          if (indexed_ev.contains("_members")) {
            // Encodes a tagged amount aggretate (_members lists the refs to member tagged amounts)
            // We need to transform also the encoded refs from index refs to value_id refs used for tagged amounts 
            auto const& s_indexed_members = indexed_ev.at("_members");
            auto indexed_members = Key::Path{s_indexed_members};
            if (auto indexed_refs = to_value_ids(indexed_members)) {
              Key::Path cas_refs{};
              for (auto const& indexed_ref : indexed_refs.value()) {
                if (index_to_id.contains(indexed_ref)) {
                  // Already in indexed_ref map OK (thus also already in cas_id_value_pairs)
                  // Transform reference from indexed to value_id in cas
                  cas_refs += text::format::to_hex_string(index_to_id.at(indexed_ref));
                }
                else {
                  // Not yet mapped = malformed input
                  logger::design_insufficiency("to_cas_environment: Failed to look up index:{} in index_to_id map",indexed_ref);
                }
              }
              // Transform the tagged amount to carry the correct inter-value cas reference list
              auto s_cas_members = cas_refs.to_string();
              ta.tags()["_members"] = s_cas_members;
              ta_ev = to_environment_value(ta); // transformed environment value
              logger::development_trace("to_cas_environment: Transformed '{}' -> '{}'",out::to_string(indexed_ev),out::to_string(ta_ev));
            }
            else {
              logger::design_insufficiency("to_cas_environment: Failed to parse inter-value refs from '{}'",s_indexed_members);
            }            
          }
          else {
            // No encoded refs
            logger::development_trace("to_cas_environment: Not transformed {} ",out::to_string(indexed_ev));
          }

          // Transform meta-data for prev link
          if (false) {
            if (indexed_ev.contains("_prev")) {
              auto const& s_indexed_id = indexed_ev.at("_prev");
              if (auto maybe_indexed_prev = to_value_id(s_indexed_id)) {
                auto indexed_prev = maybe_indexed_prev.value();
                if (index_to_id.contains(indexed_prev)) {
                  auto cas_ref = index_to_id.at(indexed_prev);
                  auto s_cas_prev = text::format::to_hex_string(cas_ref);
                  ta.tags()["_prev"] = s_cas_prev;
                  // Log
                  if (true) {
                    logger::development_trace("Indexed _prev:{} to CAS _prev:{}",s_indexed_id,s_cas_prev);
                  }
                }
                else {
                  logger::design_insufficiency("to_cas_environment: Failed to look up Indexed _prev:{} in index_to_id map",indexed_prev);
                }
              }
            }
          }

          // Transform index to value_id for tagged amount
          auto at_id = to_value_id(ta);
          index_to_id[index] = at_id;
          logger::development_trace("to_cas_environment: index:{} -> tagged amount id:{}",index,at_id);

          // Update transformed target
          cas_id_value_pairs.push_back({index_to_id[index],ta_ev});
        }
        else {
          logger::design_insufficiency("to_cas_environment: Failed to create a tagged amount from '{}'",out::to_string(indexed_ev));
        }
      }
    }
    else {
      cas_id_value_pairs = indexed_id_value_pairs; // as-is (hash value id not used, yet = hack)
    }
    result[name] = cas_id_value_pairs;
  }
  return result;
}

Environment environment_from_file(std::filesystem::path const &p) {
  return to_cas_environment(in::indexed_environment_from_file(p));
}

namespace out {
  std::ostream& operator<<(std::ostream& os,Environment::Value const& ev) {
    bool not_first{false};
    std::for_each(ev.begin(), ev.end(), [&not_first, &os](auto const &entry) {
      if (not_first) {
        os << ";"; // separator
      }
      os << entry.first;
      os << "=";
      os << entry.second;
      not_first = true;
    });
    return os;
  }

  std::ostream& operator<<(std::ostream& os,Environment::value_type const& entry) {
    auto const &[key, id_ev_pairs] = entry;
    for (auto iter = id_ev_pairs.begin(); iter != id_ev_pairs.end(); ++iter) {
      auto const &[id, ev] = *iter;
      if (iter != id_ev_pairs.begin()) {
        os << "\n";
      }
      // Note that 'EnvironmentValue' is actually a vector of name-value-pairs for a cratchit environment entry.
      // E.g., "belopp=1389,50;datum=20221023;rubrik=Klarna"

      // Use to_string on environment value to be able to use std::quoted
      os << key << ":" << std::hex << id << " " << std::quoted(to_string(ev)) << std::dec;
    }
    return os;
  }

  std::ostream& operator<<(std::ostream& os,Environment const& env) {
    for (auto iter = env.begin(); iter != env.end(); ++iter) {
      if (iter == env.begin()) {
        os << to_string(*iter);
      }
      else {
        os << "\n" << to_string(*iter);
      }
    }
    return os;
  }

  std::string to_string(Environment::Value const& ev) {
    std::ostringstream os{};
    os << ev;
    return os.str();
  }

  std::string to_string(Environment::value_type const& entry) {
    std::ostringstream os{};
    os << entry;
    return os.str();
  }

  void indexed_environment_to_file(Environment const &indexed_environment,std::filesystem::path const &p) {
    logger::scope_logger raii_log{logger::development_trace,"indexed_environment_to_file"};
    try {
      std::ofstream out{p};
      out << indexed_environment;
    } catch (std::runtime_error const &e) {
      logger::design_insufficiency(R"(ERROR - Write to {} failed. Exception: {})", p.string(), e.what());
    }
  }

} // out

Environment to_indexed_environment(Environment const& cas_environment) {
  logger::scope_logger raii_log{logger::development_trace,"to_indexed_environment"};
  Environment result{};
  for (auto const& [section,cas_id_value_pairs] : cas_environment) {
    Environment::MutableIdValuePairs indexed_id_value_pairs{}; // Transform target

    // Transform inter-value references in cas_id_value_pairs
    // from value_ids to integer indexed ids
    std::map<Environment::ValueId,Environment::ValueId> id_to_index{};
    for (auto const& [cas_id,cas_ev] : cas_id_value_pairs) {
      if (id_to_index.contains(cas_id) == false) {
        // OK, unique cas_id
        id_to_index[cas_id] = id_to_index.size(); // Simply enumerate the refs 0...

        auto indexed_ev = cas_ev; // default

        if (cas_ev.contains("_members")) {
          // transform the cas refs to index refs
          auto const& s_cas_members = cas_ev.at("_members");
          auto cas_members = Key::Path{s_cas_members};
          if (auto maybe_cas_refs = to_value_ids(cas_members)) {
            Key::Path index_refs{};
            for (auto const& cas_ref : maybe_cas_refs.value()) {
              if (id_to_index.contains(cas_ref)) {
                // Already in ref map OK (thus also already in transformed indexed_id_value_pairs)
                // Transform reference from value id in cas to intereger index ref

                // TODO: Fix this hack to create a hex-string of looked-up value id
                std::ostringstream os{};
                os << std::hex << id_to_index.at(cas_ref);
                index_refs += os.str();
              }
              else {
                // Not yet mapped = malformed input
                logger::design_insufficiency("to_indexed_environment: Failed to look up id:{} in id_to_index map",cas_ref);
              }
            }
            indexed_ev["_members"] = index_refs.to_string(); // transform the refs
            logger::development_trace("to_indexed_environment: Transformed:'{}' -> '{}'",out::to_string(cas_ev),out::to_string(indexed_ev));
          }
          else {
            logger::design_insufficiency("to_indexed_environment: Failed to parse inter-value refs from _members:'{}'",s_cas_members);
          }            
        }

        if (false) {
          if (cas_ev.contains("_prev")) {
            auto s_cas_prev = cas_ev.at("_prev");
            auto maybe_cas_prev = to_value_id(s_cas_prev);
            if (maybe_cas_prev) {
              auto cas_prev = maybe_cas_prev.value();
              if (id_to_index.contains(cas_prev)) {
                auto indexed_prev = id_to_index.at(cas_prev);
                auto s_indexed_prev = text::format::to_hex_string(indexed_prev);
                indexed_ev["_prev"] = s_indexed_prev;

                if (true) {
                  logger::development_trace("to_indexed_environment: Transformed cas _prev:{} to indexed _prev:{}",s_cas_prev,s_indexed_prev);
                }
              }
              else {
                logger::design_insufficiency("to_indexed_environment: Failed to look up _prev cas_id:{} in id_to_index",cas_prev);
              }
            }
            else {
              logger::design_insufficiency("to_indexed_environment: Failed to convert '{}' to a valid CAS id",s_cas_prev);
            }
          }
        }

        // else {
        //   logger::development_trace("to_indexed_environment: Not transformed:'{}'",out::to_string(indexed_ev));
        // }
        indexed_id_value_pairs.push_back({id_to_index[cas_id],indexed_ev});
      }
      else {
        logger::design_insufficiency("to_indexed_environment: Found duplicate value_id:{} in provided cas_environment");
      }
    }

    result[section] = indexed_id_value_pairs;
  }
  return result;
}

void environment_to_file(Environment const &environment,std::filesystem::path const &p) {
  logger::scope_logger raii_log{logger::development_trace,"environment_to_file"};
  return out::indexed_environment_to_file(to_indexed_environment(environment),p);
}
