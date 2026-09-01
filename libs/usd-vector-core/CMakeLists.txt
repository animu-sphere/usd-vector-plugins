add_library(usd-vector-core
    src/bounds.cpp
    src/diagnostics.cpp
    src/geometry.cpp
    src/identifiers.cpp
    src/validation.cpp
)

add_library(usdvector::core ALIAS usd-vector-core)

target_compile_features(usd-vector-core PUBLIC cxx_std_17)
target_include_directories(usd-vector-core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

set_target_properties(usd-vector-core PROPERTIES
    OUTPUT_NAME usdVectorCore
)

install(TARGETS usd-vector-core EXPORT usd-vector-targets)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT usd-vector-targets
    NAMESPACE usdvector::
    DESTINATION lib/cmake/usd-vector-plugins
)

if(BUILD_TESTING)
    add_executable(usd-vector-core-tests tests/core_tests.cpp)
    target_link_libraries(usd-vector-core-tests PRIVATE usdvector::core)
    add_test(NAME usd-vector-core-tests COMMAND usd-vector-core-tests)
endif()