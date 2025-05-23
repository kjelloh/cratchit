#include "environment.hpp"
#include "tokenize.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

bool is_value_line(std::string const& line) {
    return (line.size()==0)?false:(line.substr(0,2) != R"(//)");
}

// Split <key> on ':' into <name> and <id>
// <name> is 'type' or 'category' (e.g., 'HeadingAmountDateTransEntry',
// 'TaggedAmount', 'contact', 'employee', 'sie_file')) <id> is basically a hash
// of the <value> following the key (to identify duplicate values based on same
// id)
std::pair<std::string, std::optional<EnvironmentValueId>> to_name_and_id(std::string key) {
  if (false) {
    std::cout << "\nto_name_and_id(" << std::quoted(key) << ")";
  }
  if (auto pos = key.find(':'); pos != std::string::npos) {
    auto name = key.substr(0, pos);
    auto id_string = key.substr(pos + 1);
    // std::cout << "\n\tname:" << std::quoted(name) << " id_string:" <<
    // std::quoted(id_string);
    std::istringstream is{id_string};
    EnvironmentValueId id{};
    if (is >> std::hex >> id) {
      // std::cout << " ok id:" << std::hex << id;
      return {name, id};
    } else {
      std::cout << "\nDESIGN_INSUFFICIENCY: Failed to parse the value id after "
                   "':' in "
                << std::quoted(key);
      return {name, std::nullopt};
    }
  } else {
    // std::cout << " NO id in name";
    return {key, std::nullopt};
  }
}

// "belopp=1389,50;datum=20221023;rubrik=Klarna"
EnvironmentValue to_environment_value(std::string const s) {
  EnvironmentValue result{};
  auto kvps = tokenize::splits(s, ';');
  for (auto const &kvp : kvps) {
    auto const &[name, value] = tokenize::split(kvp, '=');
    result[name] = value;
  }
  return result;
}

// HeadingAmountDateTransEntry "belopp=1389,50;datum=20221023;rubrik=Klarna"
Environment environment_from_file(std::filesystem::path const &p) {
  Environment result{};
  try {
    std::ifstream in{p};
    std::string line{};
    std::map<std::string, std::size_t> index{};
    while (std::getline(in, line)) {
      if (is_value_line(line)) {
        std::istringstream in{line};
        std::string key{}, value_string{};
        in >> std::hex >> key >> std::quoted(value_string);
        auto const &[name, id] = to_name_and_id(key);
        if (id) {
          index[name] = *id;
          // std::cout << "\nRead name:index " << name << ":" <<
          // std::hex << index[name] << " for environment file entry "
          // << std::quoted(line);
        } else {
          index[name] = result[key].size();
          // std::cout << "\nNo index in environment file for name: " <<
          // name << ". Created index:" << ":" << index[name] << " for
          // environment file entry " << std::quoted(line);
        }
        // Each <name> maps to a list of pairs <index,environment value>
        // Where <index> is the index recorded in the file (to preserve order
        // to and from persisten storage in file) Also, <index> is basicvally
        // the hash of the value (expected to be unique cross all <names>
        // (types) of values) And No, I don't think this is a nice solution...
        // but it works for now.
        result[name].push_back(
            // EnvironmentValue is key-value-pairs as a map <key,value>
            // result is Environment, i.e.,
            {index[name], to_environment_value(value_string)});
      }
      /*
              TaggedAmount:b648074203011604
         "SIE=E19;_members=7480a07d8d1d605a^566b0d67be34b960;cents_amount=108375;type=aggregate;vertext=Account:NORDEA
         Text:LOOPIA AB (WEBSUPPORT) Message:62500003193947
         To:5343-2795;yyyymmdd_date=20250331"

              Environment['TaggedAmount']
                     |
                         |--> [b648074203011604]
                                     |
                                                 |--> ['SIE] = 'E19'
                                                 |--> ['_members'] =
         '7480a07d8d1d605a^566b0d67be34b960'
                                                 |--> ['cents_amount'] =
         '108375'
                                                 ...

      */
    }
  } catch (std::runtime_error const &e) {
    std::cout << "\nDESIGN_INSUFFICIENCY:ERROR - Read from " << p
              << " failed. Exception:" << e.what();
  }
  if (false) {
    std::cout << "\nenvironment_from_file(" << p << ")";
    for (auto const &[key, entry] : result) {
      std::cout << "\n\tkey:" << key << " count:" << entry.size();
    }
  }
  return result;
}
