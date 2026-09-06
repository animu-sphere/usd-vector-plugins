# Release plan: v0.2.0

- Date: 2026-09-06
- Baseline: `e811359` on `main`
- Trigger: composition requirements from `usd-geospatial-runtime` @ `7c06143`
- Release story (per [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) section 3):
  runtime-composition validation, M6

`usd-geospatial-runtime` cannot compose the published 0.1.0 product. This plan
records what was verified, what actually caused it, and the ordered work that
makes a composable 0.2.0.

## 1. What was verified

The consumer's diagnosis is correct, and the repository configuration was
already right. Every pin in source names the runtime the composition expects:

| Checked | Value | Verdict |
| --- | --- | --- |
| [strata.lock](../../strata.lock) `runtime.digest` | `sha256:3a4e3993…` | correct |
| [openstrata.ci.yaml](../../openstrata.ci.yaml), all four cells | `sha256:ebb0c7da…` | correct |
| [release.yml](../../.github/workflows/release.yml) `RUNTIME_ARTIFACT` | `sha256:ebb0c7da…` | correct |
| Local `~/.ost` store, `openstrata-cy2026-windows-x86_64-py313-usd` | `sha256:3a4e3993…` | correct |
| Local `dist/…/0.1.0/…/manifest.json` `provenance.runtime.digest` | **`sha256:ce996432…`** | **defective** |

`sha256:ebb0c7da…` and the composition's `sha256:51c19df2…` are different
archives of the same runtime identity `sha256:3a4e3993…`.

So the managed runtime on the release machine was never wrong. The packaged
product simply did not use it.

## 2. Root cause

The build environment overrode the managed runtime. The bundle's
`validation/environment.json` in the product stage records:

```text
PATH              C:/usd/openusd-26.08-cy2026/bin
CMAKE_PREFIX_PATH C:/usd/openusd-26.08-cy2026
```

`C:/usd/` holds 27 hand-placed OpenUSD trees from earlier runtime and packaging
experiments. One of them was on the path when the product was packaged. `ost`
accepted it, recorded it as `provenance.runtime.source: "local"`, computed
digest `sha256:ce996432…` — an identity present in no registry — and still
reported `provenance.validation.passed: true` and
`build_outputs.origin: "ost-managed"`.

Nothing in the source tree is wrong, and no source change fixes this. It is a
release-process defect with two distinct halves:

1. **Environment leakage.** The product was packaged from a developer working
   tree whose environment pointed at an unmanaged OpenUSD tree.
2. **No publish path.** [release.yml](../../.github/workflows/release.yml)
   builds against the correct pinned runtime, but it only creates a *draft
   GitHub release*. It never pushes to GHCR, and the repository contains no
   OCI publish tooling. The published
   `ghcr.io/animu-sphere/usd-vector-plugins` 0.1.0 artifact was therefore
   pushed by hand from the developer machine — which is exactly the machine
   with the leaked environment.

   The publish is not merely missing from the workflow, it is *out of order*.
   It happens after the workflow has ended, on a different machine, with
   nothing tying what gets pushed to what CI built and validated. Any artifact
   present on the operator's disk can occupy that slot, and one did.

The second half is the one that matters. As long as publishing is a manual
step outside CI, fixing this once does not stop it recurring.

## 3. Scope correction against the requirements note

The consumer's note states that running the existing tag-triggered release
workflow satisfies R1 as written. That is true for R1 only. Two acceptance
conditions are not reachable from the current workflow:

- **R2** requires the product published as an immutable OCI artifact pinned by
  digest. `release.yml` has no push step.
- **R5** requires SBOM and, if possible, in-toto provenance on the release
  path. The SBOM is already staged as
  `usd-vector-plugins.sbom.spdx.json`, but nothing attaches it to a registry
  artifact, and no provenance is attested.

One further correction: R1's acceptance names `runtime_digest` at the manifest
root. The actual key is `provenance.runtime.digest`. The check must read the
real path.

## 4. Ordered work

### Step 1 — Gate the drift before anything else

[tools/check_release_metadata.py](../../tools/check_release_metadata.py)
currently verifies only that version declarations agree with `VERSION`. It is
already invoked from [release.yml](../../.github/workflows/release.yml) and
from [CMakeLists.txt](../../CMakeLists.txt), so it is the cheapest place to
add the missing gate.

Add: the packaged product manifest's `provenance.runtime.digest` must equal
`strata.lock`'s `runtime.digest`, and `provenance.runtime.source` must not be
`local`. Either condition alone would have caught 0.1.0.

This gate is the deliverable that generalizes. `usd-raster-plugins` has the
same drift waiting in its working tree, and the same check belongs there.

### Step 2 — R4, report a feature count

Add `observations.featureCount` to
[tools/acceptance/packaged_probe.py](../../tools/acceptance/packaged_probe.py).

Count the authored children of `/Vector/Features` on the opened stage, not the
length of the source `features` array. The requirement exists so that a plugin
which regressed into refusing everything cannot produce a healthy-looking
record; only a count taken from the read result can carry that meaning.

`basic.geojson` currently holds one feature, so `featureCount: 1` satisfies the
stated acceptance but is weak evidence. Add a multi-feature fixture so the
count discriminates. Fixture digests change in 0.2.0 regardless, so this costs
nothing extra.

