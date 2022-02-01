//
// Created by kjell-olovhogdahl on 7/27/2016.
//

#ifndef CLANG_BCC_FRONTEND_H
#define CLANG_BCC_FRONTEND_H

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

#include "BackEnd.h"
namespace tris {
	namespace frontend {

		struct Console {}; // Console tag

	}

	template <typename Tag>
	class FrontEnd { // Primary template class never used
		static_assert(true, "Provided Frontend tag not supported");
	};

	template <>
	class FrontEnd<tris::frontend::Console> { // The Console Frontend
	public:
		FrontEnd(const tris::backend::API_STRING& sExe);
		~FrontEnd();
		FrontEnd(const FrontEnd&) = delete;
		FrontEnd& operator=(const FrontEnd&) = delete;
		int run();

		virtual bool execute(const tris::backend::API_STRING& sCommandLine, bool& done);
		virtual bool help(const tris::backend::API_STRING& sCommandLine);

	private:
		struct impl;
		std::unique_ptr<impl> m_pimpl;
	};

}

#endif //CLANG_BCC_FRONTEND_H
