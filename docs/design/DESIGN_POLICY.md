# Development Policy

Last updated: 2026-09-01

This is the standing development policy for `usd-vector-plugins`. Architecture
documents refine it; roadmap documents schedule it. Neither overrides it.

## 1. Purpose and current state

The project provides reusable C++ libraries and OpenUSD FileFormat Plugins for
GIS vector datasets. GeoJSON is the first end-to-end target. The repository is
currently at Milestone 0: documentation contracts exist, but no reader,
authoring library, or plugin is implemented.

The first milestone is complete when a GeoJSON `FeatureCollection` opens as an
OpenUSD layer with deterministic feature prims, typed properties,
precision-safe coordinates, and correct Point, Curve, and Mesh authoring.

## 2. Design principles

### 2.1 Thin plugins

A plugin registers extensions, normalizes arguments, obtains an `ArAsset`,
invokes the reader and shared authoring library, and projects diagnostics into
OpenUSD. Parsing, triangulation, naming policy, and geometry algorithms do not
live in a plugin.

### 2.2 Format-independent core

Geometry, feature identity, properties, bounds, dataset metadata, validation,
and diagnostics belong to `usdVectorCore`. The model describes vector data,
not how a particular USD prim looks.

### 2.3 Streaming-compatible APIs

The public reader API separates metadata from feature iteration. It must not
require `std::vector<Feature>` materialization, even if the first parser uses a
whole-document JSON backend internally.

### 2.4 Preserve meaning

Feature IDs, typed properties, bounds, CRS declarations, and relevant foreign
members are preserved or diagnosed. Unknown information is never silently
discarded. Property names are normalized deterministically and the original
name remains recoverable.

### 2.5 Precision before convenience

Coordinate calculations use `double`. Authored geometry is local to a
deterministic double-precision origin so large projected coordinates are not
narrowed directly into float arrays.

### 2.6 No implicit reprojection

The read path preserves source coordinates and CRS metadata. Reprojection is
an explicit converter or host workflow because axis order, datum, units, and
vertical CRS require policy beyond file-format decoding.

### 2.7 Stable user-facing contracts

Prim hierarchy, prim naming, property namespace, metadata keys, file-format
arguments, diagnostic codes, and geometry mapping are compatibility surfaces.
Changing one requires its architecture or reference document to change in the
same pull request.

### 2.8 Preview and conversion are different jobs

Direct FileFormat reads serve opening and inspection. Expensive partitioning,
payload generation, reprojection, and resumable conversion belong to the
future `usd-vector-convert` command.

## 3. Dependency policy

- Core and GeoJSON reader tests must run without OpenUSD.
- OpenUSD types appear only in the authoring library and plugin adapter.
- JSON and triangulation libraries are pinned, permissively licensed, and
  recorded in `THIRD_PARTY_NOTICES.md` before production use.
- GDAL is not an MVP runtime dependency. It may later be an optional backend or
  a test oracle.
- No HTTP client, cloud SDK, credential type, or resolver implementation is a
  build-time dependency.

## 4. Test policy

1. Pure library tests cover parsing, validation, bounds, property typing,
   identifiers, ring normalization, and diagnostic codes.
2. Authoring tests cover prim types, hierarchy, topology, metadata, local
   origin, and deterministic output with OpenUSD linked.
3. Plugin tests cover discovery, `.geojson` opening, resolver-backed assets,
   arguments, malformed input, and flatten/reopen behavior.

Small hand-authored fixtures are preferred. Semantic assertions are preferred
over full-layer golden files.

## 5. Anti-goals

1. SVG or illustration vector formats.
2. A parser or HTTP implementation inside a FileFormat Plugin.
3. Silent geometry repair or implicit CRS conversion.
4. Stringifying every property value.
5. A whole-dataset-only public reader API.
6. Renderer-specific materials, cameras, or display policy.
7. Write or round-trip support before read preservation rules are stable.