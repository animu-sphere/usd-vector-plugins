# USD Vector Plugins documentation

This documentation is organized by responsibility so current contracts,
procedures, and future work do not drift into one another. The repository is
at Milestone 4 plus active M5 scalability work: `usdVectorCore`, the buffered
and cursor-based lazy `usdGeoJson` readers, `usdVectorAuthoring`, and the
`vector-geojson` FileFormat plugin are implemented. The plugin vertical slice
has been verified through OpenStrata L0-L5; M5 baseline measurement and
incremental lazy materialization evidence are available.

| Category | Answers | Start here |
| --- | --- | --- |
| [design/](design/) | Why the project is built this way and what it will not own. | [DESIGN_POLICY.md](design/DESIGN_POLICY.md) |
| [architecture/](architecture/) | How the workspace is structured and how vector data maps to OpenUSD. | [WORKSPACE.md](architecture/WORKSPACE.md), [VECTOR_MODEL.md](architecture/VECTOR_MODEL.md) |
| [reference/](reference/) | What input is accepted and which diagnostics are stable. | [CAPABILITY_MATRIX.md](reference/CAPABILITY_MATRIX.md), [DIAGNOSTICS.md](reference/DIAGNOSTICS.md) |
| [guides/](guides/) | How to build and install the project. | [BUILDING.md](guides/BUILDING.md), [INSTALL.md](guides/INSTALL.md) |
| [roadmap/](roadmap/) | What will be implemented and in which order. | [README.md](roadmap/README.md), [IMPLEMENTATION_PLAN.md](roadmap/IMPLEMENTATION_PLAN.md) |
| [reports/](reports/) | Measured acceptance evidence and reproducibility records. | [SCALABILITY_BASELINE.md](reports/SCALABILITY_BASELINE.md) |

## The one-sentence version

`usd-vector-plugins` reads GIS feature data, GeoJSON first, through
transport-independent readers and authors deterministic OpenUSD Points,
BasisCurves, and Mesh prims without performing reprojection.

```text
GeoJSON -> usdGeoJson -> VectorDataset -> usdVectorAuthoring -> OpenUSD
              ^
              |
         byte source
              ^
              |
      ArAsset / ArResolver  (transport lives here, not in the reader)
```

## Reading order

1. [design/DESIGN_POLICY.md](design/DESIGN_POLICY.md) - standing scope and
   engineering rules.
2. [architecture/WORKSPACE.md](architecture/WORKSPACE.md) - module identities
   and legal dependency directions.
3. [architecture/VECTOR_MODEL.md](architecture/VECTOR_MODEL.md) - the shared
   feature and geometry contract.
4. [architecture/USD_MAPPING.md](architecture/USD_MAPPING.md) - stable prim
   hierarchy, naming, and property mapping.
5. [architecture/COORDINATES_AND_CRS.md](architecture/COORDINATES_AND_CRS.md) -
   precision and CRS preservation policy.
6. [roadmap/README.md](roadmap/README.md) - milestone status and MVP gates.
7. [roadmap/IMPLEMENTATION_PLAN.md](roadmap/IMPLEMENTATION_PLAN.md) - ordered
   work after the GeoJSON vertical slice.

## Repository boundaries

| Repository or layer | Owns | Never owns |
| --- | --- | --- |
| `usd-vector-plugins` | Vector formats, feature semantics, geometry validation, vector-to-USD authoring | Transport, implicit reprojection, renderer behavior |
| OpenUSD `ArResolver` implementations | Asset resolution, HTTP/S3, authentication, retries, byte transport | GeoJSON parsing or vector semantics |
| Host application or explicit converter | Reprojection, visualization policy, long-running conversion workflows | File-format registration |

**A FileFormat Plugin does not know how bytes are transported, and a resolver
does not know what vector features mean.**