cmake_minimum_required(VERSION 3.24)

project(usdVectorAuthoring VERSION 0.1.0 LANGUAGES CXX)

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

add_library(usd-vector-authoring
    src/authoring.cpp
    src/triangulation.cpp
)
add_library(usdvector::authoring ALIAS usd-vector-authoring)
set_target_properties(usd-vector-authoring PROPERTIES EXPORT_NAME authoring)

target_compile_features(usd-vector-authoring PUBLIC cxx_std_17)
target_include_directories(usd-vector-authoring
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_include_directories(usd-vector-authoring PRIVATE
    $<TARGET_PROPERTY:nlohmann_json::nlohmann_json,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(usd-vector-authoring
    PUBLIC usdvector::core
)

if(USDVECTOR_ENABLE_OPENUSD)
    find_package(pxr CONFIG REQUIRED)
    target_sources(usd-vector-authoring PRIVATE src/usd_authoring.cpp)
    target_compile_definitions(usd-vector-authoring PRIVATE USDVECTOR_ENABLE_OPENUSD=1)
    if(PXR_INCLUDE_DIRS)
        target_include_directories(usd-vector-authoring PUBLIC ${PXR_INCLUDE_DIRS})
    endif()
    if(TARGET pxr::usdGeom)
        target_link_libraries(usd-vector-authoring PUBLIC pxr::usdGeom)
    elseif(TARGET usdGeom)
        target_link_libraries(usd-vector-authoring PUBLIC usdGeom)
    elseif(PXR_LIBRARIES)
        target_link_libraries(usd-vector-authoring PUBLIC ${PXR_LIBRARIES})
    else()
        message(FATAL_ERROR
            "OpenUSD was found, but no usable usdGeom target or PXR_LIBRARIES was provided")
    endif()
endif()

set_target_properties(usd-vector-authoring PROPERTIES OUTPUT_NAME usdVectorAuthoring)

install(TARGETS usd-vector-authoring EXPORT usdVectorAuthoringTargets)
install(DIRECTORY include/ DESTINATION include
    PATTERN "usd_authoring.h" EXCLUDE)
if(USDVECTOR_ENABLE_OPENUSD)
    install(FILES include/usdvector/authoring/usd_authoring.h
            DESTINATION include/usdvector/authoring)
endif()

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/usdVectorAuthoringConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/usdVectorAuthoringConfig.cmake"
    INSTALL_DESTINATION lib/cmake/usdVectorAuthoring)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/usdVectorAuthoringConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
install(EXPORT usdVectorAuthoringTargets
    FILE usdVectorAuthoringTargets.cmake
    NAMESPACE usdvector::
    DESTINATION lib/cmake/usdVectorAuthoring)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/usdVectorAuthoringConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/usdVectorAuthoringConfigVersion.cmake"
    DESTINATION lib/cmake/usdVectorAuthoring)

if(BUILD_TESTING)
    add_executable(usd-vector-authoring-tests tests/authoring_tests.cpp)
    target_link_libraries(usd-vector-authoring-tests PRIVATE usdvector::authoring)
    add_test(NAME usd-vector-authoring-tests COMMAND usd-vector-authoring-tests)

    if(USDVECTOR_ENABLE_OPENUSD)
        add_executable(usd-vector-authoring-usd-tests tests/usd_authoring_tests.cpp)
        target_link_libraries(usd-vector-authoring-usd-tests PRIVATE usdvector::authoring)
        add_test(NAME usd-vector-authoring-usd-tests COMMAND usd-vector-authoring-usd-tests)
    endif()
endif()
