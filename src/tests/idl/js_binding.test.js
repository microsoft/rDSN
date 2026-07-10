const assert = require("assert");
const fs = require("fs");
const path = require("path");
const vm = require("vm");

const root = path.resolve(process.argv[2]);

function load(relativePath) {
    const filename = path.join(root, relativePath);
    vm.runInThisContext(fs.readFileSync(filename, "utf8"), { filename });
}

load("include/dsn/js/thrift.js");
load("include/dsn/js/dsn_transport.js");
load("include/dsn/js/dsn_types.js");
load("src/tools/webstudio/app_package/static/js/dsn/cli.client.js");

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

const command = {
    write(output) {
        output.writeStructBegin("command");
        output.writeFieldStop();
        output.writeStructEnd();
    }
};

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
    const originalFetch = global.fetch;
    try {
        let requestedUrl;
        global.fetch = function(url) {
            requestedUrl = url;
            return Promise.resolve(response('{"0":{"str":"ok"}}', 200, "ERR_OK"));
        };

        const client = new cliApp("http://localhost:27001/");
        assert.strictEqual(
            await client.call({ args: command, async: true, hash: 7 }),
            "ok");
        assert.strictEqual(
            requestedUrl,
            "http://localhost:27001/DSF_THRIFT_JSON/7/RPC_CLI_CLI_CALL");
        assert.strictEqual(await client.callAsync(command, 8), "ok");

        global.fetch = function() {
            return Promise.resolve(response("", 200, "ERR_TIMEOUT"));
        };
        await expectRpcError(client.callAsync(command), "ERR_TIMEOUT");

        let callbackError;
        const callbackResult = await client.call({
            args: command,
            async: true,
            on_fail: function(xhr, textStatus, error) {
                callbackError = error;
            }
        });
        assert.strictEqual(callbackResult, null);
        assert(callbackError instanceof DSN.RpcError);
        assert.strictEqual(callbackError.code, "ERR_TIMEOUT");

        global.fetch = function() {
            return Promise.resolve(response("unavailable", 503, null));
        };
        await expectRpcError(client.callAsync(command), "HTTP_503");

        global.fetch = function() {
            return Promise.reject(new Error("offline"));
        };
        await expectRpcError(client.callAsync(command), "TRANSPORT_ERROR");

        await assert.rejects(
            Promise.resolve().then(function() {
                return dsn_call(
                    "", "RPC_TEST", 0, "POST", "", "DSF_THRIFT_JSON", true);
            }),
            error => error instanceof TypeError);
    } finally {
        global.fetch = originalFetch;
    }
}

main().then(function() {
    process.stdout.write("JavaScript binding compatibility checks passed\n");
}).catch(function(error) {
    console.error(error);
    process.exitCode = 1;
});
