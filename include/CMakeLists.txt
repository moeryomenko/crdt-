add_library(dCRDT INTERFACE)
target_include_directories(dCRDT INTERFACE .)
target_compile_features(dCRDT INTERFACE cxx_std_20)

add_executable(dot_test dot_test.cpp)
target_include_directories(dot_test INTERFACE .)
target_compile_features(dot_test INTERFACE cxx_std_20)
target_link_libraries(dot_test rapidcheck)
add_test(NAME TestSuite COMMAND dot_test)
setup_target_for_coverage_gcovr_html(NAME coverage EXECUTABLE dot_test
                                     DEPENDENCIES dot_test)
cotire(dot_test)

