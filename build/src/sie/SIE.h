//
// Created by kjell-olovhogdahl on 8/1/2016.
//

#ifndef CRATCHIT_SIE_H
#define CRATCHIT_SIE_H

#include <string>
#include <vector>

namespace sie {

	using SIE_Element = std::string;
	using SIE_Entry = std::vector<SIE_Element>;
	using SIE_Statement = std::vector<SIE_Entry>;
	using SIE_Statements = std::vector<SIE_Statement>;

	namespace experimental {
		sie::SIE_Statement create_statement_for(const SIE_Element& statement_label);

		SIE_Statements get_template_statements();

	}

}


#endif //CRATCHIT_SIE_H
