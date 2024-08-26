cmake_minimum_required(VERSION 3.20)

# Project name and version
project(bolt VERSION 1.0 LANGUAGES CXX)


# Add include directory
# include_directories(include)

# Find all source files in src directory
file(GLOB_RECURSE BOLT_SOURCES src/*.cpp)

# Create the library
add_library(bolt STATIC ${BOLT_SOURCES})

# Optionally, you can set the target include directories
target_include_directories(bolt PUBLIC include)


# set the C++ standard to C++20 for the library
target_compile_features(bolt PUBLIC cxx_std_20)

# Optionally, you can add compiler options or definitions
# target_compile_options(bolt PRIVATE -Wall -Wextra -pedantic)
# target_compile_definitions(bolt PRIVATE SOME_DEFINITION)

# Optionally, install the library and headers
install(TARGETS bolt DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

enable_testing()
add_subdirectory(tests)
