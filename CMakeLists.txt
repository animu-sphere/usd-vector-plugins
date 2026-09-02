cmake_minimum_required(VERSION 3.24)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION"
     _usd_vector_version LIMIT_COUNT 1)
string(STRIP "${_usd_vector_version}" _usd_vector_version)

project(usd-vector-plugins
    VERSION ${_usd_vector_version}
    DESCRIPTION "OpenUSD file format plugins and libraries for GIS vector data"
    LANGUAGES CXX)

option(USDVECTOR_ENABLE_OPENUSD "Build OpenUSD-dependent targets" OFF)
option(USDVECTOR_ENABLE_BENCHMARKS "Build scalability benchmark targets" OFF)
option(USDVECTOR_USE_SYSTEM_NLOHMANN_JSON
    "Use a system-provided nlohmann/json package" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/EHsc)
endif()

include(CTest)

add_subdirectory(libs/usd-vector-core)
add_subdirectory(third_party)
add_subdirectory(libs/usd-geojson)
add_subdirectory(libs/usd-vector-authoring)

if(USDVECTOR_ENABLE_BENCHMARKS)
    add_subdirectory(tools/usd-vector-benchmark)
endif()

if(USDVECTOR_ENABLE_OPENUSD)
    message(STATUS "Building OpenUSD-dependent authoring targets")
    find_package(pxr REQUIRED CONFIG)
    add_subdirectory(plugins/vector-geojson)
endif()

install(FILES LICENSE NOTICE THIRD_PARTY_NOTICES.md
    DESTINATION "share/${PROJECT_NAME}")