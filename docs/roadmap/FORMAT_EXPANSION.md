# Format expansion

New formats enter only after the shared vector model and USD mapping are
proven by the GeoJSON MVP. A format adapts to `usdVectorCore`; it does not copy
the GeoJSON-to-USD path.

## Phase 1: GeoJSON

Establish geometry, properties, identifiers, diagnostics, precision policy,
and the complete FileFormat integration. Correctness and contract quality take
priority over whole-document parsing performance in the first implementation.

Entry gate: repository skeleton and core contracts.

Exit gate: the MVP gates in [README.md](README.md).

## Phase 2: FlatGeobuf

Validate binary parsing, spatial indexes, bounded reads, and resolver-backed
partial access. This phase is where the streaming-compatible interfaces must
demonstrate real bounded-memory behavior.

Entry gate: stable feature-reader and authoring APIs plus measured GeoJSON
memory and open-time baselines.

## Phase 3: Shapefile

Define a multi-file asset contract for `.shp`, `.shx`, `.dbf`, `.prj`, and
optional sidecars. Every sidecar is resolved through the active resolver;
filesystem sibling assumptions are not allowed in shared code.

Entry gate: resolver contract supports deterministic related-asset resolution
and missing-sidecar diagnostics.

## Phase 4: GeoPackage

Introduce container and layer selection, SQLite dependency policy, and spatial
metadata mapping. A GeoPackage can contain several vector layers, so stable
file-format arguments for layer identity are required before implementation.

Entry gate: layer-selection arguments and cache/layer identity are specified.

## Deferred cross-format work

- Reprojection is considered in `usd-vector-convert`, not each reader.
- Spatial tiling and payload generation follow measured large-dataset needs.
- GDAL/OGR may be evaluated as an optional backend or oracle, never introduced
  as an unexamined mandatory dependency.
- Write support receives a separate preservation and round-trip design after
  read behavior is stable.