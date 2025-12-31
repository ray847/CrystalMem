cmake_minimum_required(VERSION 3.15)

if (NOT TARGET foonathan_memory)

  include(FetchContent)

  FetchContent_Declare(
    foonathan_memory
    GIT_REPOSITORY https://github.com/foonathan/memory.git
    GIT_TAG v0.7-4
  )
  FetchContent_MakeAvailable(foonathan_memory)
endif()

