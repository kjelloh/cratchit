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

struct Model {
    std::string prompt{};
	bool quit{};
};

using Command = std::string;
struct Quit {};
struct Nop {};

using Msg = std::variant<Nop,Quit,Command>;

using Ux = std::vector<std::string>;

struct Updater {
	Model const& model;
	Model operator()(Command const& command) {
		Model result{model};
		result.prompt += "\nUpdate for command not yet implemented";
		result.prompt += "\n>";
		return result;
	}
	Model operator()(Quit const& quit) {
		Model result{model};
		result.prompt += "\nBy for now :)";
		result.quit = true;
		return result;
	}
	Model operator()(Nop const& nop) {return model;}
};

class Environment {
public:
	Environment(std::filesystem::path const& p) : environment_file_path{p} {}
	Model init() {
		Model result{};
		result.prompt += "\nInit from ";
		result.prompt += environment_file_path;
		return result;
	}
	Model update(Msg const& msg,Model const& model) {
		Updater updater{model};
		return std::visit(updater,msg);
	}
	Ux view(Model const& model) {
		Ux result{};
		result.push_back(model.prompt);
		return result;
	}
private:
	std::filesystem::path environment_file_path{};
};

class REPL {
public:
    REPL(std::filesystem::path const& environment_file_path,Command const& command) : environment{environment_file_path} {
        this->model = environment.init();
        in.push_back(Command{command});
    }
    bool operator++() {
        Msg msg{Nop{}};
        if (in.size()>0) {
            msg = in.back();
            in.pop_back();
        }
        // if (std::holds_alternative<Quit>(msg)) return false;
        this->model = environment.update(msg,this->model);
        auto ux = environment.view(this->model);
        for (auto const&  row : ux) std::cout << row;
		if (this->model.quit) return false; // Done
        Command user_input{};
        std::getline(std::cin,user_input);
        in.push_back(to_msg(user_input));
        return true;
    }
private:
    Msg to_msg(Command const& user_input) {
        if (user_input == "quit" or user_input=="q") return Quit{};
        else return Command{user_input};
    }
    Model model{};
    Environment environment;
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