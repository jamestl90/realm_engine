set(REALM_PROCGEN_SOURCES
    Climate.cpp
    GreaterRealm.cpp
    Hydrology.cpp
    MountainPeaks.cpp
    TerrainConstraints.cpp
)

if(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
    list(APPEND REALM_PROCGEN_SOURCES
        GreaterRealmDebug.cpp
        TerrainConstraintPainting.cpp
    )
endif()

# MSVC's runtime checks cannot coexist with optimization. Keep this adjustment
# scoped to the procgen object target so the rest of a Debug build is unchanged.
if(REALM_OPTIMIZE_PROCGEN_DEBUG AND MSVC)
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Od" "/O2" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Ob0" "/Ob2" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
endif()

add_library(realm_procgen_runtime OBJECT ${REALM_PROCGEN_SOURCES})

target_include_directories(realm_procgen_runtime PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

if(REALM_OPTIMIZE_PROCGEN_DEBUG AND MSVC)
    target_compile_definitions(realm_procgen_runtime PRIVATE
        "$<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>"
    )
endif()

if(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
    target_compile_definitions(realm_procgen_runtime PRIVATE REALM_ENABLE_PROCGEN_DEBUG_VIEW=1)
endif()

target_compile_definitions(realm_procgen_runtime PRIVATE
    "$<$<CONFIG:Debug>:REALM_ENABLE_PROCGEN_PROFILING=1>"
)

if(REALM_OPTIMIZE_PROCGEN_DEBUG AND NOT MSVC)
    target_compile_options(realm_procgen_runtime PRIVATE "$<$<CONFIG:Debug>:-O2>")
endif()
