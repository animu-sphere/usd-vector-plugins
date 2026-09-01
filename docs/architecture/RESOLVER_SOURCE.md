# Resolver-backed source contract

OpenUSD owns asset resolution. Vector readers own byte interpretation. The
FileFormat Plugin is the adapter between those boundaries.

```text
HTTP / S3 / package / authentication / retry
                    |
                ArResolver
                    |
                  ArAsset
                    |
          vector-geojson adapter
                    |
             usdGeoJson reader
```

## 1. Rules

1. The plugin resolves and opens assets through the active `ArResolver`.
2. The reader consumes a project-owned readable source abstraction, not a URL
   and not an `ArResolver` implementation.
3. No vector module retries, authenticates, interprets HTTP status, or manages
   a transport cache.
4. Resolver validation tokens and identities remain opaque.
5. Credentials, signed URLs, authorization headers, and tokens never appear in
   diagnostics, generated USD, cache descriptors, or test fixtures.
6. Local files, memory sources, and resolver-provided assets produce identical
   semantic results for identical bytes.

## 2. Source operations

The MVP GeoJSON source needs bounded reads and a known-size query when the
underlying asset provides them. The abstraction must not require a filesystem
path. A parser backend may buffer the complete document for correctness, but
that implementation choice does not change the public source boundary.

Future FlatGeobuf range reads use the same boundary with seek/read-at
capabilities. A lack of efficient random access is reported as a performance
constraint, not worked around with a private HTTP stack.

## 3. Verification

Contract tests run against local-file and in-memory implementations without an
external resolver repository. Plugin integration adds an `ArAsset` adapter
test. Cross-repository HTTP or cloud tests are composed separately and are not
required to build the core libraries.