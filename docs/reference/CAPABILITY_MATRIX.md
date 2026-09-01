# Capability Matrix

This matrix distinguishes contractual intent from working code. The repository
contains tested core, GeoJSON reader, and OpenUSD-independent authoring-plan
capabilities. OpenUSD stage emission is implemented in the optional authoring
path; plugin integration remains `planned` until its owning module is built.

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
| GeoJSON FeatureCollection | `.geojson` | implemented, not connected | 1 |
| GeoJSON FeatureCollection in generic JSON | `.json` | implemented, not connected | 1, accepted only after bounded content probing |
| FlatGeobuf | `.fgb` | deferred | 2 |
| Shapefile | `.shp` plus sidecars | deferred | 3 |
| GeoPackage | `.gpkg` | deferred | 4 |
| Local file | filesystem | planned | 1 |
| In-memory source | test/embedding | implemented, not connected | 1 |
| Resolver-provided `ArAsset` | any resolver identity | planned | 1 |
| HTTP implemented here | URL | not planned | resolver responsibility |
| Vector writing | any | not planned | separate milestone after read stability |

## GeoJSON objects and geometry

| Capability | Status | Notes |
| --- | --- | --- |
| FeatureCollection | implemented, not connected | MVP root object |
| Feature | implemented, not connected | Read through collection |
| Bare Geometry root | deferred | Not needed for first plugin milestone |
| Point / MultiPoint | implemented, not connected | `UsdGeomPoints` |
| LineString / MultiLineString | implemented, not connected | linear `UsdGeomBasisCurves` |
| Polygon | implemented, not connected | triangulated `UsdGeomMesh` |
| Polygon holes | implemented, not connected | included in triangulation |
| MultiPolygon | implemented, not connected | Xform with Mesh children |
| Null geometry | implemented, not connected | metadata-only feature Xform |
| GeometryCollection | deferred | Stable hierarchy still to be designed |
| Self-intersection repair | not planned | invalid input is diagnosed |

## Data preservation

| Capability | Status | Notes |
| --- | --- | --- |
| String and integer feature IDs | implemented, not connected | original type preserved |
| Missing IDs | implemented, not connected | stable sequential names |
| Duplicate IDs | planned | warning plus deterministic suffix; error in strict mode |
| bool, integer, number, string properties | implemented, not connected | typed USD attributes |
| null properties | implemented, not connected | explicit null-name metadata |
| homogeneous scalar arrays | implemented, not connected | typed USD arrays |
| nested or heterogeneous values | implemented, not connected | canonical JSON fallback |
| Dataset and feature `bbox` | implemented, not connected | kept distinct from computed bounds |
| Foreign members | planned | preserved subject to documented limits |
| Legacy GeoJSON `crs` | implemented, not connected | preserved and warned; no reprojection |

## Coordinates and authoring

| Capability | Status | Notes |
| --- | --- | --- |
| Double-precision internal coordinates | implemented, not connected | all geometry processing |
| Deterministic local origin | implemented, not connected | bounds-center origin in the authoring plan and stage emitter |
| Source-coordinate recovery metadata | implemented, not connected | `vector:localOrigin` and source bounds |
| Reprojection | not planned | explicit converter/host responsibility |
| One logical prim per feature | implemented, not connected | identity before geometry authoring |
| Deterministic prim paths | implemented, not connected | source ID, then sequence |
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