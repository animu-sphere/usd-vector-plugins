# Building

Status: the core-only and OpenUSD plugin builds are executable. The GeoJSON
bundle follows the generated `usd-fileformat-cpp` OpenStrata template.

## Requirements

- CMake 3.24 or newer
- A C++17 compiler: Visual Studio 2022, recent Clang, or recent GCC
- Ninja or another CMake-supported build tool
- OpenUSD 26.08 for stage emission and plugin targets
- OpenStrata CLI 0.22.8 when building or testing through the workspace manifests

The intended JSON parser and triangulation dependencies will be pinned by the
root build and documented in `THIRD_PARTY_NOTICES.md`; do not install arbitrary
system versions until those choices are committed.

## Core-only build

The pure libraries must support a build without OpenUSD:

```powershell
cmake -S . -B build/core -G Ninja -DUSDVECTOR_ENABLE_OPENUSD=OFF
cmake --build build/core
ctest --test-dir build/core --output-on-failure
```

This lane builds and tests `usdVectorCore`, `usdGeoJson`, and the
OpenUSD-independent `usdVectorAuthoring` plan layer.

## Full build

Point CMake at an OpenUSD installation using the cache variable selected by
the root build, then build and test:

```powershell
cmake -S . -B build/full -G Ninja -Dpxr_DIR=C:\path\to\openusd\lib\cmake\pxr
cmake --build build/full
ctest --test-dir build/full --output-on-failure
```

This enables the `usdVectorAuthoring` stage-emission path and the
`vector-geojson` FileFormat plugin. The supported plugin runtime range is
OpenUSD `>=26.08,<27.0`; a successful compile against an unlisted version does
not make that version supported.

## OpenStrata build

The supported workspace flow is:

```powershell
ost configure
ost build
ost test
ost plugin build plugins/vector-geojson
ost plugin doctor plugins/vector-geojson
ost plugin test plugins/vector-geojson --up-to 5
```

Build instructions must be updated in the same change that introduces or
renames an option, target, dependency, or required environment variable.