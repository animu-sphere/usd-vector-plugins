option(USDVECTOR_USE_SYSTEM_NLOHMANN_JSON
       "Use a system-provided nlohmann/json package" OFF)

if(USDVECTOR_USE_SYSTEM_NLOHMANN_JSON)
    find_package(nlohmann_json 3.11 CONFIG REQUIRED)
else()
    add_library(nlohmann_json INTERFACE)
    add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
    target_include_directories(nlohmann_json INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()

add_library(usd-vector-json INTERFACE)
add_library(usdvector::json ALIAS usd-vector-json)
target_link_libraries(usd-vector-json INTERFACE nlohmann_json::nlohmann_json)