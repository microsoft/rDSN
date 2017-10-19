
**dsn.dist.service.stateful.type1** is part of the service framework for stateful applications such as leveldb, rocksdb, redis, etc. It automatically turns them into distributed reliable storages through replication, failure recovery, reconfiguration, etc. This module is a hosting service on a single machine which receives commands from the [meta service](https://github.com/imzhenyu/rDSN.dist.service/tree/master/src/meta_server) and hosts the local components (e.g., storage) for state replication as well as serving.

***ATTENTION: all commands below have counterparts on Windows using dsn.run.cmd***  

## How to use

Developers follow the code generation process for rDSN applications (which is also required for adoption of the other service framework for stateless services). In addition, they need to implement the following methods for the target applications (e.g., a rocksdb storage component):

```
dsn_app_get_physical_error          get_physical_error;
dsn_app_sync_checkpoint             sync_checkpoint;
dsn_app_async_checkpoint            async_checkpoint;
dsn_app_get_last_checkpoint_decree  get_last_checkpoint_decree;
dsn_app_prepare_get_checkpoint      prepare_get_checkpoint;
dsn_app_get_checkpoint              get_checkpoint;
dsn_app_apply_checkpoint            apply_checkpoint;
``` 

You may check out ```dsn.apps.skv``` or our counter example in the tutorial (rDSN/tutorial) as a reference. The application is then further built into an rDSN application module, e.g., ```libcounter.so``` or ```counter.dll```. With this, the following configuration intiates a local replicated counter service, or you can also do it in the web portal by selecting the ```stateful service - replica servers``` etc. to apply a cluster deployment.

```
[modules]
dsn.tools.common
dsn.tools.nfs
dsn.dist.uri.resolver
dsn.dist.service.meta_server
dsn.dist.service.stateful.type1
counter 

[apps.client]
type = counter.client 
;arguments = localhost:34888
arguments = dsn://mycluster/counter.c1
pools = THREAD_POOL_DEFAULT

[task.RPC_COUNTER_COUNTER_ADD]
rpc_request_is_write_operation = true

[apps.replica]
type = replica
arguments = 
ports = 34801
pools = THREAD_POOL_DEFAULT,THREAD_POOL_REPLICATION,THREAD_POOL_FD,THREAD_POOL_LOCAL_APP,THREAD_POOL_REPLICATION_LONG
count = 3

[apps.meta]
type = meta
arguments = 
ports = 34601
pools = THREAD_POOL_DEFAULT,THREAD_POOL_META_SERVER,THREAD_POOL_FD,THREAD_POOL_META_STATE

[meta_server]
server_list = localhost:34601
min_live_node_count_for_unfreeze = 1

[uri-resolver.dsn://mycluster]
factory = partition_resolver_simple
arguments = localhost:34601

[replication.app]
app_name = counter.c1
app_type = counter 
partition_count = 1
max_replica_count = 3
stateful = true

[core]
start_nfs = true
tool = nativerun
toollets = tracer
```


## Service scalability 

Scalability is done by allowing partitioning upon deployment. The clients access the service using a service URL, i.e., ```dsn://mycluster/service-name```, with a request key specified for rDSN to automatically locate the correct partition for replication and serving. 

## Replication and failure recovery

The current replication protocol largely follows the one as described in [PacificA](https://www.microsoft.com/en-us/research/wp-content/uploads/2008/02/tr-2008-25.pdf). The improvement is about testing (e.g., model checking, global checking, distributed disclarative testing), performance optimization (e.g., shared replication log, checkpointing, state transfer), and programming model (conformed to rDSN model). 

## Meta server HA and reliabity 

The current protocol relies on the meta server for leader election therefore HA and reliability of the meta service is important. See details about meta server to check how we achieve HA and reliability for this service.

## TODO

Enable autonomous leader election to reduce availability lost during leader fail-over (shorter leader lease is acceptable in that case).  

