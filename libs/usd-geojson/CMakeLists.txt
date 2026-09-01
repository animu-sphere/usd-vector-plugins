add_library(usd-geojson src/reader.cpp)
add_library(usdvector::geojson ALIAS usd-geojson)

target_compile_features(usd-geojson PUBLIC cxx_std_17)
target_include_directories(usd-geojson
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(usd-geojson
    PUBLIC usdvector::core
    PRIVATE usdvector::json
)

set_target_properties(usd-geojson PROPERTIES OUTPUT_NAME usdGeoJson)

install(TARGETS usd-geojson)
install(DIRECTORY include/ DESTINATION include)

if(BUILD_TESTING)
    add_executable(usd-geojson-tests tests/reader_tests.cpp)
    target_link_libraries(usd-geojson-tests PRIVATE usdvector::geojson)
    add_test(NAME usd-geojson-tests COMMAND usd-geojson-tests)
endif()