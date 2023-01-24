cmake_minimum_required(VERSION 3.12.4)

project(unit_tests)

include_directories(${CMAKE_SOURCE_DIR})

include_directories(${SRC_FOLDER}/external/googletest/googletest/include/)
include_directories(${SRC_FOLDER}/external/googletest/googlemock/include/)
link_directories(${SRC_FOLDER}/external/googletest/lib/)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  string(APPEND CMAKE_EXE_LINKER_FLAGS " -fprofile-arcs ")
else()
  string(APPEND CMAKE_EXE_LINKER_FLAGS " -lgcov ")
endif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")

set(CMAKE_CXX_FLAGS_DEBUG "-g --coverage")

file(GLOB UNIT_TEST_SRC
    "*.cpp")

file(GLOB OBJS
#  "${SRC_FOLDER}/unit_tests/wrappers/common.o"
  )

set_source_files_properties(
    ${OBJS}
    PROPERTIES
    EXTERNAL_OBJECT true
    GENERATED true
    )

add_executable(addagent_unit_test
    ${UNIT_TEST_SRC}
    ${OBJS}
    )

target_link_libraries(addagent_unit_test
    addagent
    debug gtestd
    debug gmockd
    debug gtest_maind
    debug gmock_maind
    optimized gtest
    optimized gmock
    optimized gtest_main
    optimized gmock_main
    pthread
    sqlite3
    cjson
)
target_link_libraries(addagent_unit_test ${WRAPPER_FLAGS})

add_test(NAME addagent_unit_test
         COMMAND addagent_unit_test)


