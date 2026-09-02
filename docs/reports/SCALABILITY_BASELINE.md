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

The default run includes 1,000 and 100,000 points, 1,000 16-vertex lines,
one 1,000-vertex polygon, 1,000 small polygons, 1,000 property-heavy points,
and 1,000 points with large coordinates. A single case and count can be run
with `--case NAME --count N`.

## Metric contract

| Column | Meaning |
| --- | --- |
| `requested_count` | Requested case size; its interpretation depends on the case (for example, feature count or polygon vertex count). |
| `source_bytes` | Generated GeoJSON source size. |
| `features`, `vertices` | Counts recovered by the reader. |
| `parse_ms` | `Reader::Create`, which parses the complete buffered source. |
| `time_to_first_feature_ms` | Time from reader creation start through the first `ReadNext`. |
| `time_to_open_ms` | Time until `Reader::Create` returns; equal to parse time for the current backend. |
| `authoring_plan_ms` | Time for `BuildAuthoringPlan`. |
| `peak_rss_bytes` | Process peak working set on Windows. Run one case per process for an isolated value. |
| `copied_bytes` | Known source handoff copy: the benchmark passes the source as an lvalue to the by-value reader API. |
| `retained_feature_bytes` | Estimated retained feature, geometry, and property capacity, not an allocator trace. |
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
expected: the current reader parses and stores the whole JSON document in
`Reader::Create`. The next M5 step is to measure a streaming candidate against
this report; no backend replacement is claimed by this baseline.
