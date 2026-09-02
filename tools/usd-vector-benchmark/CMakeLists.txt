cmake_minimum_required(VERSION 3.24)

project(usdVectorBenchmark VERSION 0.1.0 LANGUAGES CXX)

add_executable(usd-vector-benchmark main.cpp)
target_compile_features(usd-vector-benchmark PRIVATE cxx_std_17)
target_link_libraries(usd-vector-benchmark
    PRIVATE usdvector::authoring usdvector::geojson)

if(WIN32)
    target_link_libraries(usd-vector-benchmark PRIVATE psapi)
endif()

if(USDVECTOR_ENABLE_OPENUSD)
    target_compile_definitions(usd-vector-benchmark PRIVATE
        USDVECTOR_ENABLE_OPENUSD=1)
endif()

if(BUILD_TESTING)
    add_test(NAME usd-vector-benchmark-smoke
        COMMAND $<TARGET_FILE:usd-vector-benchmark> --case points --count 2)
    add_test(NAME usd-vector-benchmark-invalid-count
        COMMAND $<TARGET_FILE:usd-vector-benchmark> --case points --count invalid)
    set_tests_properties(usd-vector-benchmark-invalid-count
        PROPERTIES WILL_FAIL TRUE)
endif()
