# Changelog

All notable changes to this project are documented here.

The release sequence is maintained in
[the implementation plan](docs/roadmap/IMPLEMENTATION_PLAN.md). Release
records are kept in [docs/releases](docs/releases/README.md).

## [Unreleased]

### Added

- The direct-read stage policy: `/Vector` is authored as the default prim, and
  the new `upAxis` and `metersPerUnit` file-format arguments declare stage
  orientation and units without converting source coordinates. Both stage
  values stay unauthored when the arguments are omitted.
- A `vector-geojson` argument-parsing test target, and stage-policy,
  layer-identity, and default-prim composition coverage in the packaged
  runtime acceptance probe.
- Apache-2.0 repository licensing and notices.
- Repository-wide C++ formatting configuration.
- A canonical implementation plan for artifact completion, runtime
  composition, scalability measurement, conversion, and indexed formats.

### Fixed

- The packaged runtime acceptance probe now treats an OpenUSD error posted for
  a refused source as a rejection, so its negative fixtures no longer abort the
  probe run.
- Test targets keep assertions enabled in release configurations, which the
  supported OpenUSD build uses.

### Performance

- The lazy GeoJSON reader now retains a feature-array cursor instead of one
  source span per feature while preserving the existing reader contract.

### Documentation

- Synchronized the design, architecture, capability, diagnostics, build, and
  install documents with the implemented M0-M4 GeoJSON vertical slice.

## [0.1.0] - 2026-09-04

### Added

- `usdVectorCore`, with format-independent geometry, properties, identifiers,
  bounds, validation, reader contracts, and stable diagnostics.
- `usdGeoJson`, with buffered FeatureCollection parsing and preservation of
  typed properties, CRS, bounds, and foreign members.
- `usdVectorAuthoring`, with deterministic prim naming, topology,
  triangulation, local-origin precision, and OpenUSD stage emission.
- The `vector-geojson` FileFormat plugin, including registration, arguments,
  `ArAsset` reads, `.json` probing, and OpenStrata L0-L5 evidence.