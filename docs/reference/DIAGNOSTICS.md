# Diagnostics contract

Diagnostics are typed values with stable user-facing codes. The tables below
reserve the initial code space; a code becomes published only when its
condition is reachable and tested.

## 1. Value shape

Each diagnostic contains a code, severity, message, and available anchors:

```cpp
enum class Severity { Warning, Error };

struct Diagnostic {
    DiagnosticCode code;
    Severity severity;
    std::string message;
    std::optional<std::uint64_t> byteOffset;
    std::optional<std::uint64_t> featureIndex;
    std::optional<std::string> featureId;
    std::optional<std::uint32_t> partIndex;
    std::optional<std::uint32_t> ringIndex;
    std::optional<std::uint64_t> coordinateIndex;
    std::optional<std::string> propertyName;
};
```

The project-owned enum contains semantic categories; stable textual codes are
assigned by the module that presents the error.

## 2. Rules

1. Published code meanings never change and codes are never reused.
2. Every failure has a specific code; there is no generic `Unknown`.
3. Warnings leave a valid openable stage. Otherwise the condition is an error.
4. Messages may improve without breaking tests; tests assert codes and anchors.
5. Diagnostics never contain credentials, authorization data, or signed URLs.
6. A malformed feature identifies its sequence or source ID when available.
7. Strict mode may promote a documented warning to an error without changing
   its code.

## 3. Ownership

| Prefix | Owner | Scope |
| --- | --- | --- |
| `VECxxx` | `usdVectorCore` / `usdVectorAuthoring` | common geometry, properties, bounds, naming, authoring |
| `GJSONxxx` | `usdGeoJson` | JSON and GeoJSON structure |
| `VGJSONxxx` | `vector-geojson` | plugin arguments, source access, and user-facing composition |

## 4. Initial common and authoring allocation

| Code | Default severity | Condition |
| --- | --- | --- |
| `VEC001` | Error | Unsupported geometry type |
| `VEC002` | Error | Missing, malformed, or non-finite coordinate |
| `VEC003` | Error | Line or ring has insufficient distinct coordinates |
| `VEC004` | Error | Polygon triangulation failed |
| `VEC005` | Error | Property cannot be represented under the selected policy |
| `VEC006` | Warning | Duplicate feature ID required a deterministic suffix |
| `VEC007` | Warning | Property name was normalized or collision-suffixed |
| `VEC008` | Warning | Declared bounds disagree with computed bounds |
| `VEC009` | Error | Local-coordinate conversion exceeds precision policy |
| `VEC010` | Error | USD authoring failed |

## 5. Initial GeoJSON allocation

| Code | Default severity | Condition |
| --- | --- | --- |
| `GJSON001` | Error | Malformed JSON |
| `GJSON002` | Error | Root is not a supported GeoJSON object |
| `GJSON003` | Error | Required `type` or `coordinates` member is invalid |
| `GJSON004` | Warning | Legacy `crs` member was preserved as an extension |
| `GJSON005` | Error | `FeatureCollection.features` is missing or not an array |
| `GJSON006` | Error | Feature `properties` is neither object nor null |
| `GJSON007` | Error | Invalid `bbox` member |
| `GJSON008` | Warning | Geometry foreign member cannot be preserved by the MVP model; strict mode promotes this to an error |

## 6. Initial plugin allocation

| Code | Default severity | Condition |
| --- | --- | --- |
| `VGJSON001` | Error | Unknown, invalid, or conflicting file-format argument |
| `VGJSON002` | Error | Source cannot be opened, supplied, or read |

`VGJSON003` through `VGJSON005` remain reserved. Reader and authoring failures
currently retain their owning `GJSONxxx` or `VECxxx` code when projected into
OpenUSD rather than being collapsed into a plugin code.

OpenUSD presentation includes the code and safe source identity, for example:

```text
[VEC003] roads.geojson: feature 17, part 0 has fewer than two distinct points
```

Each published code requires a focused fixture or unit test that reaches it.