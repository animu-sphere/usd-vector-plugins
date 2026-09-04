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
    add_test(NAME usd-vector-benchmark-lazy-smoke
        COMMAND $<TARGET_FILE:usd-vector-benchmark> --reader lazy
                --case points --count 2)
    add_test(NAME usd-vector-benchmark-reader-only-smoke
        COMMAND $<TARGET_FILE:usd-vector-benchmark> --reader lazy
                --reader-only --case points --count 2)
    set_tests_properties(usd-vector-benchmark-reader-only-smoke
        PROPERTIES PASS_REGULAR_EXPRESSION
        "lazy,points,2,[0-9]+,2,2,[^,]+,[^,]+,0,[^,]+,[0-9]+,0,0")
    add_test(NAME usd-vector-benchmark-batch-smoke
        COMMAND $<TARGET_FILE:usd-vector-benchmark> --reader lazy
                --reader-only --batch-size 1 --case points --count 2)
    set_tests_properties(usd-vector-benchmark-batch-smoke
        PROPERTIES PASS_REGULAR_EXPRESSION
        "lazy,points,2,[0-9]+,2,2,[^,]+,[^,]+,0,[^,]+,[0-9]+,0,0,2,1")
    add_test(NAME usd-vector-benchmark-invalid-count
        COMMAND $<TARGET_FILE:usd-vector-benchmark> --case points --count invalid)
    set_tests_properties(usd-vector-benchmark-invalid-count
        PROPERTIES WILL_FAIL TRUE)
    add_test(NAME usd-vector-benchmark-invalid-reader
        COMMAND $<TARGET_FILE:usd-vector-benchmark> --reader invalid)
    set_tests_properties(usd-vector-benchmark-invalid-reader
        PROPERTIES WILL_FAIL TRUE)
endif()
