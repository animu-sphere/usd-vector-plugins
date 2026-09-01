# Capability Matrix

This matrix distinguishes contractual intent from working code. The repository
currently contains documentation only, so every implementation capability is
`planned` or explicitly out of scope.

Status vocabulary:

```text
implemented                   present and tested in the owning module
implemented, not connected    present in a library but unreachable from plugin arguments
planned                       contract exists, implementation does not
deferred                      intentionally scheduled after the MVP
not planned                   explicitly outside project scope
```

## Formats and source access

| Capability | Extension/source | Status | Phase |
| --- | --- | --- | --- |
| GeoJSON FeatureCollection | `.geojson` | planned | 1 |
| GeoJSON FeatureCollection in generic JSON | `.json` | planned | 1, accepted only after bounded content probing |
| FlatGeobuf | `.fgb` | deferred | 2 |
| Shapefile | `.shp` plus sidecars | deferred | 3 |
| GeoPackage | `.gpkg` | deferred | 4 |
| Local file | filesystem | planned | 1 |
| In-memory source | test/embedding | planned | 1 |
| Resolver-provided `ArAsset` | any resolver identity | planned | 1 |
| HTTP implemented here | URL | not planned | resolver responsibility |
| Vector writing | any | not planned | separate milestone after read stability |

## GeoJSON objects and geometry

| Capability | Status | Notes |
| --- | --- | --- |
| FeatureCollection | planned | MVP root object |
| Feature | planned | Read through collection |
| Bare Geometry root | deferred | Not needed for first plugin milestone |
| Point / MultiPoint | planned | `UsdGeomPoints` |
| LineString / MultiLineString | planned | linear `UsdGeomBasisCurves` |
| Polygon | planned | triangulated `UsdGeomMesh` |
| Polygon holes | planned | included in triangulation |
| MultiPolygon | planned | Xform with Mesh children |
| Null geometry | planned | metadata-only feature Xform |
| GeometryCollection | deferred | Stable hierarchy still to be designed |
| Self-intersection repair | not planned | invalid input is diagnosed |

## Data preservation

| Capability | Status | Notes |
| --- | --- | --- |
| String and integer feature IDs | planned | original type preserved |
| Missing IDs | planned | stable sequential names |
| Duplicate IDs | planned | warning plus deterministic suffix; error in strict mode |
| bool, integer, number, string properties | planned | typed USD attributes |
| null properties | planned | explicit null-name metadata |
| homogeneous scalar arrays | planned | typed USD arrays |
| nested or heterogeneous values | planned | canonical JSON fallback |
| Dataset and feature `bbox` | planned | kept distinct from computed bounds |
| Foreign members | planned | preserved subject to documented limits |
| Legacy GeoJSON `crs` | planned | preserved and warned; no reprojection |

## Coordinates and authoring

| Capability | Status | Notes |
| --- | --- | --- |
| Double-precision internal coordinates | planned | all geometry processing |
| Deterministic local origin | planned | bounds-center origin |
| Source-coordinate recovery metadata | planned | `vector:localOrigin` and source bounds |
| Reprojection | not planned | explicit converter/host responsibility |
| One logical prim per feature | planned | identity before batching |
| Deterministic prim paths | planned | source ID, then sequence |
| Renderer-specific styling | not planned | host responsibility |

## File-format arguments

`strict`, `properties`, and `geometry` are planned. The normative value and
default table is [FILE_FORMAT_ARGUMENTS.md](../architecture/FILE_FORMAT_ARGUMENTS.md).

## Known MVP limitations

- GeoJSON may initially be buffered in memory even though the reader API is
  streaming-compatible.
- GeoJSON has no spatial index; bounding-box filtering would still require a
  full scan and is therefore deferred.
- No implicit reprojection or axis conversion occurs.
- Invalid self-intersecting polygons are rejected rather than repaired.
- Direct plugin reads are not the future production path for tiling or large,
  resumable conversion.