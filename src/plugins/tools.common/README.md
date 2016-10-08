
**tools.common** is a set of runtime and tool plugins that enable the basic running of a rDSN process; you may check out [here](https://github.com/Microsoft/rDSN/blob/master/src/plugins/tools.common/providers.common.cpp) to see how they are registered into the rDSN's service kernel.

- network provider (based on boost asio)
- network message (header format) parsers
  - rDSN native header
  - thrift (which enables service access with thrift generated client)
  - http (which enables service access using http clients such as a web browser)
- (disk) aio provider based on linux aio, posix aio, windows IOCP, and dummy (for testing) 
- task queue (a simple priority queue)
- locks (exclusive, exclusive + non-recursive, read-write + non-recursive)
- timer service (base on boost asio)
- native environment (random, time)
- loggers (native, screen)
- performance counters (number, percentile)
- commonly used toollets
  - tracer (tracing task flow across threads/machines)
  - profiler (tracing many performance aspects of the tasks)
  - fault injector (disk, network, task/thread scheduling, etc.)
- nativerun - a default tool configuration for native running rDSN procs


## How to use 

Users load the module in the configuration file, and set target tool/toollets as follows.

```
[modules]
dsn.tools.common 

[core]
tool = nativerun 
;toollets = 
toollets = tracer
;toollets = tracer, profiler, fault_injector
```

While ```tool = nativerun``` specifies the default providers for resource components in the system, they can also be overwrtten further.

```
[core]
logging_factory_name = dsn::tools::screen_logger
;logging_factory_name = dsn::tools::simple_logger ; this is the default in nativerun 
```

For many tools/toollets, there are many task-level configurations, e.g., 

```
[task..default]
is_trace = true
rpc_call_channel = RPC_CHANNEL_TCP

[task.RPC_FD_FAILURE_DETECTOR_PING]
is_trace = false
rpc_call_channel = RPC_CHANNEL_UDP

[task.RPC_FD_FAILURE_DETECTOR_PING_ACK]
is_trace = false
rpc_call_channel = RPC_CHANNEL_UDP
``` 

For more configurations, using the cli command:

```
[core]
cli_local = true 
```

Then you will be able to have a command line interface for each process and use the ```config-dump``` command to dump the current configurationa as well as comments into a file for further investigation.

```
>h
help|Help|h|H [command] - display help information
repeat|Repeat|r|R interval_seconds max_count command - execute command periodically
engine - get engine internal information
system.queue - get queue internal information
task-code - query task code containing any given keywords
counter.list - get the list of all counters
counter.value - get current value of a specific counter
counter.sample - get latest sample of a specific counter
counter.valuei - get current value of a specific counter
counter.samplei - get latest sample of a specific counter
counter.getindex - get index of a list of counters by name
tracer.find - find related logs
config-dump - dump configuration
daemon1.kill_partition kill_partition app_id partition_index

>config-dump current-cfg.txt 
```
