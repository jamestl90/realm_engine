cmake_minimum_required(VERSION 3.20)
project(rfd_game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC: Force synchronous PDB writes to avoid C1041 errors during parallel builds
if(MSVC)
    add_compile_options(/FS)
endif()

# External dependencies
set(SDL3_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/include")
set(SDL3_LIB_DIR "${CMAKE_SOURCE_DIR}/external/lib/SDL3/x64")
set(SDL3_TTF_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/include/SDL3_ttf")
set(SDL3_TTF_LIB_DIR "${CMAKE_SOURCE_DIR}/external/lib/SDL3_ttf/x64")
set(RAPIDJSON_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/include/rapidjson") # Adjust path if needed

# Automatically find all .cpp files in src directory
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(rfd_game ${SOURCES})

# Include directories
target_include_directories(rfd_game PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${SDL3_INCLUDE_DIR}
    ${SDL3_TTF_INCLUDE_DIR}
    ${RAPIDJSON_INCLUDE_DIR}
)

# Link SDL3 libraries
target_link_directories(rfd_game PRIVATE 
    ${SDL3_LIB_DIR}
    ${SDL3_TTF_LIB_DIR}
)
target_link_libraries(rfd_game PRIVATE SDL3 SDL3_ttf)

# Copy SDL3 DLLs to output directory
add_custom_command(TARGET rfd_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_LIB_DIR}/SDL3.dll"
        "${SDL3_TTF_LIB_DIR}/SDL3_ttf.dll"
        $<TARGET_FILE_DIR:rfd_game>
)
