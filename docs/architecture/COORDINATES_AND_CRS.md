# Coordinates and CRS

This document defines how source coordinates remain precise and recoverable in
OpenUSD. It does not define reprojection.

## 1. Source semantics

The reader stores coordinate components as `double` and preserves their order.
For RFC 7946 GeoJSON, positions are longitude, latitude, and optional height in
WGS 84 semantics. A legacy `crs` member is preserved and diagnosed as an
extension; it is not silently treated as RFC 7946.

The plugin never swaps axes, changes datum, converts angular units to meters,
or applies a vertical transformation.

## 2. Local origin

Large coordinates are not narrowed directly into float geometry arrays. The
authoring library chooses one deterministic dataset origin in source space and
authors local positions:

$$
\mathbf{p}_{local} = \mathbf{p}_{source} - \mathbf{o}_{source}
$$

Recovery is:

$$
\mathbf{p}_{source} = \mathbf{o}_{source} + \mathbf{p}_{local}
$$

The origin is the center of the computed dataset bounds, with each component
calculated in `double`. If bounds are unavailable because no geometry exists,
the origin is `(0, 0, 0)`. The chosen value is authored as
`vector:localOrigin` and as a double-precision transform on `/Vector`.

Geometry points remain float where required by the selected USD schema. A
conversion that cannot preserve the configured local precision fails rather
than silently producing collapsed geometry.

## 3. Bounds

Computed bounds use all valid feature geometry in source coordinates and
`double` precision. Source-provided dataset and feature `bbox` values are kept
separately. Strict mode rejects an invalid or contradictory `bbox`; default
mode warns and uses computed bounds for origin selection.

Longitude wrapping and antimeridian-spanning bounds are preserved as source
semantics in the MVP; they are not normalized into a different range.

## 4. Stage metadata

The plugin does not claim that angular source coordinates are meters. It
authors the source CRS declaration, original bounds, and local origin so a host
can interpret the stage deliberately. `metersPerUnit` and up-axis are not
currently invented from a GeoJSON document that does not define them. A
planned direct-read stage policy will author `/Vector` as the default prim and
leave those stage values unset unless explicitly requested. FileFormat
arguments may provide them when a host or runtime requires them. That policy
will not swap source axes, reproject, or convert source units.

An explicit converter may reproject into a metric, Y-up or Z-up stage. Such a
conversion records source CRS, target CRS, axis mapping, units, and converter
version in the generated asset.

## 5. Test invariants

- Large projected coordinates retain small local differences.
- Negative and three-dimensional coordinates round-trip through origin recovery.
- Empty and null-geometry datasets choose a deterministic zero origin.
- Declared and computed bounds remain distinguishable.
- No reader test requires OpenUSD to validate coordinate semantics.