cmake_minimum_required(VERSION 3.23)
project(cratchit CXX)

# Set the C++ standard version
set(CMAKE_CXX_STANDARD 23)  # or 20, 23, etc.
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # Ensures that the compiler supports the requested version
set(CMAKE_CXX_EXTENSIONS OFF)  # Disable compiler-specific extensions (e.g., -std=c++11 or -std=gnu++11)

message(STATUS "C++ Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")

# Add a cache variable with default value (empty means no limit)
set(MAX_COMPILE_ERRORS "" CACHE STRING "Maximum number of compilation errors to show")

# Apply the flag if the variable is set
if(MAX_COMPILE_ERRORS)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-fmax-errors=${MAX_COMPILE_ERRORS})
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-ferror-limit=${MAX_COMPILE_ERRORS})
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        add_compile_options(/errorReport:${MAX_COMPILE_ERRORS})
    endif()
    message(STATUS "Compile error limit set to: ${MAX_COMPILE_ERRORS}")
else()
    message(STATUS "No compile error limit set (unlimited errors)")
endif()

find_package(sol2 REQUIRED)
find_package(Curses REQUIRED)
find_package(pugixml REQUIRED)
find_package(icu REQUIRED)
find_package(immer REQUIRED)
find_package(spdlog REQUIRED)
find_package(GTest REQUIRED)

add_executable(cratchit 
        src/cratchit.cpp 
        src/cratchit_tea.cpp
        src/tea/ncurses_head.cpp
        src/tea/test_head.cpp
        src/main.cpp 
        src/Mod10View.cpp
        src/msgs/msg.cpp
        src/cmd.cpp
        src/sub.cpp
        src/states/StateImpl.cpp
        src/states/HADState.cpp
        src/states/HADsState.cpp
        src/states/VATReturnsState.cpp
        src/states/AccountStatementsState.cpp
        src/states/CSVFileState.cpp
        src/states/ProjectState.cpp
        src/states/WorkspaceState.cpp
        src/states/FrameworkState.cpp
        src/tokenize.cpp
        src/environment.cpp
        src/FiscalPeriod.cpp
        src/fiscal/amount/AmountFramework.cpp 
        src/fiscal/amount/TaggedAmountFramework.cpp
        src/test/test_runner.cpp
        src/test/test_fixtures.cpp
        src/test/test_integrations.cpp
        src/test/test_atomics.cpp
        src/fiscal/BASFramework.cpp
        src/fiscal/amount/HADFramework.cpp
        src/Key.cpp
        src/fiscal/SKVFramework.cpp
        src/cargo/CargoBase.cpp
        src/states/FiscalPeriodState.cpp
        src/text/charset.cpp
        src/text/encoding.cpp
        src/sie/sie.cpp
        src/csv/csv.cpp
        src/states/TaggedAmountsState.cpp
        src/states/TaggedAmountState.cpp
        src/RuntimeEncoding.cpp
        src/csv/projections.cpp)

# TODO: Understand how to make a conan profile or cmake preset for ASAN Debug build?
# target_compile_options(cratchit PRIVATE -fsanitize=address)
# target_link_options(cratchit PRIVATE -fsanitize=address)

target_include_directories(cratchit PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(cratchit PRIVATE sol2::sol2 Curses::Curses)
target_link_libraries(cratchit PRIVATE Curses::Curses)
target_link_libraries(cratchit PRIVATE pugixml::pugixml)
target_link_libraries(cratchit PRIVATE ICU::i18n ICU::uc)
target_link_libraries(cratchit PRIVATE immer::immer)
target_link_libraries(cratchit PRIVATE spdlog::spdlog)
target_link_libraries(cratchit PRIVATE gtest::gtest)

install(TARGETS cratchit DESTINATION "."
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        )

# Try to log the actual compiler options
get_target_property(cxx_flags cratchit COMPILE_OPTIONS)
message(STATUS "Compile options for target cratchit: ${cxx_flags}")
get_target_property(cxx_features cratchit INTERFACE_COMPILE_FEATURES)
message(STATUS "Compile features for target cratchit: ${cxx_features}")
message(STATUS "CMake C++ Standard: ${CMAKE_CXX_STANDARD}")
