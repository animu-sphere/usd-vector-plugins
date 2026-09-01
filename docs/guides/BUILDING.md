# Building

Status: the core-only build described here is executable. OpenUSD and plugin
targets remain planned until their milestones land.

## Requirements

- CMake 3.24 or newer
- A C++17 compiler: Visual Studio 2022, recent Clang, or recent GCC
- Ninja or another CMake-supported build tool
- OpenUSD for `usdVectorAuthoring` and plugin targets
- OpenStrata CLI only when building through the workspace manifests

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

This lane builds and tests `usdVectorCore` and `usdGeoJson`.

## Full build

Point CMake at an OpenUSD installation using the cache variable selected by
the root build, then build and test:

```powershell
cmake -S . -B build/full -G Ninja -Dpxr_DIR=C:\path\to\openusd\lib\cmake\pxr
cmake --build build/full
ctest --test-dir build/full --output-on-failure
```

The exact OpenUSD compatibility range will be documented once CI pins the
first runtime. A successful compile against an unlisted version does not make
that version supported.

## OpenStrata build

When workspace manifests are added, the supported flow is expected to be:

```powershell
ost configure
ost build
ost test
ost plugin build plugins/vector-geojson
ost plugin test plugins/vector-geojson --up-to 4
```

Build instructions must be updated in the same change that introduces or
renames an option, target, dependency, or required environment variable.