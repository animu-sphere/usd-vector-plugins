# OST Reports

These reports are append-only records of OpenStrata (`ost`) adoption and
failure modes observed in this repository. They preserve commands, runtime
versions, observed diagnostics, repository-side fixes, and follow-up asks.

## Reading Order

| Report | Date | Subject | OST version | Result |
| --- | --- | --- | --- | --- |
| [01](01-2026-09-02-v0.22.8-geojson-fileformat-dogfooding.md) | 2026-09-02 | `usd-fileformat-cpp` GeoJSON bundle, standalone build, runtime gates, and L3 serialization | 0.22.8 | Repository-side integration issues fixed; L0-L5 and workspace tests pass |

Reports are historical evidence. When a later OpenStrata version changes an
observation, add a new report rather than rewriting an old one.

## Open asks

| Report | Priority | Ask | State |
| --- | --- | --- | --- |
| [01](01-2026-09-02-v0.22.8-geojson-fileformat-dogfooding.md) | P3 | Clarify or improve provenance attribution when a plugin is built separately and then included unchanged by `ost build` | informational |
