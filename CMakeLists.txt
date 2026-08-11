cmake_minimum_required(VERSION 3.20)
project(rfd_game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(RFD_BUILD_TESTS "Create engine test targets" OFF)

set(RFD_ENABLE_PROCGEN_DEBUG_VIEW_DEFAULT ON)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(RFD_ENABLE_PROCGEN_DEBUG_VIEW_DEFAULT OFF)
endif()
option(RFD_ENABLE_PROCGEN_DEBUG_VIEW "Build the procgen debug map view into the game executable" ${RFD_ENABLE_PROCGEN_DEBUG_VIEW_DEFAULT})

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

set(RFD_ASSET_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets")
set(RFD_SHADER_ASSETS
    "${RFD_ASSET_SOURCE_DIR}/Shaders/sprite.vert.spv"
    "${RFD_ASSET_SOURCE_DIR}/Shaders/sprite.frag.spv"
    "${RFD_ASSET_SOURCE_DIR}/Shaders/sprite.vert.reflect.json"
    "${RFD_ASSET_SOURCE_DIR}/Shaders/sprite.frag.reflect.json"
)
set(RFD_FONT_ASSETS
    "${RFD_ASSET_SOURCE_DIR}/Fonts/Inter-4.1/extras/ttf/Inter-Regular.ttf"
    "${RFD_ASSET_SOURCE_DIR}/Fonts/Inter-4.1/extras/ttf/Inter-SemiBold.ttf"
    "${RFD_ASSET_SOURCE_DIR}/Fonts/Inter-4.1/LICENSE.txt"
)

# Automatically find all .cpp files in src directory
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

# Procgen visualization and its application panel are omitted from normal release builds.
set(RFD_PROCGEN_DEBUG_SOURCES
    "${CMAKE_SOURCE_DIR}/src/procgen/GreaterRealmDebug.cpp"
    "${CMAKE_SOURCE_DIR}/src/game/GreaterRealmDebugPanel.cpp"
)
list(REMOVE_ITEM SOURCES ${RFD_PROCGEN_DEBUG_SOURCES})
if(RFD_ENABLE_PROCGEN_DEBUG_VIEW)
    list(APPEND SOURCES ${RFD_PROCGEN_DEBUG_SOURCES})
endif()

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

if(RFD_ENABLE_PROCGEN_DEBUG_VIEW)
    target_compile_definitions(rfd_game PRIVATE RFD_ENABLE_PROCGEN_DEBUG_VIEW=1)
endif()

# Bundle runtime dependencies and the explicit runtime asset allowlist.
add_custom_command(TARGET rfd_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:rfd_game>/assets/Shaders"
        "$<TARGET_FILE_DIR:rfd_game>/assets/Fonts"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_LIB_DIR}/SDL3.dll"
        "${SDL3_TTF_LIB_DIR}/SDL3_ttf.dll"
        $<TARGET_FILE_DIR:rfd_game>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${RFD_SHADER_ASSETS}
        "$<TARGET_FILE_DIR:rfd_game>/assets/Shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${RFD_FONT_ASSETS}
        "$<TARGET_FILE_DIR:rfd_game>/assets/Fonts"
)

if(RFD_BUILD_TESTS)
    enable_testing()

    add_executable(rfd_engine_tests EXCLUDE_FROM_ALL
        tests/procgen/GreaterRealmTests.cpp
        src/procgen/GreaterRealm.cpp
        src/procgen/GreaterRealmDebug.cpp
    )

    target_include_directories(rfd_engine_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_compile_definitions(rfd_engine_tests PRIVATE RFD_TEST_BUILD=1)

    add_test(NAME procgen_greater_realm COMMAND rfd_engine_tests)

    add_executable(rfd_ecs_tests EXCLUDE_FROM_ALL
        tests/ecs/WorldLifecycleTests.cpp
        src/ecs/World.cpp
    )

    target_include_directories(rfd_ecs_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_compile_definitions(rfd_ecs_tests PRIVATE RFD_TEST_BUILD=1)

    add_test(NAME ecs_world_lifecycle COMMAND rfd_ecs_tests)

    add_executable(rfd_ecs_system_tests EXCLUDE_FROM_ALL
        tests/ecs/WorldSystemResourceTests.cpp
        src/ecs/World.cpp
    )

    target_include_directories(rfd_ecs_system_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_compile_definitions(rfd_ecs_system_tests PRIVATE RFD_TEST_BUILD=1)

    add_test(NAME ecs_system_resources COMMAND rfd_ecs_system_tests)

    add_executable(rfd_ecs_query_tests EXCLUDE_FROM_ALL
        tests/ecs/WorldQueryTests.cpp
        src/ecs/World.cpp
    )

    target_include_directories(rfd_ecs_query_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_compile_definitions(rfd_ecs_query_tests PRIVATE RFD_TEST_BUILD=1)

    add_test(NAME ecs_component_queries COMMAND rfd_ecs_query_tests)

    add_executable(rfd_ui_tests EXCLUDE_FROM_ALL
        tests/ui/TextMeasurementTests.cpp
        src/ui/Button.cpp
        src/ui/ComboBox.cpp
        src/ui/FocusableControl.cpp
        src/ui/InputSurface.cpp
        src/ui/Primitives.cpp
        src/ui/Style.cpp
        src/ui/TextBox.cpp
        src/ui/UIElement.cpp
    )

    target_include_directories(rfd_ui_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${SDL3_INCLUDE_DIR}
    )

    target_compile_definitions(rfd_ui_tests PRIVATE RFD_TEST_BUILD=1)

    add_test(NAME ui_text_measurement COMMAND rfd_ui_tests)
endif()
