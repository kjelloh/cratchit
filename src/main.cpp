#include <iostream>
#include <string>
#include <algorithm>
#include <iterator>
#include <regex>
#include <vector>
#include "sie/SIE.h"
#include "tris/FrontEnd.h"

using CommandLine = std::string;
using Parameter = std::string;
struct CratchCommands {};

template <typename CommandWorld>
class ExecuteCommand {}; // Anonumous template never used

template <>
class ExecuteCommand<CratchCommands> {
public:

	ExecuteCommand() = default;

	// Returns true if Done
	bool operator()(const CommandLine& command_line, bool& done) {
		std::cout << "\nCratchit Command Executor here :)";
		bool result = false; // Default failed
		std::regex whitespace_regexp("\\s+"); // tokenize on whitespace
		sie::SIE_Seed sie_seed(std::sregex_token_iterator(std::begin(command_line), std::end(command_line), whitespace_regexp, -1), std::sregex_token_iterator());
		if (sie_seed.size() > 0) {
			auto p = sie_seed[0];
			if (p == "test") {
				//sie::experimental::get_template_statements();
				sie::SIE_Statements statements = sie::experimental::parse_financial_events();
				// format statements to std::cout
				std::for_each(std::begin(statements), std::end(statements), [](const sie::SIE_Statement& statement) {
					std::for_each(std::begin(statement), std::end(statement), [](const sie::SIE_Entry& entry) {
						std::cout << "\n";
						std::for_each(std::begin(entry), std::end(entry), [](const sie::SIE_Element& element) {
							if (std::any_of(std::begin(element), std::end(element), [](char c) {return c == ' '; })) {
								std::cout << "\t\"" << element << "\""; // frame text
							}
							else {
								std::cout << "\t" << element << ""; // Don't frame value
							}
						});
					});
				});
				result = true; // Command Done 
			}
			else {
				// Process it as a SIE seed (to generate SIE statement)
				sie::SIE_Statement statement = sie::experimental::create_statement_for(sie_seed);
				// Print to stdout
				std::for_each(std::begin(statement), std::end(statement), [](const sie::SIE_Entry& entry) {
					std::cout << "\n";
					std::for_each(std::begin(entry), std::end(entry), [](const sie::SIE_Element& element) {
						if (std::any_of(std::begin(element), std::end(element), [](char c) {return c == ' '; })) {
							std::cout << "\t\"" << element << "\""; // frame text
						}
						else {
							std::cout << "\t" << element << ""; // Don't frame value
						}
					});
				});
				result = (statement.size() > 0); // Ok if we generated a statement
			}
		}
		return result;
	}

};

class CractchitConsoleFrontEnd : public tris::FrontEnd<tris::frontend::Console> {
public:
    CractchitConsoleFrontEnd(const tris::backend::API_STRING& sExe) : tris::FrontEnd<tris::frontend::Console>(sExe) {}

    virtual bool execute(const tris::backend::API_STRING& sCommandLine,bool& done) {
        return m_execute_cratch_command(sCommandLine,done);
    }

    virtual bool help(const tris::backend::API_STRING& sCommandLine) {
		return tris::FrontEnd<tris::frontend::Console>::help(sCommandLine);
	}

private:
    ExecuteCommand<CratchCommands> m_execute_cratch_command;

};

using ActualFrontEnd = CractchitConsoleFrontEnd;

int main(int argc, char *argv[]){
    int result = 0;
    try {
        auto front_end = std::make_shared<ActualFrontEnd>(argv[0]);
        front_end->run();
    }
    catch (std::runtime_error& e) {
        std::cout << "\nFailed. Exception = " << e.what();
        result = 1; // Failed
    }
    std::cout << "\nDone!";
    std::cout << "\n";
    return result;
}