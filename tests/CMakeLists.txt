function(crdt_test)
  cmake_parse_arguments(CRDT_TEST "" "NAME" "" ${ARGN})

  string(TOUPPER ${CRDT_TEST_NAME} SUIT)
  string(CONCAT SUIT_TEST ${SUIT} "_SUIT")
  string(CONCAT CRDT_TEST_SRC ${CRDT_TEST_NAME} ".cpp")

  add_executable(${CRDT_TEST_NAME} "${CRDT_TEST_SRC}")
  target_link_libraries(${CRDT_TEST_NAME} crdtxx rapidcheck)
  add_test(NAME ${SUIT_TEST} COMMAND ${CRDT_TEST_NAME})
endfunction()

crdt_test(NAME "dot_test")

crdt_test(NAME "version_vector_test")

crdt_test(NAME "gcounter_test")

crdt_test(NAME "pncounter_test")

crdt_test(NAME "lwwreg_test")

crdt_test(NAME "mvreg_test")

crdt_test(NAME "gset_test")

crdt_test(NAME "orswot_test")

crdt_test(NAME "ormwot_test")
