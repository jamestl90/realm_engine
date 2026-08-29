cmake_minimum_required(VERSION 3.20)
project(realm_engine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(REALM_BUILD_TESTS "Create engine test targets" OFF)

option(REALM_ENABLE_PROCGEN_DEBUG_VIEW "Build procgen inspection views into the runtime executable" ON)
option(REALM_OPTIMIZE_PROCGEN_DEBUG "Optimize procgen sources in Debug builds for interactive tooling" ON)

# MSVC: Force synchronous PDB writes to avoid C1041 errors during parallel builds
if(MSVC)
    add_compile_options(/FS)
endif()

# Fetch pinned third-party sources so a clean clone does not depend on a
# manually populated external/ directory. The opt-out exists for offline
# development and validates compatibility with the repository's former layout.
option(REALM_FETCH_DEPENDENCIES "Download pinned third-party dependencies" ON)

if(REALM_FETCH_DEPENDENCIES)
    include(FetchContent)

    set(SDL_SHARED ON CACHE BOOL "Build SDL as a shared library" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "Do not build a duplicate static SDL library" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "Do not build SDL's test library" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "Do not build SDL's tests" FORCE)
    FetchContent_Declare(
        SDL3
        URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.0/SDL3-3.4.0.tar.gz
        URL_HASH SHA256=082cbf5f429e0d80820f68dc2b507a94d4cc1b4e70817b119bbb8ec6a69584b8
        DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    )
    FetchContent_MakeAvailable(SDL3)
    set(SDL3_INCLUDE_DIR "${sdl3_SOURCE_DIR}/include")

    set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared third-party libraries" FORCE)
    set(SDLTTF_VENDORED ON CACHE BOOL "Build SDL_ttf dependencies from source" FORCE)
    set(SDLTTF_SAMPLES OFF CACHE BOOL "Do not build SDL_ttf samples" FORCE)
    set(SDLTTF_HARFBUZZ OFF CACHE BOOL "Text shaping is not required by realm_engine" FORCE)
    set(SDLTTF_PLUTOSVG OFF CACHE BOOL "SVG color fonts are not required by realm_engine" FORCE)
    FetchContent_Declare(
        SDL3_ttf
        URL https://github.com/libsdl-org/SDL_ttf/releases/download/release-3.2.2/SDL3_ttf-3.2.2.tar.gz
        URL_HASH SHA256=63547d58d0185c833213885b635a2c0548201cc8f301e6587c0be1a67e1e045d
        DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    )

    # SDL_ttf's release source archive omits its vendored submodules. Populate
    # the exact FreeType revision used by that release into the location its
    # build expects, without adding it to our build a second time.
    FetchContent_Declare(
        SDL3_ttf_freetype
        GIT_REPOSITORY https://github.com/libsdl-org/freetype.git
        GIT_TAG 9973564cfa63763a3e4ac67c09147899539b1e07
        GIT_SHALLOW FALSE
        SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/sdl3_ttf-src/external/freetype"
        SOURCE_SUBDIR cmake/realm-engine-no-subdirectory
    )
    FetchContent_MakeAvailable(SDL3_ttf_freetype)
    FetchContent_MakeAvailable(SDL3_ttf)

    FetchContent_Declare(
        rapidjson
        URL https://github.com/Tencent/rapidjson/archive/refs/tags/v1.1.0.tar.gz
        URL_HASH SHA256=bf7ced29704a1e696fbccf2a2b4ea068e7774fa37f6d7dd4039d0787f8bed98e
        DOWNLOAD_EXTRACT_TIMESTAMP FALSE
        SOURCE_SUBDIR cmake/realm-engine-no-subdirectory
    )
    FetchContent_MakeAvailable(rapidjson)
    set(RAPIDJSON_INCLUDE_DIR "${rapidjson_SOURCE_DIR}/include")
else()
    set(REALM_EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external")
    set(SDL3_INCLUDE_DIR "${REALM_EXTERNAL_DIR}/include")
    set(RAPIDJSON_INCLUDE_DIR "${REALM_EXTERNAL_DIR}/include")

    add_library(SDL3::SDL3 SHARED IMPORTED GLOBAL)
    set_target_properties(SDL3::SDL3 PROPERTIES
        IMPORTED_IMPLIB "${REALM_EXTERNAL_DIR}/lib/SDL3/x64/SDL3.lib"
        IMPORTED_LOCATION "${REALM_EXTERNAL_DIR}/lib/SDL3/x64/SDL3.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL3_INCLUDE_DIR}"
    )
    add_library(SDL3_ttf::SDL3_ttf SHARED IMPORTED GLOBAL)
    set_target_properties(SDL3_ttf::SDL3_ttf PROPERTIES
        IMPORTED_IMPLIB "${REALM_EXTERNAL_DIR}/lib/SDL3_ttf/x64/SDL3_ttf.lib"
        IMPORTED_LOCATION "${REALM_EXTERNAL_DIR}/lib/SDL3_ttf/x64/SDL3_ttf.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL3_INCLUDE_DIR}"
    )
endif()

add_library(realm_rapidjson INTERFACE)
add_library(RapidJSON::rapidjson ALIAS realm_rapidjson)
target_include_directories(realm_rapidjson SYSTEM INTERFACE
    "${RAPIDJSON_INCLUDE_DIR}"
)

set(REALM_ASSET_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets")
set(REALM_SHADER_ASSETS
    "${REALM_ASSET_SOURCE_DIR}/Shaders/sprite.vert.spv"
    "${REALM_ASSET_SOURCE_DIR}/Shaders/sprite.frag.spv"
    "${REALM_ASSET_SOURCE_DIR}/Shaders/sprite.vert.reflect.json"
    "${REALM_ASSET_SOURCE_DIR}/Shaders/sprite.frag.reflect.json"
    "${REALM_ASSET_SOURCE_DIR}/Shaders/terrain.vert.spv"
    "${REALM_ASSET_SOURCE_DIR}/Shaders/terrain.frag.spv"
    "${REALM_ASSET_SOURCE_DIR}/Shaders/terrain.vert.reflect.json"
    "${REALM_ASSET_SOURCE_DIR}/Shaders/terrain.frag.reflect.json"
)
set(REALM_FONT_ASSETS
    "${REALM_ASSET_SOURCE_DIR}/Fonts/Inter-4.1/extras/ttf/Inter-Regular.ttf"
    "${REALM_ASSET_SOURCE_DIR}/Fonts/Inter-4.1/extras/ttf/Inter-SemiBold.ttf"
    "${REALM_ASSET_SOURCE_DIR}/Fonts/Inter-4.1/LICENSE.txt"
)

# Automatically find all .cpp files in src directory
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

# Procgen visualization and its application panel remain optional, but all shipped presets enable them.
set(REALM_PROCGEN_DEBUG_SOURCES
    "${CMAKE_SOURCE_DIR}/src/game/GreaterRealmClimateWeatherInspection.cpp"
    "${CMAKE_SOURCE_DIR}/src/game/GreaterRealmInspectionView.cpp"
    "${CMAKE_SOURCE_DIR}/src/game/GreaterRealmDebugBiomes.cpp"
    "${CMAKE_SOURCE_DIR}/src/game/GreaterRealmDebugPanel.cpp"
)
set(REALM_PROCGEN_RUNTIME_SOURCES
    "${CMAKE_SOURCE_DIR}/src/procgen/Biome.cpp"
    "${CMAKE_SOURCE_DIR}/src/procgen/Climate.cpp"
    "${CMAKE_SOURCE_DIR}/src/procgen/GreaterRealm.cpp"
    "${CMAKE_SOURCE_DIR}/src/procgen/GreaterRealmDebug.cpp"
    "${CMAKE_SOURCE_DIR}/src/procgen/Hydrology.cpp"
    "${CMAKE_SOURCE_DIR}/src/procgen/MountainPeaks.cpp"
    "${CMAKE_SOURCE_DIR}/src/procgen/TerrainConstraints.cpp"
    "${CMAKE_SOURCE_DIR}/src/procgen/TerrainConstraintPainting.cpp"
)
list(REMOVE_ITEM SOURCES ${REALM_PROCGEN_DEBUG_SOURCES} ${REALM_PROCGEN_RUNTIME_SOURCES})
if(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
    list(APPEND SOURCES ${REALM_PROCGEN_DEBUG_SOURCES})
endif()

add_subdirectory(src/procgen)
add_executable(realm_engine ${SOURCES} $<TARGET_OBJECTS:realm_procgen_runtime>)

if(REALM_OPTIMIZE_PROCGEN_DEBUG AND MSVC)
    # Procgen data crosses the object-target boundary in STL containers, so the
    # executable must use the same iterator ABI. Test targets remain checked.
    target_compile_definitions(realm_engine PRIVATE
        "$<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>"
    )
endif()

# Include directories
target_include_directories(realm_engine PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(realm_engine PRIVATE
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
    RapidJSON::rapidjson
)

if(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
    target_compile_definitions(realm_engine PRIVATE REALM_ENABLE_PROCGEN_DEBUG_VIEW=1)
endif()

target_compile_definitions(realm_engine PRIVATE
    "$<$<CONFIG:Debug>:REALM_ENABLE_PROCGEN_PROFILING=1>"
)

# Bundle runtime dependencies and the explicit runtime asset allowlist.
add_custom_command(TARGET realm_engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:realm_engine>/assets/Shaders"
        "$<TARGET_FILE_DIR:realm_engine>/assets/Fonts"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:SDL3::SDL3>"
        "$<TARGET_FILE:SDL3_ttf::SDL3_ttf>"
        $<TARGET_FILE_DIR:realm_engine>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${REALM_SHADER_ASSETS}
        "$<TARGET_FILE_DIR:realm_engine>/assets/Shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${REALM_FONT_ASSETS}
        "$<TARGET_FILE_DIR:realm_engine>/assets/Fonts"
)

if(REALM_BUILD_TESTS)
    enable_testing()

    add_executable(realm_engine_tests EXCLUDE_FROM_ALL
        tests/procgen/GreaterRealmTests.cpp
        src/procgen/Biome.cpp
        src/procgen/Climate.cpp
        src/procgen/GreaterRealm.cpp
        src/procgen/GreaterRealmDebug.cpp
        src/procgen/Hydrology.cpp
        src/procgen/MountainPeaks.cpp
        src/procgen/TerrainConstraints.cpp
    )

    target_include_directories(realm_engine_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_compile_definitions(realm_engine_tests PRIVATE REALM_TEST_BUILD=1)

    add_test(NAME procgen_greater_realm COMMAND realm_engine_tests)

    add_executable(realm_climate_tests EXCLUDE_FROM_ALL
        tests/procgen/ClimateTests.cpp
        src/world/SeasonalClimate.cpp
        src/world/Weather.cpp
        src/procgen/Biome.cpp
        src/procgen/Climate.cpp
        src/procgen/GreaterRealm.cpp
        src/procgen/GreaterRealmDebug.cpp
        src/procgen/Hydrology.cpp
        src/procgen/MountainPeaks.cpp
        src/procgen/TerrainConstraints.cpp
    )

    target_include_directories(realm_climate_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_compile_definitions(realm_climate_tests PRIVATE REALM_TEST_BUILD=1)

    add_test(NAME procgen_greater_realm_climate COMMAND realm_climate_tests)

    add_executable(realm_biome_tests EXCLUDE_FROM_ALL
        tests/procgen/BiomeTests.cpp
        src/game/GreaterRealmDebugBiomes.cpp
        src/procgen/Biome.cpp
        src/procgen/Climate.cpp
        src/procgen/GreaterRealm.cpp
        src/procgen/GreaterRealmDebug.cpp
        src/procgen/Hydrology.cpp
        src/procgen/MountainPeaks.cpp
        src/procgen/TerrainConstraints.cpp
    )

    target_include_directories(realm_biome_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_compile_definitions(realm_biome_tests PRIVATE REALM_TEST_BUILD=1)

    add_test(NAME procgen_greater_realm_biomes COMMAND realm_biome_tests)

    add_executable(realm_procgen_regeneration_tests EXCLUDE_FROM_ALL
        tests/procgen/GreaterRealmRegenerationTests.cpp
        src/procgen/GreaterRealm.cpp
        src/procgen/Hydrology.cpp
        src/procgen/MountainPeaks.cpp
        src/procgen/TerrainConstraints.cpp
    )

    target_include_directories(realm_procgen_regeneration_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_compile_definitions(realm_procgen_regeneration_tests PRIVATE
        REALM_TEST_BUILD=1
        REALM_ENABLE_PROCGEN_PROFILING=1
    )

    add_test(NAME procgen_staged_regeneration COMMAND realm_procgen_regeneration_tests)

    add_executable(realm_procgen_pipeline_tests EXCLUDE_FROM_ALL
        tests/procgen/HydrologyConstraintTests.cpp
        src/procgen/Biome.cpp
        src/procgen/Climate.cpp
        src/procgen/GreaterRealm.cpp
        src/procgen/GreaterRealmDebug.cpp
        src/procgen/Hydrology.cpp
        src/procgen/MountainPeaks.cpp
        src/procgen/TerrainConstraints.cpp
    )

    target_include_directories(realm_procgen_pipeline_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_compile_definitions(realm_procgen_pipeline_tests PRIVATE REALM_TEST_BUILD=1)

    add_test(NAME procgen_hydrology_constraints COMMAND realm_procgen_pipeline_tests)

    add_executable(realm_procgen_paint_tests EXCLUDE_FROM_ALL
        tests/procgen/TerrainConstraintPaintingTests.cpp
        src/procgen/TerrainConstraintPainting.cpp
        src/procgen/TerrainConstraints.cpp
    )

    target_include_directories(realm_procgen_paint_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_compile_definitions(realm_procgen_paint_tests PRIVATE REALM_TEST_BUILD=1)

    add_test(NAME procgen_constraint_painting COMMAND realm_procgen_paint_tests)

    add_executable(realm_mountain_peak_tests EXCLUDE_FROM_ALL
        tests/procgen/MountainPeakTests.cpp
        src/procgen/Biome.cpp
        src/procgen/Climate.cpp
        src/procgen/GreaterRealm.cpp
        src/procgen/GreaterRealmDebug.cpp
        src/procgen/Hydrology.cpp
        src/procgen/MountainPeaks.cpp
        src/procgen/TerrainConstraints.cpp
    )

    target_include_directories(realm_mountain_peak_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_compile_definitions(realm_mountain_peak_tests PRIVATE REALM_TEST_BUILD=1)

    add_test(NAME procgen_mountain_peaks COMMAND realm_mountain_peak_tests)

    add_executable(realm_procgen_representation_tests EXCLUDE_FROM_ALL
        tests/procgen/RepresentationEvaluationTests.cpp
        src/procgen/GreaterRealm.cpp
        src/procgen/Hydrology.cpp
        src/procgen/MountainPeaks.cpp
        src/procgen/TerrainConstraints.cpp
    )

    target_include_directories(realm_procgen_representation_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_compile_definitions(realm_procgen_representation_tests PRIVATE REALM_TEST_BUILD=1)

    add_test(NAME procgen_representation_evaluation COMMAND realm_procgen_representation_tests)

    add_executable(realm_ecs_tests EXCLUDE_FROM_ALL
        tests/ecs/WorldLifecycleTests.cpp
        src/ecs/World.cpp
    )

    target_include_directories(realm_ecs_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_compile_definitions(realm_ecs_tests PRIVATE REALM_TEST_BUILD=1)

    add_test(NAME ecs_world_lifecycle COMMAND realm_ecs_tests)

    add_executable(realm_ecs_system_tests EXCLUDE_FROM_ALL
        tests/ecs/WorldSystemResourceTests.cpp
        src/ecs/World.cpp
    )

    target_include_directories(realm_ecs_system_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_compile_definitions(realm_ecs_system_tests PRIVATE REALM_TEST_BUILD=1)

    add_test(NAME ecs_system_resources COMMAND realm_ecs_system_tests)

    add_executable(realm_ecs_query_tests EXCLUDE_FROM_ALL
        tests/ecs/WorldQueryTests.cpp
        src/ecs/World.cpp
    )

    target_include_directories(realm_ecs_query_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_compile_definitions(realm_ecs_query_tests PRIVATE REALM_TEST_BUILD=1)

    add_test(NAME ecs_component_queries COMMAND realm_ecs_query_tests)

    add_executable(realm_ui_tests EXCLUDE_FROM_ALL
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

    target_include_directories(realm_ui_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${SDL3_INCLUDE_DIR}
    )

    target_compile_definitions(realm_ui_tests PRIVATE REALM_TEST_BUILD=1)

    add_test(NAME ui_text_measurement COMMAND realm_ui_tests)

    add_executable(realm_ui_repeat_button_tests EXCLUDE_FROM_ALL
        tests/ui/RepeatButtonTests.cpp
        src/ui/Button.cpp
        src/ui/FocusableControl.cpp
        src/ui/InputSurface.cpp
        src/ui/RepeatButton.cpp
        src/ui/Style.cpp
        src/ui/UIElement.cpp
    )

    target_include_directories(realm_ui_repeat_button_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${SDL3_INCLUDE_DIR}
    )

    target_compile_definitions(realm_ui_repeat_button_tests PRIVATE REALM_TEST_BUILD=1)

    add_test(NAME ui_repeat_button COMMAND realm_ui_repeat_button_tests)

    add_executable(realm_ui_slider_tests EXCLUDE_FROM_ALL
        tests/ui/SliderTests.cpp
        src/ui/FocusableControl.cpp
        src/ui/InputSurface.cpp
        src/ui/Slider.cpp
        src/ui/Style.cpp
        src/ui/UIElement.cpp
    )

    target_include_directories(realm_ui_slider_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${SDL3_INCLUDE_DIR}
    )

    target_compile_definitions(realm_ui_slider_tests PRIVATE REALM_TEST_BUILD=1)

    add_test(NAME ui_slider COMMAND realm_ui_slider_tests)

    add_executable(realm_greater_realm_debug_panel_tests EXCLUDE_FROM_ALL
        tests/game/GreaterRealmDebugPanelTests.cpp
        tests/game/GreaterRealmDebugPanelProcgenStubs.cpp
        src/game/GreaterRealmDebugPanel.cpp
        src/game/GreaterRealmInspectionView.cpp
        src/procgen/TerrainConstraintPainting.cpp
        src/ui/Button.cpp
        src/ui/ComboBox.cpp
        src/ui/FocusableControl.cpp
        src/ui/InputSurface.cpp
        src/ui/Layout.cpp
        src/ui/Primitives.cpp
        src/ui/RepeatButton.cpp
        src/ui/Slider.cpp
        src/ui/Style.cpp
        src/ui/TextBox.cpp
        src/ui/UIElement.cpp
    )

    target_include_directories(realm_greater_realm_debug_panel_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${SDL3_INCLUDE_DIR}
    )

    target_compile_definitions(realm_greater_realm_debug_panel_tests PRIVATE REALM_TEST_BUILD=1)

    add_test(NAME ui_greater_realm_debug_panel COMMAND realm_greater_realm_debug_panel_tests)

    add_executable(realm_climate_weather_inspection_tests EXCLUDE_FROM_ALL
        tests/game/ClimateWeatherInspectionTests.cpp
        src/game/GreaterRealmClimateWeatherInspection.cpp
        src/game/GreaterRealmInspectionView.cpp
        src/world/SeasonalClimate.cpp
        src/world/Weather.cpp
        src/procgen/Biome.cpp
        src/procgen/Climate.cpp
        src/procgen/GreaterRealm.cpp
        src/procgen/GreaterRealmDebug.cpp
        src/procgen/Hydrology.cpp
        src/procgen/MountainPeaks.cpp
        src/procgen/TerrainConstraints.cpp
    )

    target_include_directories(realm_climate_weather_inspection_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_compile_definitions(realm_climate_weather_inspection_tests PRIVATE REALM_TEST_BUILD=1)

    add_test(NAME game_climate_weather_inspection COMMAND realm_climate_weather_inspection_tests)

    add_executable(realm_terrain_mesh_tests EXCLUDE_FROM_ALL
        tests/rendering/TerrainMeshTests.cpp
        src/rendering/TerrainMesh.cpp
    )

    target_include_directories(realm_terrain_mesh_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    target_compile_definitions(realm_terrain_mesh_tests PRIVATE REALM_TEST_BUILD=1)

    add_test(NAME rendering_terrain_mesh COMMAND realm_terrain_mesh_tests)

    add_custom_target(realm_tests DEPENDS
        realm_engine_tests
        realm_climate_tests
        realm_biome_tests
        realm_procgen_regeneration_tests
        realm_procgen_pipeline_tests
        realm_procgen_paint_tests
        realm_mountain_peak_tests
        realm_procgen_representation_tests
        realm_ecs_tests
        realm_ecs_system_tests
        realm_ecs_query_tests
        realm_ui_tests
        realm_ui_repeat_button_tests
        realm_ui_slider_tests
        realm_greater_realm_debug_panel_tests
        realm_climate_weather_inspection_tests
        realm_terrain_mesh_tests
    )
endif()
