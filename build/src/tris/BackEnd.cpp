//
// Created by kjell-olovhogdahl on 7/27/2016.
//

/**
 * This is the BackEnd.h header in the FrontEnd-Core-BackEnd source code architecture idiom
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
#ifdef DWORD
	#error "Windows header is leaking into client code (check Backend.h includes to not bring in Windows headers)"
#endif
#include <sstream>
#include <typeinfo>
#include <iostream>
#include <algorithm>

#ifdef _WIN32
	#include <windows.h>
#endif

namespace tris {
	namespace backend {

		namespace platform_detail {
			// platform tags
			struct win32 {};
		}

#ifdef _WIN32
		using platform = platform_detail::win32;
#endif

		namespace detail {
			
			template <typename T>
			std::runtime_error runtime_exception_of_api_error_code(const std::string sPrefix, API_ERROR_CODE api_error_code) {
				static_assert(is_not_instantiated<T>::value, "runtime_exception_of_api_error_code_impl not implemented for this platform");
			}

#ifdef _WIN32
			template <>
			std::runtime_error runtime_exception_of_api_error_code<platform_detail::win32>(const std::string sPrefix, API_ERROR_CODE api_error_code) {
				std::stringstream ssMessage;
				ssMessage << sPrefix << " Error ";
				ssMessage << api_error_code << " : \"";

				LPTSTR lpMsgBuf = nullptr; // Asume not UNICODE

				if (FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM |
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					api_error_code,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR)&lpMsgBuf,
					0, NULL) == 0) {
					ssMessage << "#??#";
				}
				else {
					ssMessage << lpMsgBuf << "\"";
				}

				return std::runtime_error(ssMessage.str());
			}

		} // namespace detail
#endif

		std::runtime_error runtime_exception_of_api_error_code(const std::string sPrefix, API_ERROR_CODE api_error_code) {
			return detail::runtime_exception_of_api_error_code<platform>(sPrefix, api_error_code);
			// return detail::runtime_exception_of_api_error_code<int>(sPrefix, api_error_code); // Test compile time error of unimplemented platform
		}

		namespace process {

			template <typename T>
			class Process {
			public:
				Process(const Path& cmd, const Parameters& parameters) {
					static_assert(is_not_instantiated<T>::value, "class Process not implemented for this platform");
				}

				API_RESULT_CODE waitTerminate();
			};

#ifdef _WIN32
			template <>
			class Process<platform_detail::win32> {
			public:
				Process(const Process&) = delete; // No copy
				Process& operator=(const Process&) = delete; // No assign

				Process(const Path& cmd, const Parameters& parameters) {
					// Init m_STARTUPINFO
					{
						m_STARTUPINFO.cb = sizeof(&m_STARTUPINFO);
						m_STARTUPINFO.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
						m_STARTUPINFO.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
						m_STARTUPINFO.hStdError = GetStdHandle(STD_ERROR_HANDLE);
						m_STARTUPINFO.dwFlags |= STARTF_USESTDHANDLES; // Execute in parent IO environment (stdin, stdout, stderr)
					}
					// Init m_PROCESS_INFORMATION
					{
						// Already C++11 zero-initialised in definition
					}

					std::stringstream ssCmdLine;
					ssCmdLine << R"(")" << cmd << R"(")"; // Ensure path is within string delimiters to preserve any spaces for Linux shells (MSYS2, MINGW, CywWin)
					std::for_each(std::begin(parameters), std::end(parameters), [&ssCmdLine](Parameter p) {ssCmdLine << " " << p; });
					std::string sCmdLine(ssCmdLine.str().c_str()); // copy to provide stack instance to API

					std::cout << "\n[[CLANG-BCC]]:CreateProcess=" << sCmdLine;
					std::cout << "\n" << std::flush; // Flush our entries to stdout before letting child process do its stuff
													 // Start the child process.
					if (!CreateProcess(
						NULL,   // No module name (use command line)
						const_cast<LPSTR>(sCmdLine.c_str()),        // Command line
						NULL,           // Process handle not inheritable
						NULL,           // Thread handle not inheritable
						TRUE, // bInheritHandles
						0,
						NULL,           // Use parent's environment block
						NULL,           // Use parent's starting directory
						&m_STARTUPINFO,         // Pointer to STARTUPINFO structure
						&m_PROCESS_INFORMATION) // Pointer to PROCESS_INFORMATION structure
						)
					{
						//throw app::win32::runtime_exception_of_win_error_code(std::string("CreateProcess ") + sCmdLine + " Failed", GetLastError());
						throw backend::runtime_exception_of_api_error_code(std::string("CreateProcess ") + sCmdLine + " Failed", GetLastError());
					}
				}

				API_RESULT_CODE waitTerminate() {
					API_RESULT_CODE result = 0;
					WaitForSingleObject(m_PROCESS_INFORMATION.hProcess, INFINITE);
					if (!GetExitCodeProcess(m_PROCESS_INFORMATION.hProcess, &result)) {
					}
					return result;
				}

			private:
				STARTUPINFO m_STARTUPINFO{}; // C++11 zero initialized
				PROCESS_INFORMATION m_PROCESS_INFORMATION{}; // C++11 zero initialized
			};
#endif

			std::future<int> execute(const Path& cmd, const Parameters& parameters) {
				// Launch deferred i.e., synchronosuly on client access to returned future.
				// NOTE: Lamda captures by value to have a copy available on actual call (after return from this call)
				// This ensures the std::cout made by Process class is not mixed up by concurrent execution with clinet std::cout calls
				// TODO: Ensure Process execution is "thread safe" before chaning launch policy to async?
				return std::async(std::launch::deferred, [cmd, parameters]()->int {
					Process<platform> process(cmd, parameters);
					// Process<int> process(cmd, parameters); // Test of static_assert on unimplemented platform
					return process.waitTerminate();
				});
			}
		} // namespace process

	} // namespace backend {

	struct BackEnd::impl {
		// Hidden implementation details of BackEnd
	};

	BackEnd::BackEnd() : m_pimpl(new impl())
	{
	}

	BackEnd::~BackEnd()
	{
	}

} // namespace tris {

