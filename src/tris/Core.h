//
// Created by kjell-olovhogdahl on 7/27/2016.
//

#ifndef CLANG_BCC_CORE_H
#define CLANG_BCC_CORE_H

/**
 *  This is the Core header in the FrontEnd-Core-BackEnd source code architecture idiom

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

#include "Active.h"
#include <memory>

namespace tris {
	namespace core {
	}

	class Core : public Active {
	public:
		Core();
		~Core(); // delay to cpp so that m_pimpl is also delayed until struct impl is defined
		Core(const Core&) = delete;
		Core& operator=(const Core&) = delete;
	private:
		struct impl;
		std::unique_ptr<impl> m_pimpl;
	};

}

#endif //CLANG_BCC_CORE_H
