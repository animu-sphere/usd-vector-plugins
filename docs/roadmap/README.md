# Roadmap

The current goal is one complete GeoJSON vertical slice, not a broad format
count. Status is tracked against acceptance evidence, not directory presence.

| Milestone | Outcome | Status |
| --- | --- | --- |
| M0 | Documentation contracts, root project skeleton, CI, licensing, notices | implemented |
| M1 | `usdVectorCore`: model, bounds, validation, diagnostics, identifiers | implemented |
| M2 | `usdGeoJson`: FeatureCollection, all MVP geometries, properties, fixtures | implemented |
| M3 | `usdVectorAuthoring`: Points, Curves, Mesh triangulation, metadata, origin | implemented |
| M4 | `vector-geojson`: registration, ArAsset adapter, arguments, integration tests | implemented: OpenStrata L0-L5 verified |
| M5 | Scalability measurement and evidence-led streaming improvements | in progress: baseline evidence |
| M6 | FlatGeobuf investigation and indexed partial-read contract | deferred |

## MVP gates

1. Core and GeoJSON tests pass without OpenUSD.
2. All six MVP geometry families and polygon holes have semantic tests.
3. Properties retain scalar types and documented fallback values.
4. Large coordinates pass source-recovery and local-precision tests.
5. Prim paths and topology are deterministic across repeated reads.
6. `.geojson` and GeoJSON-bearing `.json` open through discovered plugin
   registration and an `ArAsset`; unrelated `.json` is rejected by probing.
7. Every reachable failure exposes a stable tested diagnostic code.
8. Capability, mapping, coordinate, argument, and diagnostic documents match
   the implementation.

## Working rule

Do not mark a capability `implemented` until it is present in its owning
module, connected where the capability matrix claims it is connected, and
covered by the appropriate test tier.

Format expansion after the MVP is described in
[FORMAT_EXPANSION.md](FORMAT_EXPANSION.md). The ordered work from contract sync
through runtime composition, scalability, conversion, and FlatGeobuf is in
[IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md).

The current M5 baseline and reproduction procedure are recorded in
[SCALABILITY_BASELINE.md](../reports/SCALABILITY_BASELINE.md).