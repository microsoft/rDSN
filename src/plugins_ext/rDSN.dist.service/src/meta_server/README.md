
*** dsn.dist.service.meta_server ** is a shared components of the service frameworks for both stateless and stateful services. It focuses on three aspects:

- membership management: what are the services/service partitions, what are the target machines for each partition, and what are their roles (e.g., worker, secondary, primary)

- machine pool management and failure detection: what are the machines to be deployed with the services, and whether they are healthy

- load balance: move the partition instances across machines so that the load is balanced across machines

## How to use

Using the ```dsn.svchost``` to create an meta service instance after loading ```dsn.dist.service.meta_service```, then it is done.  

```
[modules]
dsn.dist.service.meta_server

[apps.meta]
type = meta
arguments = 
ports = %port%
pools = THREAD_POOL_DEFAULT,THREAD_POOL_META_SERVER,THREAD_POOL_FD,THREAD_POOL_META_STATE
```

Daemons or replica servers need to specify the meta server address in their configuration file to join the machine pool.

```
[meta_server]
server_list = <meta-server1,meta-server2>
```

Clients accessing the services also need to specify the meta server addresses, see [dsn.dist.uri.resolver](https://github.com/Microsoft/rDSN/tree/master/src/plugins/dist.uri.resolver) for more information.

## Service availability and reliability for meta service itself

We now use zookeeper to store the membership state as well as leader election from a set of meta servers to ensure service availability and reliability. Upon development, we use a stand-alone meta server which simply uses local disk file for state reliability. Two provider interfaces, ```distributed_lock_service``` and ```meta_state_service```, are abstracted. Developers can therefore build their own for better quality in the future. 

## Customize the load blance policy   

You can implement your own ```server_load_balancer``` instance, and plug them into the meta server.

```
register_component_provider("simple_load_balancer", replication::server_load_balancer::create<replication::simple_load_balancer>);
register_component_provider("greedy_load_balancer", replication::server_load_balancer::create<replication::greedy_load_balancer>);
```

Then specify them upon deployment.

```
[meta_server]
server_load_balancer_type = <cutomized server load balancer type>
```
