set(RFD_PROCGEN_SOURCES
    Climate.cpp
    GreaterRealm.cpp
    Hydrology.cpp
    MountainPeaks.cpp
    TerrainConstraints.cpp
)

if(RFD_ENABLE_PROCGEN_DEBUG_VIEW)
    list(APPEND RFD_PROCGEN_SOURCES GreaterRealmDebug.cpp)
endif()

# MSVC's runtime checks cannot coexist with optimization. Keep this adjustment
# scoped to the procgen object target so the rest of a Debug build is unchanged.
if(RFD_OPTIMIZE_PROCGEN_DEBUG AND MSVC)
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Od" "/O2" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Ob0" "/Ob2" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
endif()

add_library(rfd_procgen_runtime OBJECT ${RFD_PROCGEN_SOURCES})

target_include_directories(rfd_procgen_runtime PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

if(RFD_OPTIMIZE_PROCGEN_DEBUG AND MSVC)
    target_compile_definitions(rfd_procgen_runtime PRIVATE
        "$<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>"
    )
endif()

if(RFD_ENABLE_PROCGEN_DEBUG_VIEW)
    target_compile_definitions(rfd_procgen_runtime PRIVATE RFD_ENABLE_PROCGEN_DEBUG_VIEW=1)
endif()

target_compile_definitions(rfd_procgen_runtime PRIVATE
    "$<$<CONFIG:Debug>:RFD_ENABLE_PROCGEN_PROFILING=1>"
)

if(RFD_OPTIMIZE_PROCGEN_DEBUG AND NOT MSVC)
    target_compile_options(rfd_procgen_runtime PRIVATE "$<$<CONFIG:Debug>:-O2>")
endif()
