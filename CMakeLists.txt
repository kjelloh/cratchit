cmake_minimum_required(VERSION 3.23)
project(cratchit CXX)

# Set the C++ standard version
set(CMAKE_CXX_STANDARD 23)  # or 20, 23, etc.
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # Ensures that the compiler supports the requested version
set(CMAKE_CXX_EXTENSIONS OFF)  # Disable compiler-specific extensions (e.g., -std=c++11 or -std=gnu++11)

find_package(sol2 REQUIRED)
find_package(Curses REQUIRED)
find_package(pugixml REQUIRED)
find_package(icu REQUIRED)
find_package(immer REQUIRED)
find_package(spdlog REQUIRED)

add_executable(cratchit src/cratchit.cpp src/main.cpp)

target_link_libraries(cratchit PRIVATE sol2::sol2 Curses::Curses)
target_link_libraries(cratchit PRIVATE Curses::Curses)
target_link_libraries(cratchit PRIVATE pugixml::pugixml)
target_link_libraries(cratchit PRIVATE ICU::i18n ICU::uc)
target_link_libraries(cratchit PRIVATE immer::immer)
target_link_libraries(cratchit PRIVATE spdlog::spdlog)

install(TARGETS cratchit DESTINATION "."
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        )
