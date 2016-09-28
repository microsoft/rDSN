
var rdsn_envs = 
[
    {
        "title" : "stateless service",
        "dsptr" : "serve as a stateless service",
        "config" : "config.deploy.ini",
        "overwrites" : {
            "apps.server.run" : "true",
            "core.tool" : "nativerun",
            "core.toollets" : "profiler",            
            }
    },
    {
        "title" : "stateful service - meta server",
        "dsptr" : "start a meta server for managing the membership, load balancing etc. for a stateful service",
        "config" : "config.deploy.ini",
        "overwrites" : {
            "apps.meta.run" : "true",
            "core.tool" : "nativerun",
            "core.toollets" : "profiler,tracer",
            "replication.app.app_name" : "{{st}}",
            "replication.app.partition_count" : 4,
            "replication.app.max_replica_count" : 3,
            "meta_server.server_list" : "{{meta}}",
            "meta_server.min_live_node_count_for_unfreeze" : 1,
            }
    },
    {
        "title" : "stateful service - replica servers",
        "dsptr" : "start replica server(s) for hosting the service state and serving client requests",
        "config" : "config.deploy.ini",
        "overwrites" : {
            "apps.replica.run" : "true",
            "core.tool" : "nativerun",
            "core.toollets" : "profiler",
            "meta_server.server_list" : "{{meta}}",
            "uri-resolver.dsn://mycluster.arguments" : "{{meta}}",
        }
    },
    {
        "title" : "clients - functional test",
        "dsptr" : "basic functional test by invoking the service APIs as specified in service specification",
        "config" : "config.deploy.ini",
        "overwrites" : {
            "apps.client.run" : "true",
            "core.tool" : "nativerun",
            "core.toollets" : "",
            "apps.client.arguments" : "dsn://mycluster/<service-app-name>",
            "uri-resolver.dsn://mycluster.arguments" : "{{meta}}",
        },
    },
    {
        "title" : "clients - performance test",
        "dsptr" : "performance test by invoking the service APIs as specified in service specification",
        "config" : "config.deploy.ini",
        "overwrites" : {
            "apps.client.perf.run" : "true",
            "core.tool" : "nativerun",
            "core.toollets" : "",
            "apps.client.perf.arguments" : "dsn://mycluster/<service-app-name>",
            "uri-resolver.dsn://mycluster.arguments" : "{{meta}}",
            "{{st}}.{{st}}.perf-test.case.1.perf_test_key_space_size" : 1000000,
            "{{st}}.{{st}}.perf-test.case.1.perf_test_concurrency" : 1,
            "{{st}}.{{st}}.perf-test.case.1.perf_test_payload_bytes" : 256,
            "{{st}}.{{st}}.perf-test.case.1.perf_test_timeouts_ms" : 100,
            "{{st}}.{{st}}.perf-test.case.1.perf_test_hybrid_request_ratio" : "1,1,",
        },
    },
]