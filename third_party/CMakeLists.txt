# Third-party dependencies are kept behind explicit CMake boundaries. The
# OpenUSD-free core still uses the JSON dependency because GeoJSON is the first
# reader, but no dependency-specific target name leaks into library consumers.
add_subdirectory(nlohmann_json)