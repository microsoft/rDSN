const assert = require("assert");
const fs = require("fs");
const path = require("path");
const { pathToFileURL } = require("url");
const vm = require("vm");

const root = path.resolve(process.argv[2]);
const packageRoot = path.join(root, "include/dsn/js");
const globalsBeforeImport = {
    Thrift: global.Thrift,
    DSN: global.DSN,
    DsnRpcError: global.DsnRpcError
};
const foreignThrift = { source: "foreign" };
const foreignDsn = { source: "foreign" };
global.Thrift = foreignThrift;
global.DSN = foreignDsn;
const runtime = require(packageRoot);
const nodeRuntime = require(path.join(packageRoot, "node.cjs"));
assert.notStrictEqual(runtime.Thrift, foreignThrift);
assert.notStrictEqual(runtime.DSN, foreignDsn);
assert.strictEqual(global.Thrift, foreignThrift);
assert.strictEqual(global.DSN, foreignDsn);
assert.strictEqual(global.DsnRpcError, globalsBeforeImport.DsnRpcError);
for (const name of Object.keys(globalsBeforeImport)) {
    if (globalsBeforeImport[name] === undefined) {
        delete global[name];
    } else {
        global[name] = globalsBeforeImport[name];
    }
}
const {
    Thrift,
    DSN,
    DsnRpcError,
    dsn_call,
    marshall_thrift_json,
    unmarshall_thrift_json
} = runtime;
const { error_code, task_code, gpid, blob, rpc_address } = runtime.types;
const generatedTypes = nodeRuntime.loadGeneratedTypes(
    path.join(root, "src/tools/webstudio/app_package/static/js/dsn/cli_types.js"),
    ["command"]);
const { command } = generatedTypes;
const browserContext = { DSN };
vm.createContext(browserContext);
new vm.Script(
    fs.readFileSync(
        path.join(
            root,
            "src/tools/webstudio/app_package/static/js/dsn/cli.client.js"),
        "utf8"),
    { filename: "cli.client.js" }).runInContext(browserContext);
const { cliApp } = browserContext;

for (const filename of ["thrift.js", "dsn_transport.js", "dsn_types.js"]) {
    assert.strictEqual(
        fs.readFileSync(path.join(packageRoot, filename), "utf8"),
        fs.readFileSync(
            path.join(
                root,
                "src/tools/webstudio/app_package/static/js/dsn",
                filename),
            "utf8"));
}

assert.strictEqual(
    Thrift.Int64.toSignedDecimalString("18446744073709551615", true),
    "-1");
assert.strictEqual(
    Thrift.Int64.toUnsignedDecimalString("-1"),
    "18446744073709551615");
assert.throws(
    () => Thrift.Int64.normalize(9007199254740992),
    error => error instanceof RangeError);

assert.strictEqual(
    marshall_thrift_json("18446744073709551615", "uint64_t"),
    '{"0":{"i64":-1}}');
assert.strictEqual(
    unmarshall_thrift_json('{"0":{"i64":-1}}', null, "uint64_t"),
    "18446744073709551615");
assert.strictEqual(
    marshall_thrift_json(gpid.from(1, 2), "struct"),
    '{"0":{"rec":{"1":{"i64":8589934593}}}}');
assert.strictEqual(
    marshall_thrift_json(new error_code(), "struct"),
    '{"0":{"rec":{"1":{"str":"ERR_OK"}}}}');
assert.strictEqual(
    marshall_thrift_json(new task_code(), "struct"),
    '{"0":{"rec":{"1":{"str":"TASK_CODE_INVALID"}}}}');
assert.strictEqual(
    marshall_thrift_json(new rpc_address(), "struct"),
    '{"0":{"rec":{"1":{"str":"invalid address"},"2":{"i32":0}}}}');
assert.strictEqual(
    marshall_thrift_json(new blob(new Uint8Array([0, 255])), "struct"),
    '{"0":{"rec":"AP8"}}');

const nested = {
    empty: [],
    values: [["9223372036854775807"], ["-9223372036854775808"]]
};
assert.deepStrictEqual(
    unmarshall_thrift_json(
        marshall_thrift_json(nested, "map<string,list<list<i64>>>"),
        null,
        "map<string,list<list<i64>>>"),
    nested);

function response(body, status, serverError) {
    return {
        ok: status >= 200 && status < 300,
        status,
        headers: {
            get(name) {
                return name.toLowerCase() === "server_error" ? serverError : null;
            }
        },
        text() {
            return Promise.resolve(body);
        }
    };
}

const request = new command({ cmd: "counter.list", arguments: [""] });

