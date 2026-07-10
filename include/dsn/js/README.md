# rDSN JavaScript client binding

The JavaScript binding generates browser clients for calling rDSN services over HTTP with
Thrift JSON. It is an RPC client binding, not a JavaScript implementation of the rDSN
application runtime.

## Generate a client

Build rDSN first so that the customized Thrift compiler is available, then run:

```sh
bin/dsn.cg.sh path/to/service.thrift js generated json single
```

On Windows, use `bin\dsn.cg.bat` with the same arguments. JavaScript generation supports
Thrift IDL with the JSON format.

The output includes:

- Apache Thrift JavaScript types generated from the IDL.
- `<program>.client.js`, containing one `<service>App` client per service.
- `<program>.types.cjs`, `<program>.client.cjs`, and `<program>.client.mjs` for Node.js.
- `<program>.test.html`, a basic browser test page.

## Enable HTTP RPC

The server process must load `dsn.tools.common`, which registers the HTTP message parser:

```ini
[modules]
dsn.tools.common
```

The binding calls the regular rDSN RPC port with URLs in this form:

```text
/DSF_THRIFT_JSON/<thread_hash>/<RPC_CODE>
```

The HTTP parser currently permits cross-origin browser calls. Do not expose an unauthenticated
rDSN RPC port directly to an untrusted network; use an authenticated reverse proxy or another
trusted-network boundary for production deployments.

## Load the scripts

Load the runtime before the generated files:

```html
<script src="thrift.js"></script>
<script src="dsn_transport.js"></script>
<script src="dsn_types.js"></script>
<script src="generated/service_types.js"></script>
<script src="generated/service.client.js"></script>
```

Included Thrift programs must also be loaded before making calls that use their types. These
browser files continue to expose globals for compatibility.

## Use from Node.js

The `include/dsn/js` directory is the `rdsn-client` npm package. Install it directly from a
source checkout or from an archive produced by `npm pack`:

```sh
npm install /path/to/rDSN/include/dsn/js
```

CommonJS:

```js
const { cliApp } = require("./generated/cli.client.cjs");
const { command } = require("./generated/cli.types.cjs");

const client = new cliApp("http://127.0.0.1:20101");
client.callAsync(new command({
    cmd: "counter.list",
    arguments: [""]
})).then(console.log);
```

ES modules:

```js
import { cliApp } from "./generated/cli.client.mjs";
import types from "./generated/cli.types.cjs";

const client = new cliApp("http://127.0.0.1:20101");
const result = await client.callAsync(
    new types.command({ cmd: "counter.list", arguments: [""] }));
```

The generated CommonJS type wrapper evaluates trusted Thrift compiler output in an isolated
Node.js VM context. Do not use it to load untrusted JavaScript.

## Call a service

Given the CLI service in `src/dev/cpp/cli.thrift`, the generated Promise API is:

```js
const client = new cliApp("http://127.0.0.1:20101");

client.callAsync(new command({
    cmd: "counter.list",
    arguments: [""]
})).then(function(result) {
    console.log(result);
}).catch(function(error) {
    console.error(error.code, error.message);
});
```

Every generated RPC method has a corresponding `<method>Async(args, hashOrOptions)` method. It
resolves to the decoded RPC result. Transport and server failures reject with `DsnRpcError`;
input and serialization errors retain their original JavaScript error type.

Client defaults and per-call overrides support headers, timeouts, cancellation, Fetch
credentials, and a custom Fetch implementation:

```js
const client = new cliApp("https://rpc.example.test/rdsn", {
    timeoutMs: 5000,
    headers: { Authorization: "Bearer token" },
    credentials: "include"
});

const controller = new AbortController();
const result = await client.callAsync(request, {
    hash: 17,
    timeoutMs: 1000,
    signal: controller.signal,
    headers: { "X-Request-ID": "request-123" }
});
```

`timeoutMs` defaults to `0`, which disables the client timeout. Calling `abort()` on the signal
rejects the Promise with code `CANCELLED`; a client timeout rejects it with `CLIENT_TIMEOUT`.
Per-call headers override client headers case-insensitively. Set a header value to `null` to
remove an inherited header.

