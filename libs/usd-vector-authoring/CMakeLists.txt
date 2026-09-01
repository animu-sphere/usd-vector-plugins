add_library(usd-vector-authoring
    src/authoring.cpp
    src/triangulation.cpp
)
add_library(usdvector::authoring ALIAS usd-vector-authoring)

target_compile_features(usd-vector-authoring PUBLIC cxx_std_17)
target_include_directories(usd-vector-authoring
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(usd-vector-authoring
    PUBLIC usdvector::core
    PRIVATE usdvector::json
)

set_target_properties(usd-vector-authoring PROPERTIES OUTPUT_NAME usdVectorAuthoring)

install(TARGETS usd-vector-authoring)
install(DIRECTORY include/ DESTINATION include)

if(BUILD_TESTING)
    add_executable(usd-vector-authoring-tests tests/authoring_tests.cpp)
    target_link_libraries(usd-vector-authoring-tests PRIVATE usdvector::authoring)
    add_test(NAME usd-vector-authoring-tests COMMAND usd-vector-authoring-tests)
endif()
