# Installation

Status: installation layout is a planned contract. No installable plugin
artifact exists yet.

## Planned layout

```text
<prefix>/
  bin/                         optional tools
  lib/                         shared libraries
  include/                     public library headers
  plugin/usd/vector-geojson/
    plugInfo.json
    resources/
```

The installed `plugInfo.json` uses paths relative to its bundle. Installation
must not embed a build-directory path.

## CMake installation

Once targets exist:

```powershell
cmake --install build/full --prefix C:\usd-vector-plugins
```

Add the directory containing the plugin registration metadata to
`PXR_PLUGINPATH_NAME` according to the installed bundle layout:

```powershell
$env:PXR_PLUGINPATH_NAME = "C:\usd-vector-plugins\plugin\usd;$env:PXR_PLUGINPATH_NAME"
```

Use OpenUSD's plugin inspection or an integration test to confirm that
`UsdVectorGeoJsonFileFormat` is discovered before attempting to open a source.
Do not copy only the shared library; registration metadata and relative
resources are part of the artifact.

## Compatibility

The plugin must be built against an ABI-compatible OpenUSD runtime and C++
toolchain. Release artifacts will be target-qualified rather than advertised
as universally portable. Resolver implementations are installed separately
and compose at runtime through OpenUSD.