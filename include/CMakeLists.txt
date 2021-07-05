add_library(crdt++ INTERFACE)
target_include_directories(crdt++ INTERFACE .)
target_compile_features(crdt++ INTERFACE cxx_std_20)

add_executable(dot_test dot_test.cpp)
target_include_directories(dot_test INTERFACE .)
target_compile_features(dot_test INTERFACE cxx_std_20)
target_link_libraries(dot_test rapidcheck)
add_test(NAME DotTestSuite COMMAND dot_test)
cotire(dot_test)

add_executable(version_vector_test version_vector_test.cpp)
target_include_directories(version_vector_test INTERFACE .)
target_compile_features(version_vector_test INTERFACE cxx_std_20)
target_link_libraries(version_vector_test rapidcheck)
add_test(NAME VectorTestSuite COMMAND version_vector_test)
cotire(version_vector_test)

setup_target_for_coverage_gcovr_html(NAME coverage
                                     EXECUTABLE dot_test version_vector_test
                                     DEPENDENCIES dot_test)
