cmake_minimum_required(VERSION 3.15)

# Project Options
project(CrystalMem CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# External Libraries
add_subdirectory(lib/CrystalBase)

# Internal Library
add_library(CrystalMem)
target_sources(CrystalMem
  PRIVATE
  src/memory.cpp
  src/header_only.cpp
  src/pool/slub/slub.cpp
  src/pool/safe_best_fit/safe_best_fit.cpp
)
target_include_directories(CrystalMem
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(CrystalMem
  PUBLIC
  CrystalBase
)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  message(STATUS "Building CrystalMem Standalone (with tests/main)")
  # Main
  add_executable(main main.cpp)
  target_include_directories(main
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  target_link_libraries(main
  PUBLIC
    CrystalMem
  )

  # Testing
  add_subdirectory(test)
endif()

