project(matrixChain)
cmake_minimum_required(VERSION 3.2)
set(CMAKE_CXX_FLAGS "-Wall -pedantic -O3")
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -pedantic -O0 -g")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Arch: ${CMAKE_SYSTEM_PROCESSOR}")

option(VERBOSE "debug" OFF)
if (VERBOSE)
  add_definitions("-DDEBUG")
endif (VERBOSE)

add_subdirectory(external/googletest EXCLUDE_FROM_ALL)

add_library(matrixChain
  chain.cpp
  properties.cpp
  utils.cpp
)

add_executable(main
  main.cpp
)

target_include_directories(matrixChain PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(main matrixChain)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")
include(sanitizers)

add_subdirectory(test)
#add_subdirectory(docs)
