#include <iostream>
#include <deque>
#include <variant>
#include <vector>
#include <optional>
#include <string_view>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <random>
#include <cstdlib>
#include <cctype>
#include <map>
#include <regex>

namespace tokenize {
	// returns s split into first,second on provided delimiter delim.
	// split fail returns first = "" and second = s
	std::pair<std::string,std::string> split(std::string s,char delim) {
		auto pos = s.find(delim);
		if (pos<s.size()) return {s.substr(0,pos),s.substr(pos+1)};
		else return {"",s}; // split failed (return right = unsplit string)
	}

	std::vector<std::string> splits(std::string s,char delim) {
		std::vector<std::string> result;
		auto head_tail = split(s,delim);
		while (head_tail.first.size()>0) {
			auto const& [head,tail] = head_tail;
			std::cout << "\nhead:" << head << " tail:" << tail;
			result.push_back(head);
			head_tail = split(tail,delim);
		}
		if (head_tail.second.size()>0) result.push_back(head_tail.second);
		return result;
	}

	enum class SplitOn {
		Undefined
		,TextAndAmount
		,TextAmountAndDate
		,Unknown
	};

	std::pair<std::string,std::string> split(std::string const& s,SplitOn split_on) {
		std::pair<std::string,std::string> result{};
		switch (split_on) {
			case SplitOn::TextAndAmount: {
				auto tokens = splits(s,' ');
				float amount{};
				std::istringstream in{tokens.back()};
				if (in >> amount) {
					// OK, last token is a float
					auto left = std::accumulate(tokens.begin(),tokens.end()-1,std::string{},[](auto acc,auto const& s) {
						if (acc.size()>0) acc += " ";
						acc += s;
						return acc;
					});
					auto right = tokens.back();
					result = {left,right};
				}
				else {
					std::cerr << "\nERROR - Expected amount but read " << tokens.back(); 
				}
			} break;
			default: {
				std::cerr << "Error - Unknown split_on argument " << static_cast<int>(split_on);
			}
		}
		return result;
	}

	enum class TokenID {
		Undefined
		,Caption
		,Date
		,Amount
		,Unknown
	};

	TokenID token_id_of(std::string const& s) {
		const std::regex date_regex("[2-9][2-9]((0[1-9])|(1[0-2]))(0[1-9]|[1-3][0-9])");
		if (std::regex_match(s,date_regex)) return TokenID::Date;
		const std::regex amount_regex("\\d+,\\d\\d");
		if (std::regex_match(s,amount_regex)) return TokenID::Amount;
		const std::regex caption_regex("(\\w|\\s|[\\.-])+");
		if (std::regex_match(s,caption_regex)) return TokenID::Caption;
		return TokenID::Unknown; 
	}

	std::vector<std::string> splits(std::string const& s,SplitOn split_on) {
		std::vector<std::string> result{};
		auto spaced_tokens = splits(s,' ');
		std::vector<TokenID> ids{};
		for (auto const& s : spaced_tokens) {
			ids.push_back(token_id_of(s));
		}
		for (int i=0;i<spaced_tokens.size();++i) {
			std::cout << "\n" << spaced_tokens[i] << " id:" << static_cast<int>(ids[i]);
		}
		switch (split_on) {
			case SplitOn::TextAmountAndDate: {
				std::vector<TokenID> expected_id{TokenID::Caption,TokenID::Amount,TokenID::Date};
				int state{0};
				std::string s{};
				for (int i=0;i<ids.size();) {
					if (ids[i] == expected_id[state]) {
						if (s.size() > 0) s += " ";
						s += spaced_tokens[i++];
					}
					else {
						if (s.size()>0) result.push_back(s);
						++state;	
						s.clear();					
					}
				}
				if (s.size()>0) result.push_back(s);
			} break;
			default: {
				std::cerr << "\nERROR - Unknown split_on value " << static_cast<int>(split_on);
			} break;
		}
		return result;
	}
}

namespace parse {
    using In = std::string_view;

    template <typename P>
    using ParseResult = std::optional<std::pair<P,In>>;

    template <typename P>
    class Parse {
    public:
        virtual ParseResult<P> operator()(In const& in) const = 0;
    };

    class ParseWord : public Parse<std::string> {
    public:
        using P = std::string;
        virtual ParseResult<P> operator()(In const& in) const {
            ParseResult<P> result{};
            P p{};
            auto iter=in.begin();
            for (;iter!=in.end();++iter) if (delimeters.find(*iter)==std::string::npos) break; // skip delimeters
            for (;iter!=in.end();++iter) {
                if (delimeters.find(*iter)!=std::string::npos) break;
                p += *iter;
            }
            if (p.size()>0) result = {p,In{iter,in.end()}}; // empty word = unsuccessful
            return result;
        }
    private:
        std::string const delimeters{" ,.;:="};
    };

    template <typename T>
    auto parse(T const& p,In const& in) {
        return p(in);
    }
}

using EnvironmentValue = std::map<std::string,std::string>;
using Environment = std::map<std::string,EnvironmentValue>;
std::string to_string(EnvironmentValue const& ev) {
	// Expand to variants when EnvironmentValue is no longer a simple string (if ever?)
	std::string result = std::accumulate(ev.begin(),ev.end(),std::string{},[](auto acc,auto const& entry){
		if (acc.size()>0) acc += ";"; // separator
		acc += entry.first;
		acc += ":";
		acc += entry.second;
		return acc;
	});
	return result;
}
EnvironmentValue to_value(std::string const s) {
	EnvironmentValue result{};
	auto kvps = tokenize::splits(s,';');
	for (auto const& kvp : kvps) {
		auto const& [name,value] = tokenize::split(kvp,':');
		result[name] = value;
	}
	return result;
}
std::string to_string(Environment::value_type const& entry) {
	std::ostringstream os{};
	os << std::quoted(entry.first);
	os << ":";
	os << std::quoted(to_string(entry.second));
	return os.str();
}

