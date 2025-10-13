#include "env/environment.hpp"
#include "tokenize.hpp"
#include "logger/log.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace in {

  bool is_comment_line(std::string const& line) {
    return (line.size()>=2) and (line.substr(0,2) == R"(//)");
  }

  bool is_value_line(std::string const& line) {
      // Not a comment
      return (line.size() > 0) and !is_comment_line(line);
  }

  // Split <key> on ':' into <name> and <id>
  // <name> is 'type' or 'category' (e.g., 'HeadingAmountDateTransEntry',
  // 'TaggedAmount', 'contact', 'employee', 'sie_file')) <id> is basically a hash
  // of the <value> following the key (to identify duplicate values based on same
  // id)
  std::pair<std::string, std::optional<EnvironmentValueId>> to_name_and_id(std::string key) {
    logger::scope_logger raii_log{logger::development_trace,"to_name_and_id"};
    logger::development_trace("key:{}",key);
    if (auto pos = key.find(':'); pos != std::string::npos) {
      auto name = key.substr(0, pos);
      auto id_string = key.substr(pos + 1);
      std::istringstream is{id_string};
      EnvironmentValueId id{};
      if (is >> std::hex >> id) {
        logger::development_trace("name:{} id:{}",name,id);
        return {name, id};
      } else {
        // std::cout << "\nDESIGN_INSUFFICIENCY: Failed to parse the value id after "
        //              "':' in "
        //           << std::quoted(key);
        logger::design_insufficiency(R"(DESIGN_INSUFFICIENCY: Failed to parse the value id after ':' in "{}")", key);
        return {name, std::nullopt};
      }
    } else {
      return {key, std::nullopt};
    }
  }

  EnvironmentValue to_environment_value(std::string const s) {
    logger::scope_logger raii_logger{logger::development_trace,"to_environment_value"};
    logger::development_trace("s:{}",s);
    EnvironmentValue result{};
    auto kvps = tokenize::splits(s, ';');
    for (auto const &kvp : kvps) {
      auto const &[name, value] = tokenize::split(kvp, '=');
      logger::development_trace("name:{} value:{}",name,value);
      result[name] = value;
    }
    return result;
  }

  IndexedEnvironment environment_from_file(std::filesystem::path const &p) {
    IndexedEnvironment result{};
    logger::scope_logger raii_log{logger::development_trace,"environment_from_file"};
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
              EnvironmentIdValuePair{current_index[name], to_environment_value(value_string)});
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

Environment to_environment(IndexedEnvironment const& indexed_environment) {
  return indexed_environment;
}

Environment environment_from_file(std::filesystem::path const &p) {
  return to_environment(in::environment_from_file(p));
}

namespace out {
  std::ostream& operator<<(std::ostream& os,EnvironmentValue const& ev) {
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

  std::ostream& operator<<(std::ostream& os,IndexedEnvironment const& env) {
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

  std::string to_string(IndexedEnvironmentValue const& ev) {
    std::ostringstream os{};
    os << ev;
    return os.str();
  }

  std::string to_string(IndexedEnvironment::value_type const& entry) {
    std::ostringstream os{};
    os << entry;
    return os.str();
  }

  void indexed_environment_to_file(IndexedEnvironment const &indexed_environment,std::filesystem::path const &p) {
    try {
      std::ofstream out{p};
      out << indexed_environment;
    } catch (std::runtime_error const &e) {
      logger::design_insufficiency(R"(ERROR - Write to {} failed. Exception: {})", p.string(), e.what());
    }
  }

} // out

IndexedEnvironment to_indexed_environment(Environment const& environment) {
  // TODO: Implement transform from hash-index to integer index 
  return environment;
}

void environment_to_file(Environment const &environment,std::filesystem::path const &p) {
  return out::indexed_environment_to_file(to_indexed_environment(environment),p);
}
