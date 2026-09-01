# nlohmann/json boundary

This directory provides the stable `usdvector::json` CMake target used by the
GeoJSON reader. It vendors the pinned nlohmann/json 3.11.3 headers and does
not access the network during a normal build. A compatible system package can
be selected with
`USDVECTOR_USE_SYSTEM_NLOHMANN_JSON=ON`.

The dependency is header-only and its headers are private to the reader
implementation. Consumers depend on project-owned model types instead. The
vendored source is accompanied by `LICENSE.MIT`.