# Workspace contract

This document fixes planned module identities, responsibilities, and legal
dependency directions. A structural change that contradicts it must update
this contract first.

Status: `usdVectorCore` and `usdGeoJson` have tested library capabilities.
OpenUSD-dependent components remain `planned`. Directories are created with
their first tested capability rather than as empty placeholders.

## 1. Components

| Identity | Directory | Kind | Status | Responsibility |
| --- | --- | --- | --- | --- |
| `usdVectorCore` | `libs/usd-vector-core` | plain CMake library | implemented | Format- and USD-independent geometry, features, properties, bounds, metadata, validation, source interfaces, identifiers, and shared diagnostic values. |
| `usdGeoJson` | `libs/usd-geojson` | plain CMake library | implemented | GeoJSON syntax, geometry/property decoding, GeoJSON validation, and conversion into `usdVectorCore`. |
| `usdVectorAuthoring` | `libs/usd-vector-authoring` | OpenUSD-linked library | planned | Stable prim hierarchy, naming, typed attributes, triangulation, local-origin policy, and dataset metadata. |
| `vector-geojson` | `plugins/vector-geojson` | OpenUSD FileFormat bundle | planned | Registration, argument parsing, `ArAsset` acquisition, reader/authoring composition, and plugin diagnostics. |
| `usd-vector-convert` | `tools/usd-vector-convert` | CLI | deferred | Explicit reprojection, partitioning, payload generation, and long-running conversion. |

## 2. Dependency directions

Allowed:

```text
usdGeoJson          -> usdVectorCore, JSON parser
usdVectorAuthoring  -> usdVectorCore, triangulator, OpenUSD
vector-geojson      -> usdGeoJson, usdVectorAuthoring, OpenUSD
usd-vector-convert  -> readers, usdVectorAuthoring, optional conversion backends
```

Forbidden:

```text
usdVectorCore       -> OpenUSD, format readers, plugins, HTTP, GDAL
usdGeoJson          -> OpenUSD, authoring, plugin registration, transport
usdVectorAuthoring  -> GeoJSON parsing, resolver policy, plugin arguments
any plugin          -> another plugin, HTTP client, cloud SDK
any dependency cycle
```

Planned package identities:

| Identity | CMake target | C++ namespace |
| --- | --- | --- |
| `usdVectorCore` | `usdvector::core` | `usdvector` |
| `usdGeoJson` | `usdvector::geojson` | `usdvector::geojson` |
| `usdVectorAuthoring` | `usdvector::authoring` | `usdvector::authoring` |
| `vector-geojson` | `UsdVectorGeoJsonFileFormat` | `usdvector::geojson_plugin` |

## 3. Source boundaries

```text
libs/usd-vector-core/
    geometry and property values, FeatureReader, validation, diagnostics

libs/usd-geojson/
    GeoJSON parsing and format-specific diagnostics

libs/usd-vector-authoring/
    OpenUSD prim and attribute authoring, triangulation, precision policy

plugins/vector-geojson/
    SdfFileFormat adapter, registration, ArAsset adaptation, argument parsing
```

The root owns composition, CI, workspace versioning, shared licensing,
documentation, notices, and aggregate tests. It does not own module logic.

## 4. Authored stage contract

The MVP stage shape is:

```text
/Vector                         UsdGeomXform
/Vector/Features                UsdGeomXform
/Vector/Features/<featureName>  logical feature prim
```

The exact child type and Multi geometry hierarchy are fixed in
[USD_MAPPING.md](USD_MAPPING.md). Coordinates are stage-local relative to
`vector:localOrigin`; source values remain recoverable as documented in
[COORDINATES_AND_CRS.md](COORDINATES_AND_CRS.md).

## 5. Change invariants

1. Readers return project-owned values and do not author USD.
2. Metadata reading and feature iteration remain separate operations.
3. Feature processing can be incremental and deterministic.
4. Source and stage-local coordinates use distinct types or explicit names.
5. Polygon holes are triangulated; they are not emitted as invalid n-gons.
6. Source IDs and property names are normalized deterministically.
7. Unknown metadata is preserved or diagnosed, never silently dropped.
8. Arguments are normalized before reading and participate in layer identity.
9. Stable diagnostics are tested by code, not full message text.
10. Registration changes include a discovery test.
11. Dependency and notice changes update manifests, CMake, and notices together.
12. Transport and credentials never enter vector contracts or diagnostics.