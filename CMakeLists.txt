cmake_minimum_required(VERSION 3.24)

project(usd-vector-plugins VERSION 0.1.0 LANGUAGES CXX)

option(USDVECTOR_ENABLE_OPENUSD "Build OpenUSD-dependent targets" OFF)
option(USDVECTOR_USE_SYSTEM_NLOHMANN_JSON
    "Use a system-provided nlohmann/json package" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

add_subdirectory(libs/usd-vector-core)
add_subdirectory(third_party)
add_subdirectory(libs/usd-geojson)

if(USDVECTOR_ENABLE_OPENUSD)
    message(STATUS "OpenUSD targets are not available yet")
endif()