#include <iostream>
#include <string>
#include <algorithm>
#include <iterator>
#include <regex>
#include <vector>


using CommandLine = std::string;
using Parameter = std::string;
using SIE_Element = std::string;
using SIE_Entry = std::vector<SIE_Element>;
using SIE_Verification = std::vector<SIE_Entry>;
struct CratchCommands {};

template <typename CommandWorld>
class ExecuteCommand {}; // Anonumous template never used

template <>
class ExecuteCommand<CratchCommands> {
public:

	bool operator()(const CommandLine& command_line) {
		bool result = false;
		std::regex tokenizer_regexp("\\s+"); // whitespace
		std::for_each(
			std::sregex_token_iterator(std::begin(command_line), std::end(command_line), tokenizer_regexp, -1)
			, std::sregex_token_iterator()
			, [&result](const Parameter& p) {
			if (p == "q") { result = true;}
			else if (p == "Telia") {
				// Test of SIE output generation
				/*
				==> Telia Faktura 160517

				==> Tidigare Telia-faktura verkar vara bokförd så här (Verifikat D:42 kodad som #VER	4	42)

				#FNR	"ITFIED"
				#FNAMN	"The ITfied AB"
				#ORGNR	"556782-8172"
				#RAR	0	20150501	20160430

				#VER	4	42	20151116	"FA407/Telia"	20160711
				{
				#TRANS	2440	{}	-1330,00	""	"FA407/Telia"
				#TRANS	2640	{}	265,94	""	"FA407/Telia"
				#TRANS	6212	{}	1064,06	""	"FA407/Telia"
				}
				*/
				int state = 0;
				SIE_Verification verification;
				bool loop_again = true;
				while (loop_again) {
					SIE_Entry entry;
					switch (state) {					
					case 0: verification.push_back(SIE_Entry({ "#FNR","ITFIED"}));
					case 1: verification.push_back(SIE_Entry({ "#FNAMN","The ITfied AB" }));
					case 2: verification.push_back(SIE_Entry({ "#ORGNR","556782-8172" }));
					case 3: verification.push_back(SIE_Entry({ "#RAR","0","20150501","20160430" }));
					case 4: verification.push_back(SIE_Entry({ "" }));
					case 5: verification.push_back(SIE_Entry({ "#VER","4","42","20151116","FA407/Telia","20160711" }));
					case 6: verification.push_back(SIE_Entry({ "{" }));
					case 7: verification.push_back(SIE_Entry({ "#TRANS","2440","{}","-1330,00","","FA407/Telia" }));
					case 8: verification.push_back(SIE_Entry({ "#TRANS","2640","{}","265,94","","FA407/Telia" }));
					case 9: verification.push_back(SIE_Entry({ "#TRANS","6212","{}","1064,06","","FA407/Telia" }));
					case 10: verification.push_back(SIE_Entry({ "}" }));
					default: loop_again = false; break;
					}
					++state;
				}
				std::for_each(std::begin(verification), std::end(verification), [](const SIE_Entry& entry) {
					std::cout << "\n";
					std::for_each(std::begin(entry), std::end(entry), [](const SIE_Element& element) {
						std::cout << "\t\"" << element << "\"";
					});
				});
			}
		});

		return result;
	}

};

int main(int argc, char *argv[]){
	std::cout << "\ncratchit> Welcome!";
	std::cout << "\ncratchit> ...Enter 'q' to quit.";
	ExecuteCommand<CratchCommands> execute_cratch_command;
	bool quit=false;
	while (!quit) {
		CommandLine sCommand;
		std::cout << "\ncratchit>";
		std::getline(std::cin, sCommand);
		quit = execute_cratch_command(sCommand);
	}
	std::cout << "\ncratchit> Bye!";
	return 0;
}