cmake_minimum_required(VERSION 3.22)
project(gkandroid C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build gk as a static library from the existing sources.
set(GK_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gk gk_build)

add_library(native-lib SHARED native-lib.c sim-lib.c)
target_link_libraries(native-lib PRIVATE gk)
