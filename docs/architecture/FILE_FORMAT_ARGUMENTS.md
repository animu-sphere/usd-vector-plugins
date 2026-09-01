# File-format arguments

The MVP argument surface stays deliberately small. Arguments are parsed and
normalized by `vector-geojson`; shared validation is called before reading or
authoring. Normalized arguments participate in layer identity.

## 1. Implemented arguments

| Argument | Values | Default | Meaning |
| --- | --- | --- | --- |
| `strict` | `true`, `false` | `false` | Promote documented recoverable input problems to errors. |
| `properties` | `all`, `none` | `all` | Author or omit feature properties. IDs and dataset metadata are unaffected. |
| `geometry` | `all`, `points`, `curves`, `meshes`, `none` | `all` | Select geometry classes while retaining each selected feature's identity. |

Boolean spelling is lowercase after normalization. Unknown names, unknown
values, duplicate keys with conflicting values, and empty values are errors.
Arguments are never silently ignored.

`geometry=points` includes Point and MultiPoint; `curves` includes LineString
and MultiLineString; `meshes` includes Polygon and MultiPolygon. `none` creates
dataset metadata only and does not author feature prims.

## 2. Extension handling

`.geojson` is registered directly. `.json` is an MVP capability, but it is not
claimed unconditionally because it is a general-purpose extension. The plugin
must use `CanRead`-equivalent content probing to confirm a supported GeoJSON
root before accepting it. The probe is bounded, restores source position, and
does not consume a non-seekable source irreversibly. Ordinary JSON documents
remain available to other handlers.

## 3. Deferred arguments

| Candidate | Reason deferred |
| --- | --- |
| `bbox=minx,miny,maxx,maxy` | GeoJSON has no spatial index; the performance contract must be explicit. |
| `featureLimit=N` | Truncation metadata and deterministic ordering must be specified first. |
| `property=name,...` | Escaping and normalization syntax need a stable design. |
| `origin=auto|none|x,y,z` | `auto` is the only MVP precision-safe policy. |
| `triangulate=true|false` | Polygon USD mapping requires triangulation; false has no valid MVP representation. |
| `crs` or `targetCrs` | Reprojection belongs to explicit conversion, not implicit reads. |

Adding an argument requires validation tests, a layer-identity test, capability
matrix updates, and documentation of unsupported combinations.