struct Model {
    std::string prompt{};
	bool quit{};
	Environment environment{};
};

using Command = std::string;
struct Quit {};
struct Nop {};

using Msg = std::variant<Nop,Quit,Command>;

using Ux = std::vector<std::string>;

std::vector<std::string> help_for(std::string topic) {
	std::vector<std::string> result{};
	result.push_back("Enter 'Quit' or 'q' to quit");
	return result;
}

// custom specialization of std::hash can be injected in namespace std
template<>
struct std::hash<EnvironmentValue>
{
    std::size_t operator()(EnvironmentValue const& ev) const noexcept {
		std::size_t result{};
		for (auto const& v : ev) {
			std::size_t h = std::hash<std::string>{}(v.second); // Todo: Subject to change if variant values are introduced
			result ^= h << 1; // Note: Shift left before XOR is from example code but I am not sure exactly whay this is "better" than plain XOR?
		}
		return result;
    }
};

struct Updater {
	Model& model;
	Model& operator()(Command const& command) {
		std::ostringstream os{};
		auto tokens = tokenize::splits(command,tokenize::SplitOn::TextAmountAndDate);			
		if (tokens.size()==3) {
			EnvironmentValue ev{};
			ev["rubrik"] = tokens[0];
			ev["belopp"] = tokens[1];
			ev["datum"] = tokens[2];
			auto entry_key = std::to_string(std::hash<EnvironmentValue>{}(ev));
			model.environment[entry_key] = ev;
		}
		else {
			os << "\nERROR - Expected Caption + Amount + Date";
		}
		os << "\ncratchit:";
		model.prompt = os.str();
		return model;
	}
	Model& operator()(Quit const& quit) {
		std::ostringstream os{};
		os << "\nBy for now :)";
		model.prompt = os.str();
		model.quit = true;
		return model;
	}
	Model& operator()(Nop const& nop) {
		std::ostringstream os{};
		auto help = help_for("");
		for (auto const& line : help) os << "\n" << line;
		os << "\ncratchit:";
		model.prompt = os.str();
		return model;
	}
};

class Cratchit {
public:
	Cratchit(std::filesystem::path const& p) 
		: cratchit_file_path{p} {}
	Model init() {
		Model result{};
		result.prompt += "\nInit from ";
		result.prompt += cratchit_file_path;
		result.environment = environment_from_file(cratchit_file_path);
		return result;
	}
	Model& update(Msg const& msg,Model& model) {
		Updater updater{model};
		Model& result = std::visit(updater,msg); // Pass Model& around
		if (result.quit) {
			environment_to_file(result.environment,cratchit_file_path);
		}
		return result;
	}
	Ux view(Model& model) {
		Ux result{};
		result.push_back(model.prompt);
		return result;
	}
private:
	std::filesystem::path cratchit_file_path{};
	bool is_value_line(std::string const& line) {
		return (line.size()==0)?false:(line.substr(0,2) != R"(//)");
	}
	Environment environment_from_file(std::filesystem::path const& p) {
		Environment result{};
		try {
			std::ifstream in{p};
			std::string line{};
			while (std::getline(in,line)) {
				if (is_value_line(line)) {
					// Read <name>:<value> where <name> is "bla bla" and <value> is "bla bla " or "StringWithNoSpaces"
					std::istringstream in{line};
					std::string name{},value{};
					char colon{};
					in >> std::quoted(name) >> colon >> std::quoted(value);
					result.insert({name,to_value(value)});
				}
			}
		}
		catch (std::runtime_error const& e) {
			std::cerr << "\nERROR - Write to " << p << " failed. Exception:" << e.what();
		}
		return result;
	}
	void environment_to_file(Environment const& environment, std::filesystem::path const& p) {
		try {
			std::ofstream out{p};		
			for (auto iter = environment.begin(); iter != environment.end();++iter) {
				if (iter == environment.begin()) out << to_string(*iter);
				else out << "\n" << to_string(*iter);
			}
		}
		catch (std::runtime_error const& e) {
			std::cerr << "\nERROR - Write to " << p << " failed. Exception:" << e.what();
		}
	}
};

class REPL {
public:
    REPL(std::filesystem::path const& environment_file_path,Command const& command) : cratchit{environment_file_path} {
        this->model = cratchit.init();
        in.push_back(Command{command});
    }
    bool operator++() {
        Msg msg{Nop{}};
        if (in.size()>0) {
            msg = in.back();
            in.pop_back();
        }
        this->model = cratchit.update(msg,this->model);
        auto ux = cratchit.view(this->model);
        for (auto const&  row : ux) std::cout << row;
		if (this->model.quit) return false; // Done
		else {
			Command user_input{};
			std::getline(std::cin,user_input);
			in.push_back(to_msg(user_input));
			return true;
		}
    }
private:
    Msg to_msg(Command const& user_input) {
        if (user_input == "quit" or user_input=="q") return Quit{};
		else if (user_input.size()==0) return Nop{};
        else return Command{user_input};
    }
    Model model{};
    Cratchit cratchit;
    std::deque<Msg> in{};
};

int main(int argc, char *argv[])
{
    std::string sPATH{std::getenv("PATH")};
    std::cout << "\nPATH=" << sPATH;
    std::string command{};
    for (int i=1;i<argc;i++) command+= std::string{argv[i]} + " ";
    auto current_path = std::filesystem::current_path();
    auto environment_file_path = current_path / "cratchit.env";
    REPL repl{environment_file_path,command};
    while (++repl);
    // std::cout << "\nBye for now :)";
    std::cout << std::endl;
    return 0;
}