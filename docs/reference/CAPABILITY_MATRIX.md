# Capability Matrix

This matrix distinguishes contractual intent from working code. The repository
contains tested core, GeoJSON reader, authoring-plan, OpenUSD stage-emission,
and GeoJSON FileFormat capabilities. The complete vertical slice is exercised
against the pinned OpenStrata runtime.

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
| GeoJSON FeatureCollection | `.geojson` | implemented | 1 |
| GeoJSON FeatureCollection in generic JSON | `.json` | implemented | 1, accepted only after bounded content probing |
| FlatGeobuf | `.fgb` | deferred | 2 |
| Shapefile | `.shp` plus sidecars | deferred | 3 |
| GeoPackage | `.gpkg` | deferred | 4 |
| Local file | filesystem | implemented | 1 |
| In-memory source | test/embedding | implemented | 1 |
| Resolver-provided `ArAsset` | any resolver identity | implemented | 1 |
| HTTP implemented here | URL | not planned | resolver responsibility |
| Vector writing | any | not planned | separate milestone after read stability |

## GeoJSON objects and geometry

| Capability | Status | Notes |
| --- | --- | --- |
| FeatureCollection | implemented | MVP root object |
| Feature | implemented | Read through collection |
| Bare Geometry root | deferred | Not needed for first plugin milestone |
| Point / MultiPoint | implemented | `UsdGeomPoints` |
| LineString / MultiLineString | implemented | linear `UsdGeomBasisCurves` |
| Polygon | implemented | triangulated `UsdGeomMesh` |
| Polygon holes | implemented | included in triangulation |
| MultiPolygon | implemented | Xform with Mesh children |
| Null geometry | implemented | metadata-only feature Xform |
| GeometryCollection | deferred | Stable hierarchy still to be designed |
| Self-intersection repair | not planned | invalid input is diagnosed |

## Data preservation

| Capability | Status | Notes |
| --- | --- | --- |
| String and integer feature IDs | implemented | original type preserved |
| Missing IDs | implemented | stable sequential names |
| Duplicate IDs | implemented | warning plus deterministic suffix; error in strict mode |
| bool, integer, number, string properties | implemented | typed USD attributes |
| null properties | implemented | explicit null-name metadata |
| homogeneous scalar arrays | implemented | typed USD arrays |
| nested or heterogeneous values | implemented | canonical JSON fallback |
| Dataset and feature `bbox` | implemented | kept distinct from computed bounds |
| Foreign members | implemented | canonical JSON in `vector:foreignMembers`; geometry-object extras remain diagnosed |
| Legacy GeoJSON `crs` | implemented | preserved and warned; no reprojection |

## Coordinates and authoring

| Capability | Status | Notes |
| --- | --- | --- |
| Double-precision internal coordinates | implemented | all geometry processing |
| Deterministic local origin | implemented | bounds-center origin in the authoring plan and stage emitter |
| Source-coordinate recovery metadata | implemented | `vector:localOrigin` and source bounds |
| Reprojection | not planned | explicit converter/host responsibility |
| One logical prim per feature | implemented | identity before geometry authoring |
| Deterministic prim paths | implemented | source ID, then sequence |
| Renderer-specific styling | not planned | host responsibility |

## File-format arguments

`strict`, `properties`, and `geometry` are implemented. The normative value and
default table is [FILE_FORMAT_ARGUMENTS.md](../architecture/FILE_FORMAT_ARGUMENTS.md).

## Known MVP limitations

- The default GeoJSON factory remains buffered. A tested lazy materialization
  candidate is available, but it still parses a complete JSON DOM at open and
  is not a bounded-memory streaming implementation.
- GeoJSON has no spatial index; bounding-box filtering would still require a
  full scan and is therefore deferred.
- No implicit reprojection or axis conversion occurs.
- Invalid self-intersecting polygons are rejected rather than repaired.
- Direct plugin reads are not the future production path for tiling or large,
  resumable conversion.

## Release acceptance

The GeoJSON reference implementation remains releasable only while the core,
authoring, and plugin lanes cover all six supported geometry types, polygon
holes, typed scalar properties, stable identifiers and prim paths,
local-origin precision, CRS and foreign-member preservation, `.json` probing,
`ArAsset` reads, malformed-input diagnostics, and flatten/reopen behavior.

Scalability, artifact-composition, and format-expansion gates are maintained in
[IMPLEMENTATION_PLAN.md](../roadmap/IMPLEMENTATION_PLAN.md).