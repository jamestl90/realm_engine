cmake_minimum_required(VERSION 3.20)
project(rfd_game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# SDL3 include
include_directories(external/SDL3/include)

# SDL3 library path
link_directories(external/SDL3/lib/x64)

add_executable(rfd_game 
    src/main.cpp
)

target_include_directories(rfd_game PRIVATE include)

target_link_libraries(rfd_game PRIVATE SDL3)

# Copy SDL3.dll to output
add_custom_command(TARGET rfd_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/external/SDL3/lib/x64/SDL3.dll"
        $<TARGET_FILE_DIR:rfd_game>
)
