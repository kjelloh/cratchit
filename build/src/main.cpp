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
		bool result = true; // Default success
		std::regex tokenizer_regexp("\\s+"); // whitespace
		std::for_each(
			std::sregex_token_iterator(std::begin(command_line), std::end(command_line), tokenizer_regexp, -1)
			, std::sregex_token_iterator()
			, [&result](const Parameter& p) {
			if (p == "test") {
				sie::experimental::get_template_statements();
			}
			else if (p == "bokslut Apr-14") { // Bokslut Maj-13...Apr-14
				// Serie A
				{
					/*
					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20130501	20140430

					#VER	1	113	20140430	"�terf�ring hyra april.14"	20160729
					{
					#TRANS	5010	{}	8892,00	""	"�terf�ring hyra april.14"
					#TRANS	1710	{}	-8892,00	""	"�terf�ring hyra april.14"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20130501	20140430

					#VER	1	114	20140430	"�terf. juli-sept. 2013"	20160729
					{
					#TRANS	5010	{}	25898,00	""	"�terf. juli-sept. 2013"
					#TRANS	1710	{}	-25898,00	""	"�terf. juli-sept. 2013"
					}
					*/
				}
				// Serie I
				{
					/*
					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20130501	20140430

					#VER	9	1	20140430	"Omf�ring l�neskuld 2911 - 2820"	20160729
					{
					#TRANS	2911	{}	28731,00	""	"Omf�ring l�neskuld 2911 - 2820"
					#TRANS	2893	{}	-28731,00	""	"Omf�ring l�neskuld 2911 - 2820"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20130501	20140430

					#VER	9	2	20140430	"Avskrivningar"	20160729
					{
					#TRANS	1229	{}	-9485,00	""	"Avskrivningar"
					#TRANS	7832	{}	9485,00	""	"Avskrivningar"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20130501	20140430

					#VER	9	3	20140430	"Upplupna int�kter"	20160729
					{
					#TRANS	1790	{}	290860,00	""	"Upplupna int�kter"
					#TRANS	3590	{}	-290860,00	""	"Upplupna int�kter"
					#TRANS	1790	{}	7900,00	""	"F�rutbetald kostnad"
					#TRANS	5800	{}	-7900,00	""	"F�rutbetald kostnad"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20130501	20140430

					#VER	9	4	20140430	"�terf fg �rs interimer"	20160729
					{
					#TRANS	1790	{}	-132760,00	""	"�terf fg �rs interimer"
					#TRANS	3590	{}	122200,00	""	"�terf fg �rs interimer"
					#TRANS	4010	{}	10560,00	""	"�terf fg �rs interimer"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20130501	20140430

					#VER	9	5	20140430	"Omf fg �rs resultat"	20160729
					{
					#TRANS	2099	{}	219890,72	""	"Omf fg �rs resultat"
					#TRANS	2098	{}	-219890,72	""	"Omf fg �rs resultat"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20130501	20140430

					#VER	9	6	20140430	"Korr konto 2510 slutlig skatt"	20160729
					{
					#TRANS	2510	{}	-16242,00	""	"Korr konto 2510 slutlig skatt tax13"
					#TRANS	2514	{}	16242,00	""	"Korr konto 2510 slutlig skatt tax13"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20130501	20140430

					#VER	9	7	20140430	"L�ne- och avkastningsskatt"	20160729
					{
					#TRANS	2514	{}	-16455,00	""	"L�ne- och avkastningsskatt"
					#TRANS	7533	{}	16455,00	""	"L�ne- och avkastningsskatt"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20130501	20140430

					#VER	9	8	20140430	"Upplupna semesterl�ner"	20160729
					{
					#TRANS	2920	{}	-9331,00	""	"Upplupna semesterl�ner"
					#TRANS	7090	{}	9331,00	""	"Upplupna semesterl�ner"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20130501	20140430

					#VER	9	9	20140430	"Skuld sociala avgifter"	20160729
					{
					#TRANS	2940	{}	-2932,00	""	"Skuld sociala avgifter"
					#TRANS	7519	{}	2932,00	""	"Skuld sociala avgifter"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20130501	20140430

					#VER	9	10	20140430	"Uppl r�nta sparkonto"	20160729
					{
					#TRANS	1760	{}	1824,00	""	"Uppl r�nta sparkonto"
					#TRANS	8311	{}	-1824,00	""	"Uppl r�nta sparkonto"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20130501	20140430

					#VER	9	11	20140430	"Omf�ring konto 8314 till 8423"	20160729
					{
					#TRANS	8314	{}	-10,00	""	"Omf�ring konto 8314 till 8423"
					#TRANS	8423	{}	10,00	""	"Omf�ring konto 8314 till 8423"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20130501	20140430

					#VER	9	12	20140430	"F�r�ndring av periodiseringsfo"	20160729
					{
					#TRANS	2115	{}	-32104,00	""	"F�r�ndring av periodiseringsfond"
					#TRANS	8811	{}	32104,00	""	"F�r�ndring av periodiseringsfond"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20130501	20140430

					#VER	9	13	20140430	"�rets skattekostnad"	20160729
					{
					#TRANS	2510	{}	-21188,00	""	"�rets skattekostnad"
					#TRANS	8910	{}	21188,00	""	"�rets skattekostnad"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20130501	20140430

					#VER	9	14	20140430	"�rets resultat"	20160729
					{
					#TRANS	8999	{}	71250,62	""	"�rets resultat"
					#TRANS	2099	{}	-71250,62	""	"�rets resultat"
					}
					*/
				}
			}
			else if (p == "bokslut Apr-15") { // Bokslut Maj-14...Apr-15
				// Serie A
				{
					/*
					// Bokslut 150430
					<Serie A>

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20140501	20150430

					#VER	1	125	20150430	"R�ttning A113"	20160729
					{
					#TRANS	2893	{}	-85,00	""	"R�ttning A113"
					#TRANS	6071	{}	76,00	""	"R�ttning A113"
					#TRANS	2640	{}	9,00	""	"R�ttning A113"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20140501	20150430

					#VER	1	126	20150430	"Beanstalk 26"	20160729
					{
					#TRANS	1920	{}	-134,67	""	"Beanstalk 26"
					#TRANS	6230	{}	134,67	""	"Beanstalk 26"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20140501	20150430

					#VER	1	127	20150430	"�terf�ring hyra april.15"	20160729
					{
					#TRANS	1710	{}	-9159,00	""	"�terf�ring hyra april.15"
					#TRANS	5010	{}	9159,00	""	"�terf�ring hyra april.15"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20140501	20150430

					#VER	1	128	20150430	"Korr ver A112"	20160729
					{
					#TRANS	2893	{}	-222,00	""	"Korr ver A112"
					#TRANS	6071	{}	198,00	""	"Korr ver A112"
					#TRANS	2640	{}	24,00	""	"Korr ver A112"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20140501	20150430

					#VER	1	129	20150430	"�terf�ringar"	20160729
					{
					#TRANS	1710	{}	-17783,00	""	"�terf�ringar"
					#TRANS	5010	{}	8892,00	""	"�terf�ringar hyra maj 14"
					#TRANS	5010	{}	8891,00	""	"�terf�ringar hyra juni 14"
					#TRANS	1760	{}	-1824,00	""	"�terf�ringar"
					#TRANS	8311	{}	1824,00	""	"�terf�ringar"
					#TRANS	1790	{}	-298760,00	""	"�terf�ringar"
					#TRANS	3590	{}	103330,00	""	"�terf�ringar"
					#TRANS	3590	{}	94430,00	""	"�terf�ringar"
					#TRANS	3590	{}	93100,00	""	"�terf�ringar"
					#TRANS	5800	{}	7900,00	""	"�terf�ringar"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20140501	20150430

					#VER	1	130	20150430	"Upplupen int�kt"	20160729
					{
					#TRANS	1790	{}	189000,00	""	"Upplupen int�kt"
					#TRANS	3590	{}	-189000,00	""	"Upplupen int�kt"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20140501	20150430

					#VER	1	131	20150430	"fg �rs vinst"	20160729
					{
					#TRANS	2099	{}	71250,62	""	"fg �rs vinst"
					#TRANS	2098	{}	78749,38	""	"fg �rs vinst"
					#TRANS	2089	{}	-150000,00	""	"fg �rs vinst"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20140501	20150430

					#VER	1	132	20150430	"momsomf�ring april"	20160729
					{
					#TRANS	2640	{}	-374,40	""	"momsomf�ring april"
					#TRANS	2610	{}	43050,00	""	"momsomf�ring april"
					#TRANS	2650	{}	-42675,60	""	"momsomf�ring april"
					}

					*/
				}

				// Serie I
				{
					/*
					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20140501	20150430

					#VER	9	1	20150430	"Avskrivningar"	20160729
					{
					#TRANS	1229	{}	-8165,00	""	"Avskrivningar"
					#TRANS	7832	{}	8165,00	""	"Avskrivningar"
					#TRANS	8853	{}	1024,00	""	"Avskrivningar"
					#TRANS	2153	{}	-1024,00	""	"Avskrivningar"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20140501	20150430

					#VER	9	2	20150430	"L�ne- och avkastningsskatt"	20160729
					{
					#TRANS	2514	{}	-16597,00	""	"L�ne- och avkastningsskatt"
					#TRANS	7533	{}	16597,00	""	"L�ne- och avkastningsskatt"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20140501	20150430

					#VER	9	3	20150430	"Avrundning"	20160729
					{
					#TRANS	2650	{}	-0,40	""	"Avrundning"
					#TRANS	3740	{}	0,40	""	"Avrundning"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20140501	20150430

					#VER	9	6	20150430	"Korr a120"	20160729
					{
					#TRANS	2510	{}	-16455,00	""	"Korr a120"
					#TRANS	2514	{}	16455,00	""	"Korr a120"
					#TRANS	2820	{}	-28731,00	""	"Korr a120"
					#TRANS	2893	{}	28731,00	""	"Korr a120"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20140501	20150430

					#VER	9	7	20150430	"7010 -> 7210"	20160729
					{
					#TRANS	7010	{}	-39000,00	""	"7010 -> 7210"
					#TRANS	7210	{}	39000,00	""	"7010 -> 7210"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20140501	20150430

					#VER	9	8	20150430	"�rets skattekostnad"	20160729
					{
					#TRANS	2510	{}	-84950,00	""	"�rets skattekostnad"
					#TRANS	8910	{}	84950,00	""	"�rets skattekostnad"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20140501	20150430

					#VER	9	9	20150430	"�rets resultat"	20160729
					{
					#TRANS	8999	{}	298526,70	""	"�rets resultat"
					#TRANS	2099	{}	-298526,70	""	"�rets resultat"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20140501	20150430

					#VER	9	10	20150430	"Korr a61 moms"	20160729
					{
					#TRANS	5410	{}	-1598,00	""	"Korr a61 moms"
					#TRANS	2651	{}	1598,00	""	"Korr a61 moms"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20140501	20150430

					#VER	9	11	20150430	"F�r�ndring av periodiseringsfo"	20160729
					{
					#TRANS	2116	{}	-128713,00	""	"F�r�ndring av periodiseringsfond"
					#TRANS	8811	{}	128713,00	""	"F�r�ndring av periodiseringsfond"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20140501	20150430

					#VER	9	12	20150430	"Upplupna semesterl�ner"	20160729
					{
					#TRANS	2920	{}	8003,00	""	"Upplupna semesterl�ner"
					#TRANS	7090	{}	-8003,00	""	"Upplupna semesterl�ner"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20140501	20150430

					#VER	9	13	20150430	"Skuld sociala avgifter"	20160729
					{
					#TRANS	2940	{}	2515,00	""	"Skuld sociala avgifter"
					#TRANS	7519	{}	-2515,00	""	"Skuld sociala avgifter"
					}

					#FNR	"ITFIED"
					#FNAMN	"The ITfied AB"
					#ORGNR	"556782-8172"
					#RAR	0	20140501	20150430

					#VER	0	1	20140501	"Kontoavslut 2099 mot 2098"	20160729
					{
					#TRANS	2099	{}	-148640,10	""	""
					#TRANS	2098	{}	148640,10	""	""
					}

					*/

				}
			}
			else if (p == "Telia") {
				// Test of SIE output generation
				/*
				==> Telia Faktura 160517

				==> Tidigare Telia-faktura verkar vara bokf�rd s� h�r (Verifikat D:42 kodad som #VER	4	42)

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
				sie::SIE_Statement verification = sie::experimental::create_statement_for("Telia");

				//int state = 0;
				//SIE_Statement verification;
				//bool loop_again = true;
				//while (loop_again) {
				//	SIE_Entry entry;
				//	switch (state) {					
				//	case 0: verification.push_back(SIE_Entry({ "#FNR","ITFIED"}));
				//	case 1: verification.push_back(SIE_Entry({ "#FNAMN","The ITfied AB" }));
				//	case 2: verification.push_back(SIE_Entry({ "#ORGNR","556782-8172" }));
				//	case 3: verification.push_back(SIE_Entry({ "#RAR","0","20150501","20160430" }));
				//	case 4: verification.push_back(SIE_Entry({ "" }));
				//	case 5: verification.push_back(SIE_Entry({ "#VER","4","42","20151116","FA407/Telia","20160711" }));
				//	case 6: verification.push_back(SIE_Entry({ "{" }));
				//	case 7: verification.push_back(SIE_Entry({ "#TRANS","2440","{}","-1330,00","","FA407/Telia" }));
				//	case 8: verification.push_back(SIE_Entry({ "#TRANS","2640","{}","265,94","","FA407/Telia" }));
				//	case 9: verification.push_back(SIE_Entry({ "#TRANS","6212","{}","1064,06","","FA407/Telia" }));
				//	case 10: verification.push_back(SIE_Entry({ "}" }));
				//	default: loop_again = false; break;
				//	}
				//	++state;
				//}
				std::for_each(std::begin(verification), std::end(verification), [](const sie::SIE_Entry& entry) {
					std::cout << "\n";
					std::for_each(std::begin(entry), std::end(entry), [](const sie::SIE_Element& element) {
						std::cout << "\t\"" << element << "\"";
					});
				});
			}
			else if (p == "Binero") {
			}
			else if (p == "SE_Konsult") {
			}
			else if (p == "Kontoutdrag") { // Booking of payments (in/out)
			}
			else if (p == "TaxDeclaration") { // Clearing/declaration of Tax for period
			}
			else if (p == "Privat") {
			}
			else if (p == "Beanstalk") { // Could be sub-payment of "Kontoutdrag" (Shows only as payment)
			}
			else {
				result = false; // unknown command
			}
		});

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