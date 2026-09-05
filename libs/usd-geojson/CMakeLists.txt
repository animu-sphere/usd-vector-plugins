cmake_minimum_required(VERSION 3.24)

project(usdGeoJson VERSION 0.1.0 LANGUAGES CXX)

if(NOT TARGET usdvector::core)
    find_package(usdVectorCore CONFIG REQUIRED)
endif()
if(NOT TARGET nlohmann_json::nlohmann_json)
    if(USDVECTOR_USE_SYSTEM_NLOHMANN_JSON)
        find_package(nlohmann_json 3.11 CONFIG REQUIRED)
    else()
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/nlohmann_json"
                         "${CMAKE_CURRENT_BINARY_DIR}/nlohmann_json")
    endif()
endif()

add_library(usd-geojson src/reader.cpp)
add_library(usdvector::geojson ALIAS usd-geojson)
set_target_properties(usd-geojson PROPERTIES EXPORT_NAME geojson)

target_compile_features(usd-geojson PUBLIC cxx_std_17)
target_include_directories(usd-geojson
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_include_directories(usd-geojson PRIVATE
    $<TARGET_PROPERTY:nlohmann_json::nlohmann_json,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(usd-geojson
    PUBLIC usdvector::core
)

set_target_properties(usd-geojson PROPERTIES OUTPUT_NAME usdGeoJson)

install(TARGETS usd-geojson EXPORT usdGeoJsonTargets)
install(DIRECTORY include/ DESTINATION include)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/usdGeoJsonConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/usdGeoJsonConfig.cmake"
    INSTALL_DESTINATION lib/cmake/usdGeoJson)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/usdGeoJsonConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
install(EXPORT usdGeoJsonTargets
    FILE usdGeoJsonTargets.cmake
    NAMESPACE usdvector::
    DESTINATION lib/cmake/usdGeoJson)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/usdGeoJsonConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/usdGeoJsonConfigVersion.cmake"
    DESTINATION lib/cmake/usdGeoJson)

if(BUILD_TESTING)
    add_executable(usd-geojson-tests tests/reader_tests.cpp)
    target_link_libraries(usd-geojson-tests PRIVATE usdvector::geojson)
    target_compile_options(usd-geojson-tests PRIVATE
        $<IF:$<CXX_COMPILER_ID:MSVC>,/UNDEBUG,-UNDEBUG>)
    add_test(NAME usd-geojson-tests COMMAND usd-geojson-tests)
endif()