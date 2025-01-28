cmake_minimum_required(VERSION 3.15)
project(cratchit CXX)



add_executable(cratchit src/cratchit.cpp src/main.cpp)



install(TARGETS cratchit DESTINATION "."
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        )
