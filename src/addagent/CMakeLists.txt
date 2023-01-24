cmake_minimum_required(VERSION 3.12.4)

project(addagent)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

enable_testing()

get_filename_component(SRC_FOLDER     ${CMAKE_SOURCE_DIR}/../ ABSOLUTE)

if(COVERITY)
  add_definitions(-D__GNUC__=8)
endif(COVERITY)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wshadow -Wnon-virtual-dtor -Woverloaded-virtual -Wunused -Wcast-align -Wformat=2 -std=c++14")

set(CMAKE_CXX_FLAGS_DEBUG "-g")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3")
else()
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -s")
endif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")

if(FSANITIZE)
  set(CMAKE_CXX_FLAGS_DEBUG "-g -fsanitize=address,leak,undefined")
endif(FSANITIZE)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(${CMAKE_SOURCE_DIR}/)
include_directories(${SRC_FOLDER}/)
include_directories(${SRC_FOLDER}/headers/)
include_directories(${SRC_FOLDER}/external/sqlite/)
include_directories(${SRC_FOLDER}/external/nlohmann/)
include_directories(${SRC_FOLDER}/external/cJSON/)
include_directories(${SRC_FOLDER}/external/procps/)
include_directories(${SRC_FOLDER}/external/bzip2/)
include_directories(${SRC_FOLDER}/external/openssl/include/)
include_directories(${SRC_FOLDER}/shared_modules/utils)
include_directories(${SRC_FOLDER}/shared_modules/common/)
include_directories(${SRC_FOLDER}/external/openssl/include/)
include_directories(${SRC_FOLDER}/external/libplist/bin/include/)
include_directories(${SRC_FOLDER}/external/libdb/build_unix/)
if(NOT CMAKE_CHECK_CENTOS5) # Avoid incompatible libraries in CentOS 5 and Red Hat 5
include_directories(${SRC_FOLDER}/external/pacman/lib/libalpm/)
include_directories(${SRC_FOLDER}/external/libarchive/libarchive/)
include_directories(${SRC_FOLDER}/external/rpm/builddir/output/include/)
endif(NOT CMAKE_CHECK_CENTOS5)

link_directories(${SRC_FOLDER})
link_directories(${SRC_FOLDER}/external/sqlite/)
link_directories(${SRC_FOLDER}/external/cJSON/)
link_directories(${SRC_FOLDER}/external/procps/)
link_directories(${SRC_FOLDER}/external/bzip2/)
link_directories(${SRC_FOLDER}/external/libplist/bin/lib/)
link_directories(${SRC_FOLDER}/external/libdb/build_unix/)
if(NOT CMAKE_CHECK_CENTOS5) # Avoid incompatible libraries in CentOS 5 and Red Hat 5
link_directories(${SRC_FOLDER}/external/pacman/lib/libalpm/)
link_directories(${SRC_FOLDER}/external/libarchive/.libs/)
link_directories(${SRC_FOLDER}/external/rpm/builddir/)
endif(NOT CMAKE_CHECK_CENTOS5)
link_directories(${SRC_FOLDER}/external/openssl/)

file(GLOB ADDAGENT_SRC
  "${CMAKE_SOURCE_DIR}/manage_agents.c"
  "${CMAKE_SOURCE_DIR}/manage_keys.c"
  "${CMAKE_SOURCE_DIR}/read_from_user.c"
  "${CMAKE_SOURCE_DIR}/validate.c"
  )
add_definitions(-DLINUX_TYPE=LinuxType::STANDARD) # Standard compilation in compatible systems

add_library(addagent SHARED
  ${ADDAGENT_SRC}
  )

set_target_properties(addagent PROPERTIES
  LINK_FLAGS "-static-libgcc -static-libstdc++")

list(APPEND WRAPPER_FLAGS "-Wl,--wrap,print_agent_status")
list(APPEND WRAPPER_FLAGS "-Wl,--wrap,wdbc_close")
list(APPEND WRAPPER_FLAGS "-Wl,--wrap,OS_RemoveCounter")
list(APPEND WRAPPER_FLAGS "-Wl,--wrap,auth_remove_agent")
list(APPEND WRAPPER_FLAGS "-Wl,--wrap,decode_base64")
list(APPEND WRAPPER_FLAGS "-Wl,--wrap,auth_close")
list(APPEND WRAPPER_FLAGS "-Wl,--wrap,OS_AddKey")
list(APPEND WRAPPER_FLAGS "-Wl,--wrap,wdbc_query_ex")
list(APPEND WRAPPER_FLAGS "-Wl,--wrap,OS_MD5_Str")
list(APPEND WRAPPER_FLAGS "-Wl,--wrap,auth_connect")
list(APPEND WRAPPER_FLAGS "-Wl,--wrap,get_agent_status")
list(APPEND WRAPPER_FLAGS "-Wl,--wrap,wdb_remove_agent")
list(APPEND WRAPPER_FLAGS "-Wl,--wrap,encode_base64")
list(APPEND WRAPPER_FLAGS "-Wl,--wrap,os_random")
list(APPEND WRAPPER_FLAGS "-Wl,--wrap,delete_agentinfo")
list(APPEND WRAPPER_FLAGS "-Wl,--wrap,wdb_get_agent_info")
list(APPEND WRAPPER_FLAGS "-Wl,--wrap,w_request_agent_add_local")
list(APPEND WRAPPER_FLAGS "-Wl,--wrap,mkstemp_ex")
list(APPEND WRAPPER_FLAGS "-Wl,--wrap,rename_ex")
list(APPEND WRAPPER_FLAGS "-Wl,--wrap,read_from_user")
  
if(NOT CMAKE_SYSTEM_NAME STREQUAL "AIX")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,-rpath=$ORIGIN")
else()
  string(REPLACE ";" ":" CXX_IMPLICIT_LINK_DIRECTORIES_STR "${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES}")
  string(REPLACE ";" ":" PLATFORM_REQUIRED_RUNTIME_PATH_STR "${CMAKE_PLATFORM_REQUIRED_RUNTIME_PATH}")
  target_link_libraries(addagent -Wl,-blibpath:${INSTALL_PREFIX}/lib:${CXX_IMPLICIT_LINK_DIRECTORIES_STR}:${PLATFORM_REQUIRED_RUNTIME_PATH_STR})
endif(NOT CMAKE_SYSTEM_NAME STREQUAL "AIX")

target_link_libraries(addagent wazuhext wazuhshared)

target_link_libraries(addagent ${WRAPPER_FLAGS})

add_subdirectory(tests)
