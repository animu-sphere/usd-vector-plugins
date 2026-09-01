# Third-party dependencies

This directory owns the CMake boundary for external libraries used by the
vector readers. Dependency-specific target names and headers stay below this
boundary; project libraries consume the stable `usdvector::` targets instead.

The current dependency is nlohmann/json 3.11.3 for GeoJSON decoding. By
default CMake obtains the pinned source with FetchContent. Builds with a
managed package can set `USDVECTOR_USE_SYSTEM_NLOHMANN_JSON=ON` and provide a
compatible `nlohmann_json` CMake package.

No transport, resolver, OpenUSD, or reprojection dependency belongs here.
See [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md) for license and source
information.