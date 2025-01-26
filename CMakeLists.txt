cmake_minimum_required(VERSION 3.15)
project(cratchit VERSION 1.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the Lua version to download
set(LUA_VERSION 5.4.4)
set(LUA_URL https://www.lua.org/ftp/lua-${LUA_VERSION}.tar.gz)
set(LUA_SOURCE_DIR ${CMAKE_BINARY_DIR}/lua-src)

# Download Lua source if not already downloaded
include(FetchContent)
FetchContent_Declare(
    lua
    URL ${LUA_URL}
    SOURCE_DIR ${LUA_SOURCE_DIR}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(lua)

# List Lua source files
file(GLOB LUA_SOURCES
    ${LUA_SOURCE_DIR}/src/*.c
)

# Exclude Lua's CLI tools (optional)
list(REMOVE_ITEM LUA_SOURCES
    ${LUA_SOURCE_DIR}/src/lua.c
    ${LUA_SOURCE_DIR}/src/luac.c
)

# Add Lua as a static library
add_library(lua STATIC ${LUA_SOURCES})

# Add Lua's include directory
target_include_directories(lua PUBLIC ${LUA_SOURCE_DIR}/src)

# Add platform-specific libraries (e.g., math library for Linux/macOS)
if(UNIX)
    target_link_libraries(lua PRIVATE m)
endif()

# Add the Cratchit executable
add_executable(cratchit src/main.cpp)

# Link Lua to Cratchit
target_link_libraries(cratchit PRIVATE lua)

# Ensure everything builds
add_dependencies(cratchit lua)
