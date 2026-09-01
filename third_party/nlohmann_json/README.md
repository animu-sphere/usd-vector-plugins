# nlohmann/json boundary

This directory provides the stable `usdvector::json` CMake target used by the
GeoJSON reader. It defaults to the pinned nlohmann/json 3.11.3 source through
FetchContent and can use a compatible system package with
`USDVECTOR_USE_SYSTEM_NLOHMANN_JSON=ON`.

The dependency is header-only and its headers are private to the reader
implementation. Consumers depend on project-owned model types instead.