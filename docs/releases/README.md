# Release records

Each tagged version receives an immutable record describing its scope,
supported behavior, limitations, compatibility, and verification evidence.
The record is prepared in the release commit immediately before the tag.

| Version | Date | Record |
| --- | --- | --- |
| v0.1.0 | 2026-09-04 | [v0.1.0.md](v0.1.0.md) - GeoJSON vertical slice |

## Release gate

A release is ready to tag only after:

1. `VERSION`, `openstrata.toml`, the plugin manifest, the plugin CMake
   project, and the changelog agree.
2. `tools/check_release_metadata.py` passes.
3. `ost ci validate`, `ost configure`, `ost build`, and `ost test` pass.
4. The OpenUSD-free CMake lane passes.
5. The plugin verification pyramid passes through L5.
6. A clean CMake install contains the plugin registration, fixtures,
   manifests, and `LICENSE`, `NOTICE`, and `THIRD_PARTY_NOTICES.md`; the
   OpenStrata package contains its manifest, SBOM, provenance, and fixtures.
7. Repeating the package step without source changes produces the same
   inventory and digest.

The tag must be created on the exact commit that contains the finalized
release record. Changes after the tag belong to the next release.

## Planned sequence

| Version | Theme |
| --- | --- |
| v0.1.0 | GeoJSON vertical slice and deterministic OpenUSD mapping |
| v0.1.x | GeoJSON stabilization, diagnostics, tests, documentation, and artifact reproducibility |
| v0.2.0 | Architecture maturity through M5, runtime composition, shared-format validation, or a selective-read contract |
| v0.3.x and later | Production FlatGeobuf, indexed reads, selective composition, or other evidence-backed expansion |

The roadmap defines the exact release gate; format count alone does not justify
a minor release.