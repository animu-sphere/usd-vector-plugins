# Changelog

All notable changes to this project are documented here.

The project has not tagged a release. The release sequence is maintained in
[the implementation plan](docs/roadmap/IMPLEMENTATION_PLAN.md).

## [Unreleased]

### Added

- Apache-2.0 repository licensing and notices.
- Repository-wide C++ formatting configuration.
- A canonical implementation plan for artifact completion, runtime
  composition, scalability measurement, conversion, and indexed formats.

### Documentation

- Synchronized the design, architecture, capability, diagnostics, build, and
  install documents with the implemented M0-M4 GeoJSON vertical slice.

## [0.1.0] - Unreleased

### Added

- `usdVectorCore`, with format-independent geometry, properties, identifiers,
  bounds, validation, reader contracts, and stable diagnostics.
- `usdGeoJson`, with buffered FeatureCollection parsing and preservation of
  typed properties, CRS, bounds, and foreign members.
- `usdVectorAuthoring`, with deterministic prim naming, topology,
  triangulation, local-origin precision, and OpenUSD stage emission.
- The `vector-geojson` FileFormat plugin, including registration, arguments,
  `ArAsset` reads, `.json` probing, and OpenStrata L0-L5 evidence.