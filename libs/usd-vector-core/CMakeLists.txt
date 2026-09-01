cmake_minimum_required(VERSION 3.24)

project(usdVectorCore VERSION 0.1.0 LANGUAGES CXX)

add_library(usd-vector-core
    src/bounds.cpp
    src/diagnostics.cpp
    src/geometry.cpp
    src/identifiers.cpp
    src/validation.cpp
)

add_library(usdvector::core ALIAS usd-vector-core)
set_target_properties(usd-vector-core PROPERTIES EXPORT_NAME core)

target_compile_features(usd-vector-core PUBLIC cxx_std_17)
target_include_directories(usd-vector-core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

set_target_properties(usd-vector-core PROPERTIES
    OUTPUT_NAME usdVectorCore
)

install(TARGETS usd-vector-core EXPORT usdVectorCoreTargets)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT usdVectorCoreTargets
    FILE usdVectorCoreTargets.cmake
    NAMESPACE usdvector::
    DESTINATION lib/cmake/usdVectorCore)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/usdVectorCoreConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/usdVectorCoreConfig.cmake"
    INSTALL_DESTINATION lib/cmake/usdVectorCore)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/usdVectorCoreConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/usdVectorCoreConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/usdVectorCoreConfigVersion.cmake"
    DESTINATION lib/cmake/usdVectorCore
)

if(BUILD_TESTING)
    add_executable(usd-vector-core-tests tests/core_tests.cpp)
    target_link_libraries(usd-vector-core-tests PRIVATE usdvector::core)
    add_test(NAME usd-vector-core-tests COMMAND usd-vector-core-tests)
endif()