Non-safelisted headers on a cross-origin browser call cause a CORS preflight. Use a reverse
proxy that handles `OPTIONS` and explicitly allows those headers; the direct rDSN HTTP parser
does not provide a general-purpose CORS policy for arbitrary application headers.

The compatibility callback API remains available:

```js
client.call({
    args: new command({ cmd: "counter.list", arguments: [""] }),
    hash: 0,
    async: true,
    timeoutMs: 5000,
    on_success: function(result) {
        console.log(result);
    },
    on_fail: function(xhr, textStatus, error) {
        console.error(error.code, error.message);
    }
});
```

An asynchronous callback call returns its underlying transport handle. Its exact type depends
on the environment, so portable code should use the generated Promise method instead.
Synchronous calls (`async: false`) remain for compatibility but rely on synchronous XHR and
emit a one-time deprecation warning. Timeouts and cancellation are not accepted for synchronous
calls.

Endpoints must be absolute HTTP or HTTPS URLs without credentials, query strings, or fragments.
Trailing slashes are normalized. Invalid endpoints throw `DsnRpcError` with code `INVALID_URL`.

## Errors

Transport and server failures use `DsnRpcError`, also available through the compatibility alias
`DSN.RpcError`. Important properties are:

| Property | Description |
| --- | --- |
| `code` | rDSN error such as `ERR_TIMEOUT`, `HTTP_<status>`, `CLIENT_TIMEOUT`, `CANCELLED`, or `TRANSPORT_ERROR` |
| `status` | HTTP status, or `0` when no HTTP response was received |
| `response` | Fetch, XHR, or jQuery response object when available |
| `responseText` | Response body when available |
| `cause` | Original transport or parsing error when available |

The server reports rDSN failures through the `server_error` response header even though the
HTTP status is normally 200. The binding checks both this header and the HTTP status before
decoding a result.

## Exact integers

JavaScript numbers cannot exactly represent every 64-bit integer.

- Signed and unsigned 64-bit writers accept a safe integer, a decimal string, or `BigInt`.
- An unsafe JavaScript number is rejected instead of being silently rounded.
- Decoded values within the safe integer range are numbers; larger values are decimal strings.
- `uint64_t` uses signed Thrift `i64` wire bits because Thrift JSON has no unsigned 64-bit type.
  For example, `18446744073709551615` is encoded as `-1` and decoded back to the unsigned
  decimal string.
- Unsigned 8-, 16-, and 32-bit values use their same-width signed Thrift wire bits.

## rDSN wire types

`dsn_types.js` provides the rDSN-specific types used by generated clients:

| Type | JavaScript representation |
| --- | --- |
| `error_code` | String code, defaulting to `ERR_OK` |
| `task_code` | String code, defaulting to `TASK_CODE_INVALID` |
| `gpid` | Exact packed signed `i64`, with `app_id` and `partition_index` helpers |
| `rpc_address` | Host and port |
| `blob` | Binary string, `Uint8Array`, or `ArrayBuffer` input |

Thrift JSON binary values use unpadded base64 to match rDSN's C++ runtime. Padded base64 is
accepted while reading.

Nested `list`, `vector`, `set`, and `map` values are supported by the generic marshalling
helpers. JavaScript `Set` and `Map` values are accepted where available.

## Runtime provenance and synchronization

`thrift.js` is derived from Apache Thrift 0.9.3. `THRIFT_UPSTREAM.md` records the exact baseline,
the protocol patches retained by rDSN, and the update procedure.

`include/dsn/js` is authoritative for `thrift.js`, `dsn_transport.js`, and `dsn_types.js`. The
`sync_js_binding` build target copies those files into webstudio automatically. A standalone
update or consistency check can also be run with:

```sh
cmake -DDSN_ROOT="$PWD" -DMODE=UPDATE -P bin/sync_js_binding.cmake
cmake -DDSN_ROOT="$PWD" -DMODE=CHECK -P bin/sync_js_binding.cmake
```

## Scope

The binding can call existing rDSN RPC services. It does not expose application registration,
server-side RPC handlers, tasks, timers, AIO, replication callbacks, or component-provider
registration. Implementing native JavaScript rDSN applications would require a separate native
runtime binding.
