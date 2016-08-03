//
// Created by kjell-olovhogdahl on 7/27/2016.
//

/**
 * This is the FrontEnd in the Frontend-Core-Backend source code architecture idiom.
 *
	FrontEnd.cpp	<—	FronEnd.h	—>	FrontEndImpl.h	->	FrontEndImpl.cpp
	    ^					            ^
	    |----------------------			    |
			  	  |			    |
	Core.cpp	<—	Core.h		->	CoreImpl.h	->	CoreImpl.cpp
	    ^						    ^
	    |----------------------			    |
			  	  |			    |

	BackEnd.cpp	<—	BackEnd.h	->	BackendImpl.h	->	BackendImpl.cpp
	    ^			\___________________________________/		      ^
	    |					 |				      |
	    |				     Isolated				      |
	    |									      |
	    ---------------------------------------------------------------------------
						 |
					Build Environment
					    Platform
 */

#include "FrontEnd.h"
#include "Core.h"
#include "BackEnd.h"
#include "Active.h"
#include <iostream>
#include <regex>

namespace tris {
	namespace frontend {

		namespace detail {
			tris::backend::API_STRING file_name(const tris::backend::API_STRING& s) {
				tris::backend::API_STRING result("?console?");
				std::regex rgx(R"(\w*(?=.exe$))"); // Match any word character before (look ahead excluded) ".exe" at end of line
				std::smatch match;
				if (std::regex_search(s.begin(), s.end(), match, rgx)) {
					result = match[0]; // only one match expected
				}
				return result;
			}
		}


	} // namespace frontend {

	struct FrontEnd<frontend::Console>::impl {
		tris::backend::API_STRING m_name;
		tris::Core m_core;
		// FrontEnd private Implementation (Not in header)
		impl(const tris::backend::API_STRING& name) : m_name(name) {}
	};

	FrontEnd<frontend::Console>::FrontEnd(const tris::backend::API_STRING& sExe) : m_pimpl(new impl(frontend::detail::file_name(sExe))) {}

	FrontEnd<frontend::Console>::~FrontEnd()
	{
	}

	int FrontEnd<frontend::Console>::run() {
		int result = 0;
		std::cout << "\n" << m_pimpl->m_name << ">";
		std::cout << R"(Enter "quit" to exit)";
		bool done = false;
		std::string sCommandLine;
		while (!done) {
			std::cout << "\n" << m_pimpl->m_name << ">";
			std::getline(std::cin, sCommandLine);
			if (this->execute(sCommandLine, done)) {
				// ok
			}
			else if (sCommandLine.find("quit") != std::string::npos) {
				done = true;
			}
			else {
				this->help(sCommandLine);
			}
		}
		return result;
	}

	bool FrontEnd<frontend::Console>::execute(const tris::backend::API_STRING& sCommandLine, bool& done) {
		return false;
	}

	bool FrontEnd<frontend::Console>::help(const tris::backend::API_STRING& sCommandLine) {
		if (sCommandLine == "") {
			std::cout << R"(Enter "quit" to exit)";
		}
		else {
			std::cout << "\nUnknown Command";
		}
		return true;
	}

} // namespace tris {
