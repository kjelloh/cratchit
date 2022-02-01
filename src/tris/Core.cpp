//
// Created by kjell-olovhogdahl on 7/27/2016.
//

/**
 *  This is the Core unit in the FrontEnd-Core-BackEnd source code architecture idiom

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

#include "Core.h"
#include "BackEnd.h"

namespace tris {
	namespace core {

	} // namespace core {

	struct Core::impl {
		tris::BackEnd m_back_end;
	};


	Core::Core() : m_pimpl(new impl())
	{
	}

	Core::~Core()
	{
	}

} // namespace tris {

