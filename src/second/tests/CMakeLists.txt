# This is the CMAKELists.txt for the test_cratchit binary.
# The test_cratchit is a google test based executable based on goggle test main
# and selected parts from cratchit/second to test

#--------------------------------------------------
# BEGIN: STATIC LIB of code to test
#--------------------------------------------------
# set a conveniant alias for our test binary target
set(LIB_TO_TEST_NAME test_cratchit_lib)

# Tell cmake to activate (?) the cmake testing framework
enable_testing()

# Integrate with conan package manager (or however we bring in external dependancies)
# We need Google Test (wherever it happens to be installed / located)
# Note: I find it opaque how cmake perfoerms th search, but anyhow...
find_package(GTest REQUIRED)

# conveniant listing or sources to compile and link to static lib of code to test
set(SourcesToTest
  ${CMAKE_SOURCE_DIR}/src/second/parse_archive.cpp
)

# Bundle parts we are to test into a static library
add_library(${LIB_TO_TEST_NAME} STATIC
  ${SourcesToTest}
)

# I suppose we need to tell cmake to configure the tool chain
# to know whaty dirs to span in search of include files 
# Otherwise '#include ...' will fail?
target_include_directories(${LIB_TO_TEST_NAME}
    PUBLIC
        ${CMAKE_SOURCE_DIR}/second
)

#--------------------------------------------------
# END: STATIC LIB of code to test
#--------------------------------------------------

#--------------------------------------------------
# BEGIN: Actual test app based on google test
#--------------------------------------------------

# Conveniant alias for our actual test app name
set(THIS_TEST_APP_NAME cratchit_test)

# Define source code that defines our actual tests
# Note: Paths are relative to the location of this CMakeLists.txt!
#       It seems ${CMAKE_SOURCE_DIR} is the location of the top CMakeLists.txt?
#       I also asume it is set when 'project' is invoked?
set(TestAppSources
  # ${CMAKE_SOURCE_DIR}/src/second/tests/src/TemplateTests.cpp
  src/TemplateTests.cpp
)

# define our app with test case source files
add_executable(${THIS_TEST_APP_NAME}
  ${TestAppSources}
)

# Link our app with google test main entry point
target_link_libraries(${THIS_TEST_APP_NAME} PRIVATE
    test_cratchit_lib
    GTest::gtest_main
)

#--------------------------------------------------
# END: Actual test app based on google test
#--------------------------------------------------

# Ensure there is a 'build/Debug/src/second/tests/CTestTestfile.cmake' file with content
add_test(
  NAME ${THIS_TEST_APP_NAME}
  COMMAND ${THIS_TEST_APP_NAME}
)

# Now we can do '>ctest --test-dir build/Debug/src/second/tests'
# Note: The folder name 'build' is determined by the argument given to cmake to build.
#       The folder name 'Debug' is determined by the build type applied by cmake built tool chain
#       The path 'src/second/tests' is applied by cmake due to the relative location
#       of this child CMakeLists.txt!