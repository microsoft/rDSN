(function(root) {
    "use strict";
    if (!root || !root.DSN || typeof root.DSN.create_client_class !== "function") {
        throw new Error("dsn_transport.js must be loaded before the generated rDSN client");
    }
    root.cliApp = root.DSN.create_client_class(
        "cli",
        [
            {
                "name": "call",
                "rpcCode": "RPC_CLI_CLI_CALL",
                "requestType": "command",
                "responseType": "string",
                "payloadFormat": "DSF_THRIFT_JSON",
                "oneWay": false
            }
        ]);
})(typeof globalThis !== "undefined" ? globalThis :
    typeof window !== "undefined" ? window : this);
