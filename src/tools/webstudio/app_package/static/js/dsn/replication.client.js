(function(root) {
    "use strict";
    if (!root || !root.DSN || typeof root.DSN.create_client_class !== "function") {
        throw new Error("dsn_transport.js must be loaded before the generated rDSN client");
    }
    root.meta_sApp = root.DSN.create_client_class(
        "meta_s",
        [
            {
                "name": "create_app",
                "rpcCode": "RPC_CM_CREATE_APP",
                "requestType": "configuration_create_app_request",
                "responseType": "configuration_create_app_response",
                "payloadFormat": "DSF_THRIFT_JSON",
                "oneWay": false
            },
            {
                "name": "drop_app",
                "rpcCode": "RPC_CM_DROP_APP",
                "requestType": "configuration_drop_app_request",
                "responseType": "configuration_drop_app_response",
                "payloadFormat": "DSF_THRIFT_JSON",
                "oneWay": false
            },
            {
                "name": "list_nodes",
                "rpcCode": "RPC_CM_LIST_NODES",
                "requestType": "configuration_list_nodes_request",
                "responseType": "configuration_list_nodes_response",
                "payloadFormat": "DSF_THRIFT_JSON",
                "oneWay": false
            },
            {
                "name": "list_apps",
                "rpcCode": "RPC_CM_LIST_APPS",
                "requestType": "configuration_list_apps_request",
                "responseType": "configuration_list_apps_response",
                "payloadFormat": "DSF_THRIFT_JSON",
                "oneWay": false
            },
            {
                "name": "query_configuration_by_node",
                "rpcCode": "RPC_CM_QUERY_NODE_PARTITIONS",
                "requestType": "configuration_query_by_node_request",
                "responseType": "configuration_query_by_node_response",
                "payloadFormat": "DSF_THRIFT_JSON",
                "oneWay": false
            },
            {
                "name": "query_configuration_by_index",
                "rpcCode": "RPC_CM_QUERY_PARTITION_CONFIG_BY_INDEX",
                "requestType": "configuration_query_by_index_request",
                "responseType": "configuration_query_by_index_response",
                "payloadFormat": "DSF_THRIFT_JSON",
                "oneWay": false
            }
        ]);
})(typeof globalThis !== "undefined" ? globalThis :
    typeof window !== "undefined" ? window : this);
