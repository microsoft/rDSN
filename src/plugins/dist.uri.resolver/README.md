
**dist.uri.resolver** is a client side plugin that translates a service URL (e.g., ```dsn://mycluster/my-first-service```) to the target machine (e.g., ```10.172.64.32:54332```) where a service (partition) is running. This particular implementation is working togegether with the meta service as implemented in [dsn.dist.service.meta_server](https://github.com/imzhenyu/rDSN.dist.service/tree/master/src/meta_server). The idea is to ask the meta service about the current membership of the service (partitions). When bad things happen (e.g., service access timeout or rejected by the target machine), the resolver will query the meta service again for the updated information. 

## How to use 

Application clients accesing the target service simply load this module, and change the target service address from an ``IP:PORT`` to a service URL. Therefore no client code is changed.

```
[modules]
dsn.dist.uri.resolver 

[apps.client]
type = %service_type%.client 
arguments = dsn://mycluster/<target-service>
pools = THREAD_POOL_DEFAULT
```

This particular module implementation requires the addresses of the meta services in order to query, configured as follows. For clusters that have virtual host or ip support, the current implementation can be simplified.   

```
[uri-resolver.dsn://mycluster]
factory = partition_resolver_simple
arguments = <meta-server1,meta-server2>
``` 




