# Format expansion

This document defines how another vector format enters the project. Delivery
order and release gates belong to [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md).
A format adapts to `usdVectorCore`; it does not copy the GeoJSON-to-USD path.

## Compatibility baseline: GeoJSON

Establish geometry, properties, identifiers, diagnostics, precision policy,
and the complete FileFormat integration. Correctness and contract quality take
priority over whole-document parsing performance in the first implementation.

The v0.1.0 vertical slice is the compatibility baseline. Stabilization work
must preserve its public mapping while closing documented edge cases.

## Next validation format: FlatGeobuf

FlatGeobuf is selected to test the architecture, not to increase the format
count. Its binary representation, large-dataset use cases, spatial index, and
range-read behavior apply pressure that GeoJSON does not.

Entry gates:

1. GeoJSON stabilization preserves the v0.1.x compatibility contracts.
2. Feature-reader and authoring APIs are stable under measured M5 workloads.
3. Runtime composition has validated resolver, metadata, and placement
   boundaries, or has identified the contract changes needed before a second
   format enters.

Implementation order:

1. Sequential FlatGeobuf reader.
2. Mapping into the shared vector model.
3. Reuse of the shared USD authoring path.
4. Comparative benchmark.
5. Index-aware range-read investigation.
6. Spatially selective materialization.

The indexed contract is derived after the sequential vertical slice. The first
implementation does not need to solve every partial-read use case.

## Admission rules

- A reader produces project-owned vector semantics and diagnostics.
- Format parser types do not leak into `usdVectorCore` or authoring.
- The existing deterministic USD mapping is reused.
- Byte acquisition remains transport-independent and resolver-compatible.
- Reprojection and placement remain explicit upper-layer policies.
- New dependencies receive an ownership, packaging, and license review.
- Capability claims include semantic tests and measured evidence where scale
  or partial access is part of the claim.

## Deferred candidates

Shapefile, GeoPackage, and other formats remain candidates rather than planned
phases. Their contracts are defined only when a concrete use case reaches the
admission gates. Shapefile would require resolver-safe sidecar handling;
GeoPackage would require explicit container and layer identity.

Reprojection, spatial tiling, payload generation, write support, and broad
GDAL/OGR integration remain separate deferred work.