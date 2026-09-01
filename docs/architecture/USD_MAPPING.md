# OpenUSD mapping

This document fixes the implemented mapping from `usdVectorCore` values to
OpenUSD. It is a compatibility contract for the GeoJSON reference
implementation and future format readers.

## 1. Root hierarchy

```text
/Vector                         UsdGeomXform
/Vector/Features                UsdGeomXform
/Vector/Features/<featureName>  one logical feature
```

One source feature maps to one logical feature prim. Multi geometries may add
`part_0`, `part_1`, and subsequent children, but are never split into unrelated
top-level features.

## 2. Geometry mapping

| Vector geometry | Logical feature prim | Child policy |
| --- | --- | --- |
| Point | `UsdGeomPoints` | one point |
| MultiPoint | `UsdGeomPoints` | all points in one prim |
| LineString | `UsdGeomBasisCurves` | one linear nonperiodic curve |
| MultiLineString | `UsdGeomBasisCurves` | one curve per part using `curveVertexCounts` |
| Polygon | `UsdGeomMesh` | triangles after outer/inner-ring triangulation |
| MultiPolygon | `UsdGeomXform` | one `UsdGeomMesh` child per polygon |
| Null geometry | `UsdGeomXform` | metadata and properties only |

Meshes explicitly author `subdivisionScheme = "none"`. Winding is normalized
before triangulation, and topology is deterministic for identical input and
library version.

## 3. Feature names

Naming priority is:

1. `id_<normalizedSourceId>` when a source ID exists.
2. `f_<eightDigitSequence>` when no source ID exists.

Normalization produces a valid USD identifier, is locale-independent, and
applies a deterministic suffix for collisions. Properties never determine a
prim path. The original source ID is authored as `vector:featureId`.

## 4. Properties

Properties use the `vector:properties:` namespace. Names are normalized as USD
identifiers; `vector:propertyNames` records the normalized-to-original mapping
whenever normalization or collision handling changes a name.

| Source value | USD value |
| --- | --- |
| null | attribute omitted and name recorded in `vector:nullProperties` |
| bool | `bool` |
| signed or unsigned integer | `int64` or `uint64` |
| number | `double` |
| string | `string` |
| homogeneous scalar array | corresponding typed array |
| heterogeneous array or object | canonical JSON string |

Canonical JSON fallback is UTF-8, compact, and uses lexicographically sorted
object keys. It is a preservation mechanism, not a query-friendly schema.

## 5. Metadata keys

The project-defined keys below are stored in the prim's USD `customData`
dictionary. This keeps the keys available without requiring a runtime schema
registration; geometry and feature properties use typed USD attributes as
specified above.

| Key | Location | Meaning |
| --- | --- | --- |
| `vector:format` | `/Vector` | source format, initially `GeoJSON` |
| `vector:sourceBounds` | `/Vector` | source-coordinate bounds as doubles |
| `vector:localOrigin` | `/Vector` | source-space origin used for local positions |
| `vector:crs` | `/Vector` | preserved CRS declaration when present |
| `vector:featureCount` | `/Vector` | count when known without destructive iteration |
| `vector:featureId` | feature | original source ID with source type preserved |
| `vector:nullProperties` | feature | property names explicitly set to null |
| `vector:propertyNames` | feature | normalized-to-original name dictionary |
| `vector:foreignMembers` | `/Vector` and feature | compact canonical JSON for preserved root/feature foreign members |

Root and feature foreign members use the same recursive value model as
properties and are serialized as compact canonical JSON in customData. Unknown
members on geometry objects are diagnosed because the MVP geometry model has no
owner for geometry-level metadata.