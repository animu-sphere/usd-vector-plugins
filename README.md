# OpenUSD Vector Plugins

OpenUSD FileFormat Plugins and reusable C++ libraries for GIS vector data.
GeoJSON is the first format target. The project keeps transport and
reprojection outside the reader and authors deterministic OpenUSD geometry
through a separate authoring layer.

## Status

The OpenUSD-independent `usdVectorCore` model and buffered `usdGeoJson` reader
are implemented and tested. OpenUSD authoring and FileFormat registration are
the next milestones.

| Milestone | Scope | Status |
| --- | --- | --- |
| M0 | Repository skeleton, CMake, CI, OpenStrata manifests | in progress |
| M1 | `usdVectorCore` model, validation, bounds, diagnostics, identifiers | done |
| M2 | `usdGeoJson` FeatureCollection reader and MVP geometries | done |
| M3 | OpenUSD authoring, triangulation, local-origin metadata | planned |
| M4 | FileFormat registration, `ArAsset`, arguments, integration tests | planned |

## Building

The OpenUSD-free lane can be built with plain CMake:

```powershell
cmake -S . -B build/core -G Ninja -DUSDVECTOR_ENABLE_OPENUSD=OFF
cmake --build build/core
ctest --test-dir build/core --output-on-failure
```

The OpenStrata workspace manifests are provided for the pinned `cy2026` /
`usd` environment:

```text
ost configure
ost build
ost test
```

See [docs/README.md](docs/README.md) for the architecture and capability
contracts. External dependencies are bounded under [third_party](third_party)
and documented in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).