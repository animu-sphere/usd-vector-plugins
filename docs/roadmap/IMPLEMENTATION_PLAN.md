# Implementation Plan

Last updated: 2026-09-03

This is the canonical ordered plan after completion of the GeoJSON vertical
slice. The existing GeoJSON implementation is the compatibility baseline, not
a prototype to bypass while adding formats.

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

### Step 1: contract sync

Keep README, design, architecture, capability, diagnostics, build, install,
and roadmap documents aligned with the implemented M0-M4 state. A capability
is not implemented merely because a directory or class exists.

### Step 2: M4 hardening

Close the remaining plugin edge cases with semantic tests for discovery,
GeoJSON-bearing `.json` probing, unrelated JSON rejection, resolver-backed
`ArAsset` reads, malformed sources, arguments, deterministic output, and
flatten/reopen behavior. Prefer small hand-authored fixtures grouped by the
semantic behavior they exercise.

### Step 3: artifact completion

Verify a clean OpenStrata build, install layout, post-install plugin discovery,
license and notice inclusion, version metadata, and reproducible artifact
output. Publish the plugin as an independently versioned component with an
explicit OpenUSD compatibility range.

### Step 4: runtime composition

After artifact completion, `usd-geospatial-runtime` may pin and compose the
published vector artifact. Runtime acceptance covers local and
resolver-backed GeoJSON, `.json` probing, geometry and property preservation,
CRS and local-origin metadata, diagnostics, artifact reconstruction, SBOM,
and provenance. Runtime concerns must not add transport or sibling-plugin
dependencies here.

### Step 5: M5 baseline

Measure before replacing the buffered backend. The benchmark/evidence runner
records source bytes, feature and vertex counts, parse time, authoring-plan
time, USD emission time, time to first feature, time to open, peak RSS, copied
bytes, temporary geometry memory, and flattened layer size.

Synthetic cases should include 1,000 and 100,000 points, many lines, one large
polygon, many small polygons, property-heavy features, and large coordinate
values. The report must identify the dataset sizes at which buffered GeoJSON
becomes unsuitable.

### Step 6: evidence-led scalability work

Select only improvements justified by the baseline: incremental JSON parsing,
lazy property decoding, geometry callbacks, allocation changes, reduced
copying, direct plan generation, or bounded feature batches. Parser-owned JSON
types must not leak into the core reader contract.

The current GeoJSON lazy reader applies incremental JSON structure scanning:
`CreateLazy` defers feature validation and materialization to `ReadNext`, while
`ReadMetadata` can perform a non-consuming complete scan when full metadata is
requested before iteration. The measured behavior is recorded in
[SCALABILITY_BASELINE.md](../reports/SCALABILITY_BASELINE.md).

### Step 7: explicit converter

Start `tools/usd-vector-convert` only after scale, reprojection, partitioning,
or tiling requirements are concrete. The FileFormat plugin and converter must
share readers and authoring instead of implementing parallel mappings.

### Step 8: FlatGeobuf investigation

Specify the indexed read, seek/range source, feature iteration, source
identity, and resolver interaction contracts before implementation. The goal
is to prove `ArAsset` plus indexed partial reads, not merely increase the
format count. GDAL remains optional or converter-only; it does not enter
`usdVectorCore`.

## 3. Release sequence

| Candidate | Required story |
| --- | --- |
| v0.1 | GeoJSON vertical slice, deterministic mapping, precision-safe coordinates, typed properties, CRS preservation, resolver compatibility, reproducible artifact, and acceptance evidence |
| v0.2 | M5 evidence, bounded-memory improvement where justified, and a documented practical size envelope |
| v0.3 | Indexed-source experiment, FlatGeobuf vertical slice, and range-read contract |

Version numbers may change before publication, but a release must tell one of
these complete stories rather than claiming readiness from format discovery
alone.

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