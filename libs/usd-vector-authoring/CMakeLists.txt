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

if(USDVECTOR_ENABLE_OPENUSD)
    find_package(pxr CONFIG REQUIRED)
    target_sources(usd-vector-authoring PRIVATE src/usd_authoring.cpp)
    target_compile_definitions(usd-vector-authoring PRIVATE USDVECTOR_ENABLE_OPENUSD=1)
    if(PXR_INCLUDE_DIRS)
        target_include_directories(usd-vector-authoring PRIVATE ${PXR_INCLUDE_DIRS})
    endif()
    if(TARGET pxr::usdGeom)
        target_link_libraries(usd-vector-authoring PRIVATE pxr::usdGeom)
    elseif(TARGET usdGeom)
        target_link_libraries(usd-vector-authoring PRIVATE usdGeom)
    else()
        target_link_libraries(usd-vector-authoring PRIVATE ${PXR_LIBRARIES})
    endif()
endif()

set_target_properties(usd-vector-authoring PROPERTIES OUTPUT_NAME usdVectorAuthoring)

install(TARGETS usd-vector-authoring)
install(DIRECTORY include/ DESTINATION include)

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
