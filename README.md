# OpenUSD Vector Plugins

OpenUSD FileFormat Plugins and reusable C++ libraries for GIS vector data.
GeoJSON is the first format target. The project keeps transport and
reprojection outside the reader and authors deterministic OpenUSD geometry
through a separate authoring layer.

This project is licensed under the [Apache License 2.0](LICENSE). Third-party
components and their licenses are listed in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Open a GeoJSON asset

After installing the `vector-geojson` plugin into an OpenUSD environment,
GeoJSON participates in composition like any other supported asset:

```cpp
#include <pxr/usd/usd/stage.h>

pxr::UsdStageRefPtr stage = pxr::UsdStage::Open("roads.geojson");
```

The stage contains deterministic OpenUSD geometry under a stable hierarchy:

```text
/Vector                         UsdGeomXform
/Vector/Features                UsdGeomXform
/Vector/Features/<featureName>  Points, BasisCurves, Mesh, or Xform
```

See [docs/guides/INSTALL.md](docs/guides/INSTALL.md) for plugin installation
and [docs/architecture/USD_MAPPING.md](docs/architecture/USD_MAPPING.md) for
the complete geometry, property, and metadata mapping.

## Status

The OpenUSD-independent `usdVectorCore` model and the buffered and cursor-based
lazy `usdGeoJson` readers are implemented and tested. Shared bounded feature
batches and incremental authoring plans are available for measured large-data
workloads; the production GeoJSON FileFormat path remains buffered until a
reopenable two-pass source workflow is adopted. The authoring library emits an
in-memory OpenUSD stage, and the GeoJSON FileFormat bundle is built from the
generated OpenStrata template and verified against the pinned runtime.

| Milestone | Scope | Status |
| --- | --- | --- |
| M0 | Repository skeleton, CMake, CI, OpenStrata manifests | done |
| M1 | `usdVectorCore` model, validation, bounds, diagnostics, identifiers | done |
| M2 | `usdGeoJson` FeatureCollection reader and MVP geometries | done |
| M3 | OpenUSD authoring, triangulation, local-origin metadata | done |
| M4 | FileFormat registration, `ArAsset`, arguments, integration tests | done: OpenStrata L0-L5 verified |
| M5 | Scalability baseline and evidence-led bounded-memory improvements | done: cursor-based lazy materialization, shared bounded batches, and bounded authoring plans |
| M6 | Runtime composition validation with `usd-geospatial-runtime` | in progress: packaged local-runtime probe; external composition pending |
| M7 | FlatGeobuf architecture validation | deferred |
| M8 | Indexed partial-read and selective-composition contract | deferred |

## Building

The OpenUSD-free lane can be built with plain CMake:

```powershell
cmake -S . -B build/core -G Ninja -DUSDVECTOR_ENABLE_OPENUSD=OFF
cmake --build build/core
ctest --test-dir build/core --output-on-failure
```

Build the optional M5 scalability runner with
`-DUSDVECTOR_ENABLE_BENCHMARKS=ON`. Its reproduction procedure and captured
baseline are in [docs/reports/SCALABILITY_BASELINE.md](docs/reports/SCALABILITY_BASELINE.md).
For example, this measures the lazy reader with bounded batches and bounded
authoring planning:

```powershell
.\build\m5\tools\usd-vector-benchmark\usd-vector-benchmark.exe `
    --reader lazy --authoring incremental --batch-size 256 `
    --case points --count 10000
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
The ordered implementation plan is documented in
[docs/roadmap/IMPLEMENTATION_PLAN.md](docs/roadmap/IMPLEMENTATION_PLAN.md).
Release artifacts are checked against the pinned runtime digest and carry
SBOM/provenance evidence; placement and cross-component composition remain
owned by `usd-geospatial-runtime`.