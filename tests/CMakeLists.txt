add_executable(dot_test dot_test.cpp)
target_link_libraries(dot_test crdtxx rapidcheck)
add_test(NAME DotTestSuite COMMAND dot_test)

add_executable(version_vector_test version_vector_test.cpp)
target_link_libraries(version_vector_test crdtxx rapidcheck)
add_test(NAME VectorTestSuite COMMAND version_vector_test)