async function expectRpcError(promise, code) {
    try {
        await promise;
        assert.fail("Expected RPC call to reject");
    } catch (error) {
        assert(error instanceof DSN.RpcError);
        assert.strictEqual(error.name, "DsnRpcError");
        assert.strictEqual(error.code, code);
    }
}

async function main() {
    const esmRuntime = await import(pathToFileURL(
        path.join(packageRoot, "index.mjs")).href);
    assert.strictEqual(esmRuntime.DsnRpcError, DsnRpcError);
    assert.strictEqual(Thrift.UpstreamVersion, "0.9.3");

    let requestedUrl;
    let fetchOptions;
    const client = new cliApp("http://localhost:27001/base///", {
        credentials: "include",
        headers: {
            "X-Client": "default",
            "X-Remove": "remove-me"
        },
        fetch: function(url, options) {
            requestedUrl = url;
            fetchOptions = options;
            return Promise.resolve(response('{"0":{"str":"ok"}}', 200, "ERR_OK"));
        }
    });

    assert.strictEqual(
        await client.call({
            args: request,
            async: true,
            hash: 7,
            headers: {
                "x-client": "override",
                "X-Remove": null,
                "X-Call": "call"
            }
        }),
        "ok");
    assert.strictEqual(
        requestedUrl,
        "http://localhost:27001/base/DSF_THRIFT_JSON/7/RPC_CLI_CLI_CALL");
    assert.strictEqual(fetchOptions.credentials, "include");
    assert.strictEqual(fetchOptions.headers["X-Client"], "override");
    assert.strictEqual(fetchOptions.headers["X-Call"], "call");
    assert.strictEqual(fetchOptions.headers["X-Remove"], undefined);
    assert.strictEqual(await client.callAsync(request, 8), "ok");

    await expectRpcError(
        new cliApp("http://localhost:27001", {
            fetch: function() {
                return Promise.resolve(response("", 200, "ERR_TIMEOUT"));
            }
        }).callAsync(request),
        "ERR_TIMEOUT");

    let callbackError;
    const callbackResult = await new cliApp("http://localhost:27001", {
        fetch: function() {
            return Promise.resolve(response("", 200, "ERR_TIMEOUT"));
        }
    }).call({
        args: request,
        async: true,
        on_fail: function(xhr, textStatus, error) {
            callbackError = error;
        }
    });
    assert.strictEqual(callbackResult, null);
    assert(callbackError instanceof DSN.RpcError);
    assert.strictEqual(callbackError.code, "ERR_TIMEOUT");

    await expectRpcError(
        new cliApp("http://localhost:27001", {
            fetch: function() {
                return Promise.resolve(response("unavailable", 503, null));
            }
        }).callAsync(request),
        "HTTP_503");

    await expectRpcError(
        new cliApp("http://localhost:27001", {
            fetch: function() {
                    return Promise.reject(new Error("offline"));
            }
        }).callAsync(request),
        "TRANSPORT_ERROR");

    const abortingFetch = function(url, options) {
        return new Promise(function(resolve, reject) {
            options.signal.addEventListener("abort", function() {
                const error = new Error("aborted");
                error.name = "AbortError";
                reject(error);
            }, { once: true });
        });
    };
    await expectRpcError(
        new cliApp("http://localhost:27001", {
            timeoutMs: 1,
            fetch: abortingFetch
        }).callAsync(request),
        "CLIENT_TIMEOUT");

    const controller = new AbortController();
    const cancelled = new cliApp("http://localhost:27001", {
        fetch: abortingFetch
    }).callAsync(request, { signal: controller.signal });
    controller.abort();
    await expectRpcError(cancelled, "CANCELLED");

    for (const invalidUrl of [
        "",
        "localhost:27001",
        "ftp://localhost:27001",
        "http://user@example.test",
        "http://localhost:27001?",
        "http://localhost:27001#fragment"
    ]) {
        assert.throws(
            () => new cliApp(invalidUrl),
            error => error instanceof DsnRpcError && error.code === "INVALID_URL");
    }

    await assert.rejects(
        Promise.resolve().then(function() {
            return dsn_call(
                "",
                "RPC_TEST",
                0,
                "POST",
                "",
                "DSF_THRIFT_JSON",
                true);
        }),
        error => error instanceof DsnRpcError && error.code === "INVALID_URL");

    await assert.rejects(
        new cliApp("http://localhost:27001", {
            fetch: function() {
            return Promise.reject(new Error("offline"));
            }
        }).callAsync(request, { timeoutMs: -1 }),
        error => error instanceof RangeError);
}

main().then(function() {
    process.stdout.write("JavaScript binding compatibility checks passed\n");
}).catch(function(error) {
    console.error(error);
    process.exitCode = 1;
});
