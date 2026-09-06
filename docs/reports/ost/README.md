# OST Reports

These reports are append-only records of OpenStrata (`ost`) adoption and
failure modes observed in this repository. They preserve commands, runtime
versions, observed diagnostics, repository-side fixes, and follow-up asks.

## Reading Order

| Report | Date | Subject | OST version | Result |
| --- | --- | --- | --- | --- |
| [01](01-2026-09-02-v0.22.8-geojson-fileformat-dogfooding.md) | 2026-09-02 | `usd-fileformat-cpp` GeoJSON bundle, standalone build, runtime gates, and L3 serialization | 0.22.8 | Repository-side integration issues fixed; L0-L5 and workspace tests pass |
| [02](02-2026-09-06-v0.22.8-release-provenance-dogfooding.md) | 2026-09-06 | Release provenance: a validated product packaged against an unmanaged runtime, and lock verification | 0.22.8 | Cause identified; repository-side gate planned; three OST asks raised |

Reports are historical evidence. When a later OpenStrata version changes an
observation, add a new report rather than rewriting an old one.

## Open asks

| Report | Priority | Ask | State |
| --- | --- | --- | --- |
| [01](01-2026-09-02-v0.22.8-geojson-fileformat-dogfooding.md) | P3 | Clarify or improve provenance attribution when a plugin is built separately and then included unchanged by `ost build` | informational |
| [02](02-2026-09-06-v0.22.8-release-provenance-dogfooding.md) | P1 | Compare a packaged product's runtime digest against `strata.lock` during `ost package` / `ost validate` | blocking downstream composition, worked around locally |
| [02](02-2026-09-06-v0.22.8-release-provenance-dogfooding.md) | P2 | Make `ost lock --check` compare lock content rather than bytes | blocks CI adoption of the drift gate |
| [02](02-2026-09-06-v0.22.8-release-provenance-dogfooding.md) | P3 | Align `ost lock --check` exit code with its documented value | informational |
