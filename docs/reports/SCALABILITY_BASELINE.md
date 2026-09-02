# GeoJSON scalability baseline

Date: 2026-09-02

The baseline runner is `tools/usd-vector-benchmark`. It measures the current
buffered `usdGeoJson` reader before any streaming replacement is attempted.
The runner is OpenUSD-independent; an OpenUSD-enabled build also reports stage
emission time and flattened layer size.

## Reproduce

```powershell
cmake -S . -B build/m5 -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DUSDVECTOR_ENABLE_OPENUSD=OFF `
    -DUSDVECTOR_ENABLE_BENCHMARKS=ON
cmake --build build/m5 --target usd-vector-benchmark
.\build\m5\tools\usd-vector-benchmark\usd-vector-benchmark.exe `
    --output build/m5/baseline.csv
```

To isolate reader work from authoring and USD emission, run one case per
process with `--reader-only`:

```powershell
.\build\m5\tools\usd-vector-benchmark\usd-vector-benchmark.exe `
    --reader lazy --reader-only --case points --count 1000
```

Reader-only mode still opens the source and iterates every feature so the
feature and vertex counts remain meaningful. It discards each materialized
feature after counting it, skips `BuildAuthoringPlan`, and leaves the
authoring and USD columns at zero or blank.

The default run includes 1,000 and 100,000 points, 1,000 16-vertex lines,
one 1,000-vertex polygon, 1,000 small polygons, 1,000 property-heavy points,
and 1,000 points with large coordinates. A single case and count can be run
with `--case NAME --count N`. The default reader is `buffered`; the
M5-compatible source-span lazy materialization candidate can be measured with
`--reader lazy`. Both modes use the same project-owned reader contract and
produce the same semantic feature sequence. The lazy candidate retains the
source text and lightweight feature ranges, rather than a whole JSON DOM or
all project-owned Features.

To preserve the existing eager failure and metadata contract, `CreateLazy` also
validates each source feature while opening and discards that temporary model.
The output Feature is then materialized again when `ReadNext` is called. This
candidate reduces retained project-owned feature state after open, but it does
not yet reduce open-time feature parsing work or deliver the first feature before
the complete source has been validated.

The benchmark materializes every feature again for the shared authoring-plan
measurement. Consequently, `peak_rss_bytes` and `retained_feature_bytes` in a
lazy run describe the complete reader-plus-authoring workflow, not the reader's
post-open retained state. Use `time_to_open_ms`, `time_to_first_feature_ms`,
and a dedicated reader-only process measurement when evaluating this candidate.

## Metric contract

| Column | Meaning |
| --- | --- |
| `reader` | Reader backend: `buffered` retains project-owned Features after open; `lazy` retains source text and feature ranges and materializes one Feature per `ReadNext`. |
| `case` | Benchmark case name: `points`, `lines`, `large-polygon`, `small-polygons`, `property-heavy`, or `large-coordinates`. |
| `requested_count` | Requested case size; its interpretation depends on the case (for example, feature count or polygon vertex count). |
| `source_bytes` | Generated GeoJSON source size. |
| `features`, `vertices` | Counts recovered by the reader. |
| `parse_ms` | `Reader::Create`, including complete-source validation; `lazy` parses one temporary feature at a time while opening. |
| `time_to_first_feature_ms` | Time from reader creation start through the first `ReadNext`. |
| `time_to_open_ms` | Time until `Reader::Create` returns; equal to parse time for the current backend. |
| `authoring_plan_ms` | Time for `BuildAuthoringPlan`. |
| `peak_rss_bytes` | Process peak working set on Windows. Run one case per process for an isolated value. |
| `copied_bytes` | Known source handoff copies. The benchmark moves its generated source into the by-value reader API and reports zero for this handoff; any nonzero value identifies an explicit copy in the measured path. |
| `retained_feature_bytes` | Estimated feature, geometry, and property capacity retained by the full benchmark workflow, not an allocator trace. Reader-only mode reports zero because each materialized feature is discarded after counting. |
| `usd_emission_ms` | OpenUSD-enabled builds only. |
| `flattened_layer_bytes` | OpenUSD-enabled builds only; serialized root layer size. |

## Observed baseline

The following values were captured on Windows with MSVC 19.51, OpenUSD off,
and one default process. Times are wall-clock milliseconds.

| Case | Features | Vertices | Source bytes | Parse | Authoring plan | Retained feature |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| points / 1,000 | 1,000 | 1,000 | 112,271 | 3.1 | 0.9 | 216,000 |
| points / 100,000 | 100,000 | 100,000 | 11,822,271 | 404.4 | 206.6 | 21,600,000 |
| lines / 1,000 | 1,000 | 16,000 | 452,189 | 11.2 | 1.9 | 755,744 |
| large polygon / 1,000 vertices | 1 | 1,000 | 42,970 | 1.8 | 7.5 | 32,248 |
| small polygons / 1,000 | 1,000 | 4,000 | 204,387 | 5.1 | 1.8 | 353,248 |
| property-heavy / 1,000 | 1,000 | 1,000 | 702,295 | 21.2 | 10.4 | 3,120,000 |
| large coordinates / 1,000 | 1,000 | 1,000 | 132,931 | 2.8 | 0.9 | 216,000 |

A single-process Release run of the 100,000-point case reported peak RSS of
230,961,152 bytes. The default multi-case run reported the same order of peak
because RSS is process-wide and cumulative. With a provisional 200 MiB
comparison threshold, 1,000 points remains below the threshold while 100,000
points is the first measured case above it. This is an evidence boundary for
follow-up work, not a universal hardware limit.

The first-feature time is effectively the open time for every case. That is
expected for both current readers because `Create` and `CreateLazy` validate the
complete source before returning. The buffered reader stores project-owned
Features, while the lazy candidate stores source text and feature ranges and
discards each temporary Feature after validation. Its results are evidence for
reduced retained model state only; an incremental parser must be measured
separately before claiming bounded memory or earlier first-feature delivery.

## Source-span candidate observation

On 2026-09-03, the same OpenUSD-free Release benchmark was rerun for the
100,000-point case after `CreateLazy` changed to retain source text and feature
ranges instead of a JSON DOM. The lazy run completed with 200,802,304 bytes of
process peak working set and 1,451.1 ms open time, compared with 231,927,808
bytes and 1,607.4 ms for buffered. These are complete reader-plus-authoring
process measurements, not isolated post-open reader allocations. The candidate
still validates the complete source before returning from `CreateLazy`, so the
measurement does not establish earlier first-feature delivery or a universal
memory limit.

## Source handoff observation

On 2026-09-03, the benchmark source handoff was changed from an lvalue to a
move into both reader factories. A 1,000-point run reported `copied_bytes=0`
for buffered and lazy readers while retaining the same 1,000 feature and 1,000
vertex counts. This removes the benchmark's known full-source handoff copy;
it does not claim that parser or authoring allocations are copy-free.

## Reader-only observation

On 2026-09-03, reader-only runs for 1,000 points reported zero
`authoring_plan_ms` and zero `retained_feature_bytes` for both buffered and
lazy readers while retaining the same feature and vertex counts. The lazy
run reported 14.1 ms parse time and 14.2 ms to first feature; the buffered run
reported 15.8 ms for both measurements. These values are single-process
observations, not a hardware-independent performance claim. The mode makes
the reader comparison explicit; it does not change the current lazy reader's
complete-source validation behavior.
