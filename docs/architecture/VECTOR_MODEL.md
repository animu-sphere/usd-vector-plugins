# Vector model

`usdVectorCore` represents GIS features independently of GeoJSON and OpenUSD.
This document is the semantic contract; exact C++ spelling may evolve until
the first public API release.

## 1. Dataset and reader

```cpp
class FeatureReader {
public:
    virtual Result<DatasetMetadata> ReadMetadata() = 0;
    virtual Result<std::optional<Feature>> ReadNext() = 0;
    virtual ~FeatureReader() = default;
};
```

`ReadMetadata` may inspect the source but does not consume feature iteration.
`ReadNext` returns one logical feature, end-of-stream, or a typed failure. The
interface permits a streaming implementation and does not expose parser-owned
JSON values.

## 2. Geometry

The MVP model supports Point, MultiPoint, LineString, MultiLineString, Polygon,
and MultiPolygon. A coordinate has required `double x` and `double y` values
and an optional `double z`. Mixed dimensionality within one geometry is an
error in strict mode and follows an explicit normalization policy otherwise.

A polygon consists of one outer ring and zero or more inner rings. A ring is
stored without a duplicate terminal coordinate after normalization. The model
retains ring boundaries; triangulation is an authoring operation.

Null geometry is valid for a feature and is distinct from malformed geometry.
GeometryCollection is outside the MVP and produces a stable unsupported-type
diagnostic.

## 3. Feature identity

`FeatureId` supports source string and integer IDs. It does not infer identity
from properties. When absent, authoring uses the zero-based source sequence to
generate a stable name. Duplicate source IDs remain visible to authoring so it
can apply deterministic suffixes or reject them in strict mode.

## 4. Properties

Property values preserve these source categories:

```text
null, bool, signed integer, unsigned integer, double, string,
array, object
```

Arrays and objects are recursive project-owned values. Readers do not flatten
or stringify them. Authoring decides whether a value has a natural USD type or
must use the documented JSON fallback.

## 5. Dataset metadata

Dataset metadata can carry source bounds, declared CRS information, GeoJSON
`bbox`, format identity, and preserved foreign members. Feature metadata can
carry source bounds and foreign members in addition to ID and properties.

Computed bounds and declared `bbox` are distinct values. A mismatch is
diagnosed; it is not silently reconciled.

## 6. Validation

Common validation rejects non-finite coordinates, insufficient line or ring
vertices, missing polygon outer rings, and structurally inconsistent Multi
geometries. GeoJSON syntax and member-shape failures remain owned by
`usdGeoJson`.

Self-intersecting polygons are not repaired in the MVP. Authoring reports a
triangulation failure with feature and ring anchors.