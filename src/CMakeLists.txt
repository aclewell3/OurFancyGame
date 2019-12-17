cmake_minimum_required (VERSION 3.10.2 FATAL_ERROR)
project(Game)

enable_language(C)
enable_language(CXX)

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

if(${APPLE}) 
  message("Include Mac directories")
  message(${CMAKE_CURRENT_SOURCE_DIR})
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
  set (CMAKE_CXX_STANDARD 17)
endif()

if(VERBOSE) 
  add_definitions(-DVERBOSE)
endif()

add_subdirectory(app)
