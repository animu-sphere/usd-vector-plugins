# Roadmap

The v0.1.0 GeoJSON vertical slice is the compatibility baseline. Current work
stabilizes that baseline and establishes the M5 scalability contract before
runtime composition, FlatGeobuf, and indexed partial reads. Status is tracked
against acceptance evidence, not directory presence or format count.

| Milestone | Outcome | Status |
| --- | --- | --- |
| M0 | Documentation contracts, root project skeleton, CI, licensing, notices | implemented |
| M1 | `usdVectorCore`: model, bounds, validation, diagnostics, identifiers | implemented |
| M2 | `usdGeoJson`: FeatureCollection, all MVP geometries, properties, fixtures | implemented |
| M3 | `usdVectorAuthoring`: Points, Curves, Mesh triangulation, metadata, origin | implemented |
| M4 | `vector-geojson`: registration, ArAsset adapter, arguments, integration tests | implemented: OpenStrata L0-L5 verified |
| M5 | Scalability measurement and evidence-led streaming improvements | in progress: baseline, cursor-based lazy materialization, and shared bounded batches |
| M6 | Runtime composition validation with `usd-geospatial-runtime` | planned |
| M7 | FlatGeobuf as a format-independent architecture validation | deferred |
| M8 | Indexed partial-read and selective-composition contract | deferred |

## Delivery phases

```text
Stabilize -> Scale -> Compose -> Generalize -> Select
 v0.1.x       M5       M6          M7          M8
```

The canonical sequence, gates, and completion conditions are in
[IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md). This file records current
status; it does not duplicate the implementation plan.

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

Rules for admitting a second format are in
[FORMAT_EXPANSION.md](FORMAT_EXPANSION.md).

The current M5 baseline and reproduction procedure are recorded in
[SCALABILITY_BASELINE.md](../reports/SCALABILITY_BASELINE.md).