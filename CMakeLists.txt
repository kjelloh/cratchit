cmake_minimum_required(VERSION 3.15)
project(cratchit CXX)




add_library(cratchit src/cratchit.cpp)
target_include_directories(cratchit PUBLIC include)



set_target_properties(cratchit PROPERTIES PUBLIC_HEADER "include/cratchit.h")
install(TARGETS cratchit)
