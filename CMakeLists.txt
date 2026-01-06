cmake_minimum_required(VERSION 3.20)
project(rfd_game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SDL3 include
include_directories(external/SDL3/include)

# SDL3 library path
link_directories(external/SDL3/lib/x64)

add_executable(rfd_game 
    src/main.cpp
)

target_include_directories(rfd_game PRIVATE include)

# Link SDL3 to your executable
target_link_libraries(rfd_game SDL3 SDL3)

# ---------- Copy SDL3.dll to output ----------
add_custom_command(TARGET rfd_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/external/SDL3/lib/x64/SDL3.dll"
        $<TARGET_FILE_DIR:rfd_game>
)cmake_minimum_required(VERSION 3.20)
project(SDL3GameEngine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add SDL3 (assuming it's in external/SDL3)
add_subdirectory(external/SDL3)

# Engine library
add_library(engine STATIC
    src/core/Engine.cpp
    src/core/Time.cpp
    src/ecs/World.cpp
)

target_include_directories(engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(engine PUBLIC
    SDL3::SDL3
)

# Main executable
add_executable(game
    src/main.cpp
)

target_link_libraries(game PRIVATE
    engine
)

# Copy SDL3 DLL on Windows
if(WIN32)
    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:game>
    )
endif()