R3 constrains this step: the probe keeps its installed path
`share/usd-vector-plugins/probes/packaged_probe.py`, its
`--prefix <composed prefix>` invocation, its self-resolved fixtures, its
non-zero exit on failure, and its single JSON object on stdout.

### Step 3 — R2, version declarations

`check_release_metadata.py` requires seven declarations to agree. All must move
together or the release fails its own gate:

| File | Form |
| --- | --- |
| `VERSION` | `0.2.0` |
| `openstrata.toml` | `version = "0.2.0"` |
| `plugins/vector-geojson/openstrata.plugin.yaml` | `version: 0.2.0` |
| `plugins/vector-geojson/CMakeLists.txt` | `VERSION 0.2.0` |
| `CHANGELOG.md` | `[Unreleased]` closed into `## [0.2.0] - <date>` |
| `docs/releases/v0.2.0.md` | new, first line `# v0.2.0` |
| `docs/releases/README.md` | new table row |

### Step 4 — R2 and R5, publish from the packaging job itself

`ost` already provides the producer verb:

```text
ost artifact push <digest> oci://ghcr.io/animu-sphere/usd-vector-plugins[:tag][@sha256:…]
```

It verifies the pinned digest against the computed manifest digest and applies
publisher identity policy from `openstrata-artifact-policy.toml`.

**The push belongs in the packaging job, not in a later one.** This is the
ordering half of the defect and it matters as much as the runtime pin.

Today `release.yml` has two jobs: `build` packages the product, and `release`
downloads the uploaded assets and assembles a draft GitHub release. Adding a
third job that re-imports an artifact and pushes it would recreate the same
seam that produced 0.1.0 — a step whose input is "some artifact from
somewhere" rather than "the artifact this job just built against the pinned
runtime".

Instead, extend the existing `Package release artifacts` step in the `build`
job. It already runs:

```text
ost plugin package --workspace --product --target … --profile … --json \
  | tee release-stage/package.json
```

Immediately after that, in the same job, on the same runner, with the pinned
runtime still materialized:

1. run the step 1 runtime-digest gate against the freshly written
   `dist/products/…/manifest.json`;
2. `ost artifact import` the packaged product into the local registry;
3. `ost artifact push` it to
   `oci://ghcr.io/animu-sphere/usd-vector-plugins` pinned by digest;
4. attach the already-staged `usd-vector-plugins.sbom.spdx.json` and attest
   in-toto provenance;
5. record the resulting OCI digest into `release-stage/` so the draft release
   notes state the digest consumers should pin.

The artifact that reaches GHCR is then, by construction, the one that was built
and validated against `sha256:3a4e3993…` seconds earlier. There is no window in
which a different artifact can be substituted, and no human-operated publish
step to forget or to run from a laptop.

Ordering consequence for the `release` job: it keeps assembling the draft
GitHub release, but it is now downstream of a completed publish rather than
being the terminal step. The GHCR artifact is the composable product; the
GitHub release is the human-readable record of it. If the push fails, the
draft release is never assembled, which is the correct failure direction —
today the reverse is possible, and 0.1.0 is what that looks like.

This is the largest piece of work and the only one that makes the fix durable.
Step 1 protects artifacts that CI packages; it cannot protect an artifact a
human pushes from a laptop. Until publishing moves into the tagged workflow,
R5's provenance stays `skipped` on pull and the 0.1.0 failure mode remains
reachable.

## 5. Sequencing and release gate

Steps 1 through 3 are independent of step 4 and can land first. The release is
cut only when all of the following hold:

1. `ost lock --check` passes, or its known false negative is understood
   (see [OST report 02](../reports/ost/02-2026-09-06-v0.22.8-release-provenance-dogfooding.md)).
2. `check_release_metadata.py` passes, including the new runtime-digest gate.
3. The tag-triggered workflow — not a working tree — produces the artifact.
4. The product manifest records `provenance.runtime.digest: sha256:3a4e3993…`
   and a `validation/environment.json` naming a path under the managed runtime
   store.
5. The packaged probe reports `observations.featureCount` as a positive integer
   alongside `unrelatedJsonRejected` and `invalidGeoJsonRejected`.
6. The product is pushed to GHCR by digest with its SBOM, from the same job
   and runner that packaged it, before the draft GitHub release is assembled.

## 6. Open decisions

| Decision | Options | Recommendation |
| --- | --- | --- |
| Include step 4 in 0.2.0 | Publish from the packaging job now, or keep pushing by hand under a discipline of publishing only CI-built artifacts | Automate. The manual path is the root cause; a discipline is not a gate. |
| Where the push runs | Inside the `build` job right after `ost plugin package`, or as a separate downstream job | Inside `build`. A separate job takes "an artifact" as input; the packaging job takes the artifact it just produced. |
| Multi-feature fixture | Add one, or ship `featureCount: 1` | Add one. The requirement's intent needs a count that can discriminate. |

## 7. Downstream

Once published, `usd-geospatial-runtime` pins the new archive and OCI digests,
recomposes, runs its six-check acceptance including `vector`, and cuts its own
release. The blocker is tracked as P0 in that repository's roadmap.

## Related documents

- [Implementation plan](IMPLEMENTATION_PLAN.md)
- [Roadmap status](README.md)
- [OST report 02: release provenance](../reports/ost/02-2026-09-06-v0.22.8-release-provenance-dogfooding.md)
- [Release records](../releases/README.md)
