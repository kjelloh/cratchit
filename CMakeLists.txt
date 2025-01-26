cmake_minimum_required(VERSION 3.15)
project(cratchit CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)  # Ensure it's strictly enforced

find_package(lua)
add_executable(cratchit src/cratchit.cpp src/main.cpp)
target_link_libraries(cratchit lua::lua)

install(TARGETS cratchit DESTINATION "."
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        )
