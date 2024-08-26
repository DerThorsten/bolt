# CMakeLists.txt for tests


add_executable(bolt_tests main.cpp
    test_bolt.cpp
    #test_value.cpp
)

# Link the bolt library and doctest to the test executable
target_link_libraries(bolt_tests PRIVATE bolt)



# Download and include doctest (if not installed globally)
find_package(doctest REQUIRED)


target_link_libraries(bolt_tests PRIVATE doctest::doctest bolt)