# Implementation Plan

Last updated: 2026-09-04

This is the canonical ordered plan after completion of the GeoJSON vertical
slice. The existing GeoJSON implementation is the compatibility baseline, not
a prototype to bypass while adding formats.

```text
Stabilize -> Scale -> Compose -> Generalize -> Select
 v0.1.x       M5       runtime     FlatGeobuf   indexed reads
```

The goal is not to maximize the number of formats. It is to prove that the
format-independent ingestion architecture remains useful under operational,
large-data, and runtime-composition constraints.

## 1. Fixed boundaries

- `usdVectorCore` owns format- and OpenUSD-independent geometry, properties,
  identity, bounds, metadata, validation, and diagnostics.
- Format readers translate bytes into project-owned vector values.
- `usdVectorAuthoring` owns deterministic topology, triangulation,
  local-origin precision, metadata, and USD mapping.
- FileFormat plugins only register formats, normalize arguments, acquire
  `ArAsset` data, invoke readers and authoring, and project diagnostics.
- ArResolver implementations own transport, authentication, retry, and cache
  policy. This repository does not implement HTTP or S3.
- Reprojection, tiling, payload generation, and resumable durable conversion
  belong to an explicit converter or host workflow.

Deterministic prim hierarchy, names, properties, metadata keys, local origin,
topology, arguments, and diagnostic codes are public compatibility contracts.

## 2. Ordered delivery

### Phase 1: Stabilize the v0.1.x line

Close GeoJSON edge cases and keep diagnostics, FileFormat arguments, CRS,
foreign members, bounds, and large-coordinate behavior aligned with their
documented contracts. Add regression tests around each corrected behavior.

Improve clean build, install, package, and release reproducibility. Keep the
root README focused on the shortest path from `UsdStage::Open` to authored
Points, BasisCurves, and Mesh prims; keep detailed contracts in `docs/`.

### Phase 2: Establish the M5 scalability contract

Measure the complete boundary, not only reader parsing:

```text
Reader -> Feature materialization -> Authoring plan -> USD stage emission
```

The benchmark records source bytes, feature and vertex counts, parse time,
authoring-plan time, USD emission time, time to first feature, time to open,
peak RSS, copied bytes, temporary geometry memory, and flattened layer size.
The current baseline and reproduction procedure are in
[SCALABILITY_BASELINE.md](../reports/SCALABILITY_BASELINE.md).

Retain `VectorDataset` for deterministic batch processing. Introduce bounded
feature batches or incremental authoring only when measurements show that
reader laziness is being defeated by downstream materialization. Parser-owned
types must not leak into the shared vector contract.

M5 is complete when buffered and lazy use cases are explicit, reader memory is
measurable, the authoring materialization boundary is understood, and the need
for incremental authoring can be decided from evidence.

### Phase 3: Validate runtime composition

Integrate the published artifact with `usd-geospatial-runtime` and verify the
boundary in a real composition:

```text
ArResolver -> Vector FileFormat -> source-space USD
                                      |
                                      v
                         usd-geospatial-runtime
                                      |
                                      v
                         placement and composition
```

Acceptance covers local and resolver-backed assets, CRS and local-origin
metadata handoff, runtime placement, layer composition, diagnostics, artifact
reconstruction, SBOM, and provenance. Integration findings may refine a
contract, but must not move transport, reprojection, or placement policy into
this repository.

### Phase 4: Generalize with FlatGeobuf

Use FlatGeobuf to validate the format-independent architecture under binary
parsing, large datasets, spatial indexing, and range-read pressure. Implement
in this order: sequential reader, shared vector-model mapping, shared USD
authoring, benchmark, index-aware investigation, then selective reads.

Do not create a format-specific authoring path. GDAL may be evaluated as an
optional backend or oracle, but does not enter `usdVectorCore` as a mandatory
dependency. Detailed admission rules are in
[FORMAT_EXPANSION.md](FORMAT_EXPANSION.md).

### Phase 5: Define indexed partial reads

Specify seek/range byte sources, spatial selection, feature iteration, source
identity, and resolver interaction from evidence gathered with an indexed
format. Bounding-box, tile, feature-range, LOD, and runtime-driven loading are
candidate capabilities, not early v0.2 requirements.

The target is spatially selective USD composition without whole-file feature
materialization. Avoid a general LOD or tiling framework until a concrete
format and runtime use case establishes the required abstraction.

## 3. Release sequence

| Candidate | Required story |
| --- | --- |
| v0.1.x | Stabilized GeoJSON behavior, diagnostics, tests, documentation, installation, and reproducible artifacts |
| v0.2.0 | A clear architecture advance demonstrated by M5 completion, runtime-composition validation, a second format on the shared architecture, or the first selective-read contract |
| v0.3.x and later | Production FlatGeobuf, indexed reads, selective composition, streaming authoring, runtime-driven spatial loading, or additional formats as justified |

The exact v0.2.0 gate is selected from measured integration needs; format count
alone is not a release story. Version numbers may change before publication,
but each release must describe one complete architecture advance.

## 4. Definition of done

A capability is `implemented` only when all applicable conditions hold:

1. Code exists in the owning module.
2. Its public contract is documented.
3. Expected success and important failure paths are tested.
4. Diagnostics use stable codes.
5. Dependency manifests, licenses, and notices are current.
6. The capability matrix is current.
7. Plugin capabilities include discovery coverage.
8. Artifact capabilities pass clean composition and reconstruction.
9. Compatibility output is deterministic.

Use one pull request for one contract change or independently testable
capability. Do not combine format expansion, streaming, conversion, and CRS
transformation into one change.

## 5. Deferred scope

Renderer policy, materials, cameras, implicit reprojection, write-back,
round-trip fidelity, vector-tile rendering, SVG, broad GDAL format support,
HTTP clients, credentials, resolver implementations, and a monolithic shared
geospatial core remain outside the near-term plan.