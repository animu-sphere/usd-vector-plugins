# vector-geojson - OpenUSD GeoJSON file-format plugin

The bundle registers `.geojson` and GeoJSON-bearing `.json` sources and authors
an in-memory OpenUSD stage through the shared vector authoring library.

## Layout

```
openstrata.plugin.yaml          bundle contract (identity, runtime, provides, tests)
CMakeLists.txt                  builds the platform library into lib/
cmake/OpenStrataPlugin.cmake    pinned, self-contained build/install mechanics
src/file_format.cpp             the SdfFileFormat adapter
plugin/resources/vector-geojson/plugInfo.json   USD plugin registration
tests/fixtures/                 valid, golden, and negative fixtures
```

The CMake helper is versioned with this scaffold and requires neither an
OpenStrata checkout nor `ost` at build time. Keep bundle-specific targets,
components, resources, and tests in `CMakeLists.txt`.

## Workflow

```powershell
ost plugin inspect plugins/vector-geojson
ost plugin build plugins/vector-geojson
ost plugin doctor plugins/vector-geojson
ost plugin test plugins/vector-geojson --up-to 5
```

The plugin adapts the resolver's `ArAsset` to the GeoJSON reader, applies the
supported file-format arguments, and calls the shared authoring entry point.

## Supported sources and arguments

`.geojson` is always treated as GeoJSON. `.json` is probed by the GeoJSON
reader, so unrelated JSON is rejected with the normal file-format diagnostics.
The supported arguments are `strict=true|false`, `properties=all|none`, and
`geometry=all|points|curves|meshes|none`.

## Authored OpenUSD result

A successful read authors the documented `/Vector` stage hierarchy. Source
coordinates remain recoverable through `vector:localOrigin` and the associated
source metadata. Points, curves, polygon meshes, polygon holes, properties,
and deterministic feature naming are provided by `usdVectorAuthoring`.

## Plugin discovery and installation

`plugin/resources/vector-geojson/plugInfo.json` is the discovery root. Set
`PXR_PLUGINPATH_NAME` to that directory after installing the bundle; see
[`INSTALL.md`](../../docs/guides/INSTALL.md). The CMake configure step
regenerates its platform-specific `LibraryPath` before a build or package is
used.

The aggregate plugin product installs its runtime acceptance probe at
`share/usd-vector-plugins/probes/packaged_probe.py`. The probe loads the
registered GeoJSON format, opens both the `.geojson` and GeoJSON-bearing
`.json` fixtures, verifies the authored point, feature identity, typed
property, and local-origin metadata, and confirms that unrelated JSON and
invalid GeoJSON are rejected.

## Runtime and compatibility

The bundle targets OpenUSD `>=26.08,<27.0`, platform `cy2026`, profile `usd`.
It links the in-repository `usdVectorCore`, `usdGeoJson`, and
`usdVectorAuthoring` libraries. No resolver or transport implementation is
linked.

## Known limitations

The current bundle covers the local resolver and synchronous stage-read path.
Golden roundtrip coverage is provided for the basic fixture; broader runtime
matrix, resolver integration, and streaming performance coverage remain future
work.

## Licensing

The plugin follows the repository Apache-2.0 license.
