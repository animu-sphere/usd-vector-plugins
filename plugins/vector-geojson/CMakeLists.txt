cmake_minimum_required(VERSION 3.22)

project(VectorGeojsonPlugin
    VERSION 0.1.0
    DESCRIPTION "OpenUSD geojson file-format plugin"
    LANGUAGES CXX)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenStrataPlugin.cmake")
openstrata_default_build_type()

if(NOT pxr_FOUND)
    find_package(pxr REQUIRED CONFIG)
endif()

if(NOT TARGET usdvector::core)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../libs/usd-vector-core"
                     "${CMAKE_CURRENT_BINARY_DIR}/usd-vector-core")
endif()
if(NOT TARGET usdvector::geojson)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../libs/usd-geojson"
                     "${CMAKE_CURRENT_BINARY_DIR}/usd-geojson")
endif()
if(NOT TARGET usdvector::authoring)
    set(USDVECTOR_ENABLE_OPENUSD ON)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../libs/usd-vector-authoring"
                     "${CMAKE_CURRENT_BINARY_DIR}/usd-vector-authoring")
endif()

set(PLUGIN_NAME UsdVectorGeoJsonFileFormat)

add_library(${PLUGIN_NAME} SHARED
    src/file_format.cpp
)

target_compile_features(${PLUGIN_NAME} PRIVATE cxx_std_17)
target_include_directories(${PLUGIN_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    ${PXR_INCLUDE_DIRS}
)
target_link_libraries(${PLUGIN_NAME}
    PRIVATE
        usdvector::authoring
        usdvector::geojson
)

openstrata_link_openusd(
    TARGET ${PLUGIN_NAME}
    COMPONENTS arch tf gf vt sdf usd)

openstrata_configure_plugin(
    TARGET ${PLUGIN_NAME}
    PLUG_INFO_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/plugin/resources/vector-geojson/plugInfo.json.in"
    PLUG_INFO_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/plugin/resources/vector-geojson/plugInfo.json")

openstrata_install_plugin_bundle(
    TARGET ${PLUGIN_NAME}
    RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/plugin/resources/vector-geojson/plugInfo.json"
    RESOURCE_DESTINATION "plugin/resources/vector-geojson"
    MANIFESTS
        "${CMAKE_CURRENT_SOURCE_DIR}/openstrata.plugin.yaml"
        "${CMAKE_CURRENT_SOURCE_DIR}/openstrata.scaffold.yaml"
    FIXTURE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")

if(PROJECT_IS_TOP_LEVEL)
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/../../LICENSE"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../NOTICE"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../THIRD_PARTY_NOTICES.md"
        DESTINATION "share/usd-vector-plugins")
endif()