cmake_minimum_required(VERSION 3.20)
project(rfd_game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# SDL3 paths
set(SDL3_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/include")
set(SDL3_LIB_DIR "${CMAKE_SOURCE_DIR}/external/lib/SDL3/x64")

# Debug path information
message(STATUS "CHECK: Project Source Dir is: ${CMAKE_SOURCE_DIR}")
message(STATUS "CHECK: SDL3 Include Path is: ${SDL3_INCLUDE_DIR}")
message(STATUS "CHECK: SDL3 Lib Path is: ${SDL3_LIB_DIR}")

add_executable(rfd_game 
    src/main.cpp
)

target_include_directories(rfd_game PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${SDL3_INCLUDE_DIR}
)

target_link_directories(rfd_game PRIVATE ${SDL3_LIB_DIR})
target_link_libraries(rfd_game PRIVATE SDL3)

# Copy SDL3.dll to output
add_custom_command(TARGET rfd_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_LIB_DIR}/SDL3.dll"
        $<TARGET_FILE_DIR:rfd_game>
)
