option(USDVECTOR_USE_SYSTEM_NLOHMANN_JSON
       "Use a system-provided nlohmann/json package" OFF)

if(USDVECTOR_USE_SYSTEM_NLOHMANN_JSON)
    find_package(nlohmann_json 3.11 CONFIG REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

add_library(usd-vector-json INTERFACE)
add_library(usdvector::json ALIAS usd-vector-json)
target_link_libraries(usd-vector-json INTERFACE nlohmann_json::nlohmann_json)