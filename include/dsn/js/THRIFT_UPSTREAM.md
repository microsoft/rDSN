# Apache Thrift JavaScript runtime

`thrift.js` is based on `lib/js/src/thrift.js` from Apache Thrift 0.9.3. Its
`Thrift.UpstreamVersion` field records that baseline; `Thrift.RdsnPatchLevel` changes whenever
the local compatibility layer changes.

The file retains the Apache License 2.0 header. rDSN-specific RPC transport, custom wire types,
module packaging, and generated-client behavior are deliberately kept outside the vendored
runtime in:

- `dsn_transport.js`
- `dsn_types.js`
- `index.cjs` and `index.mjs`
- `node.cjs` and `node.mjs`

The remaining changes inside `thrift.js` are protocol-level compatibility patches that cannot
be implemented solely by an external transport:

- exact signed and unsigned 64-bit decimal conversion;
- lossless JSON parsing for integers outside JavaScript's safe range;
- C++-compatible unpadded base64 encoding and padded-input acceptance;
- control-character and special-double handling;
- negative-size, recursion-depth, unknown-type, and set-uniqueness validation;
- corrected nested collection traversal;
- CommonJS export support.

When updating the baseline:

1. Start from the named Apache Thrift release rather than another project's bundled copy.
2. Reapply only the protocol patches listed above; keep rDSN RPC behavior in the separate files.
3. Update `Thrift.UpstreamVersion`, increment `Thrift.RdsnPatchLevel`, and update this inventory.
4. Regenerate the webstudio copies and generated clients.
5. Build `dsn.idl.tests` and run the JavaScript interoperability checks on the target platforms.
