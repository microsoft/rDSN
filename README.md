[![Build Status](https://travis-ci.org/Microsoft/rDSN.svg?branch=master)](https://travis-ci.org/Microsoft/rDSN) [![Build status](https://ci.appveyor.com/api/projects/status/hkryxjf16uhyqie7?svg=true)](https://ci.appveyor.com/project/Microsoft/rdsn)

**Robust Distributed System Nucleus (rDSN)** is a framework for quickly building robust distributed systems. It has a microkernel for pluggable components, including applications, distributed frameworks, devops tools, and local runtime/resource providers, enabling their independent development and seamless integration. The project was originally developed for Microsoft Bing, and now has been adopted in production both inside and outside Microsoft.

* [What are the existing modules I can immediately use?](#existing)
* [What scenaios are enabled by combining these modules differently?](#scenarios)
* [How does rDSN build robustness?](#novel)
* [Related papers](#papers)

### Top Links
 * [[Case](https://github.com/imzhenyu/rocksdb)] RocksDB made replicated using rDSN!
 * [[Tutorial](https://github.com/Microsoft/rDSN/wiki/Tutorial:-one-box-cluster)] A one-box cluster demo to understand how rDSN helps service registration, deployment, monitoring etc..
 * [[Tutorial](https://github.com/Microsoft/rDSN/wiki/Tutorial:-Build-A-Single-Node-Counter-Service)] Build a counter service with built-in tools (e.g., codegen, auto-test, fault injection, bug replay, tracing)
 * [[Tutorial](https://github.com/Microsoft/rDSN/wiki/Tutorial:-Build-A-Scalable-and-Reliable-Counter-Service)] Build a scalable and reliable counter service with built-in replication support
 * [API Reference](http://imzhenyu.github.io/rDSN/documents/v1/html/index.html)
 * [Installation](https://github.com/Microsoft/rDSN/wiki/Installation)

 
### <a name="existing">Existing pluggable modules (and growing) </a>

The core of rDSN is a service kernel with which we can develop (via [Service API](http://imzhenyu.github.io/rDSN/documents/v1/html/group__service-api.html) and [Tool API](http://imzhenyu.github.io/rDSN/documents/v1/html/group__tool-api.html)) and plugin lots of different application, framework, tool, and local runtime modules, so that they can seamlessly benefit each other. Here is an incomplete list of the pluggable modules.

| Pluggable modules | Description | Release |
|--------|-------------|------|
| [dsn.core](https://github.com/Microsoft/rDSN/tree/master/src/core) | rDSN service kernel | [1.0.0](https://github.com/Microsoft/rDSN/releases/tag/v1.0.0) | 
| [dsn.dist.service.stateless](https://github.com/imzhenyu/rDSN.dist.service/tree/master/src/app_daemon)      | scale-out and fail-over for stateless services (e.g., micro services) | [1.0.0](https://github.com/imzhenyu/rDSN.dist.service/releases/tag/v1.0.0) |
| [dsn.dist.service.stateful.type1](https://github.com/imzhenyu/rDSN.dist.service/tree/master/src/replica_server) | scale-out, replicate, and fail-over for stateful services (e.g., storage) | [1.0.0](https://github.com/imzhenyu/rDSN.dist.service/releases/tag/v1.0.0) |
| [dsn.dist.service.meta_server](https://github.com/imzhenyu/rDSN.dist.service/tree/master/src/meta_server)    | membership, load balance, and machine pool management for the above service frameworks | [1.0.0](https://github.com/imzhenyu/rDSN.dist.service/releases/tag/v1.0.0) |
| [dsn.dist.uri.resolver](https://github.com/Microsoft/rDSN/tree/master/src/plugins/dist.uri.resolver)           | a client-side helper module that resolves service URL to target machine | [1.0.0](https://github.com/Microsoft/rDSN/releases/tag/v1.0.0) |
| [dsn.dist.traffic.router](https://github.com/imzhenyu/rDSN.dist.traffic.router)         | fine-grain RPC request routing/splitting/forking to multiple services (e.g., A/B test) | todo |
| [dsn.tools.common](https://github.com/Microsoft/rDSN/tree/master/src/plugins/tools.common)                | deployment runtime (e.g., network, aio, lock, timer, perf counters, loggers) for both Windows and Linux; simple toollets, such as tracer, profiler, and fault-injector | [1.0.0](https://github.com/Microsoft/rDSN/releases/tag/v1.0.0) |
| [dsn.tools.nfs](https://github.com/Microsoft/rDSN/tree/master/src/plugins/tools.nfs)                   | an implementation of remote file copy based on rpc and aio | [1.0.0](https://github.com/Microsoft/rDSN/releases/tag/v1.0.0) |
| [dsn.tools.emulator](https://github.com/Microsoft/rDSN/tree/master/src/plugins/tools.emulator)              | an emulation runtime for whole distributed system emulation with auto-test, replay, global state checking, etc. | [1.0.0](https://github.com/Microsoft/rDSN/releases/tag/v1.0.0) |
| [dsn.tools.hpc](https://github.com/imzhenyu/rDSN.tools.hpc)                   | high performance counterparts for the modules as implemented in tools.common | todo |
| [dsn.tools.explorer](https://github.com/imzhenyu/rDSN.tools.explorer)              | extracts task-level dependencies automatically | [1.0.0](https://github.com/imzhenyu/rDSN.tools.explorer/releases/tag/v1.0.0) |
| [dsn.tools.log.monitor](https://github.com/imzhenyu/rDSN.tools.log.monitor)           | collect critical logs (e.g., log-level >= WARNING) in cluster | [1.0.0](https://github.com/imzhenyu/rDSN.tools.log.monitor/releases/tag/v1.0.0) |
| [dsn.app.simple_kv](https://github.com/Microsoft/rDSN/tree/master/src/plugins/apps.skv)                    | an example application module | [1.0.0](https://github.com/Microsoft/rDSN/releases/tag/v1.0.0) | 

### <a name="scenarios"> Scenarios by different module combination and configuration </a>

rDSN provides flexible configuration so that developers can combine and configure the modules differently to enable different scenarios. All modules are loaded by [dsn.svchost](https://github.com/Microsoft/rDSN/tree/master/src/tools/svchost), a common process runner in rDSN, with the given configuration file. The following table lists some examples (note **dsn.core** is always required therefore omitted in ```Modules``` column).

| Scenarios | Modules  | Config | Demo | 
|----------|---------------|--------|------|
| logic correctness development | dsn.app.simple_kv + dsn.tools.emulator + dsn.tools.common | [config](https://github.com/Microsoft/rDSN/blob/master/tutorial/simple_kv/config.logic.ini)  | todo |
| logic correctness with failure | dsn.app.simple_kv + dsn.tools.emulator + dsn.tools.common | [config](https://github.com/Microsoft/rDSN/blob/master/tutorial/simple_kv/config.logic.failure.ini)  | todo |
| performance tuning | dsn.app.simple_kv + dsn.tools.common | [config](https://github.com/Microsoft/rDSN/blob/master/tutorial/simple_kv/config.logic.perf.ini)  | todo |
| progressive performance tuning | dsn.app.simple_kv + dsn.tools.common + dsn.tools.emulator | [config](https://github.com/Microsoft/rDSN/blob/master/tutorial/simple_kv/config.logic.perf.prog.ini)  | todo |
| Paxos enabled stateful service | dsn.app.simple_kv + dsn.tools.common + dsn.tools.emulator + dsn.dist.uri.resolver + dsn.dist.serivce.meta_server + dsn.dist.service.stateful.type1 | [config](https://github.com/Microsoft/rDSN/blob/master/tutorial/simple_kv/config.stateful.ini)  | todo |

There are a lot more possibilities. rDSN provides a [web portal](https://github.com/Microsoft/rDSN/tree/master/src/tools/webstudio) to enable quick deployment of these scenarios in a cluster, and allow easy operations through simple clicks as well as rich visualization. Deployment scenarios are defined [here](https://github.com/Microsoft/rDSN/blob/master/src/tools/webstudio/app_package/static/js/rdsn.envs.js), and developers can add more on demand. 

### <a name="novel"> How does rDSN build robustness? </a>

 * **reduced system complexity via microkernel architecture**: applications, frameworks (e.g., replication, scale-out, fail-over), local runtime libraries (e.g., network libraries, locks), and tools are all pluggable modules into a microkernel to enable independent development and seamless integration (therefore modules are reusable and transparently benefit each other)

 ![rDSN Architecture](doc/imgs/arch.png)

 * **flexible configuration with global deploy-time view**: tailor the module instances and their connections on demand with configurable system complexity and resource allocation (e.g., run all nodes in one simulator for testing, allocate CPU resources appropriately for avoiding resource contention, debug with progressively added system complexity)

 ![rDSN Configuration](doc/imgs/config.png)

 * **transparent tooling support**: dedicated tool API for tool development; built-in plugged tools for understanding, testing, debugging, and monitoring the upper applications and frameworks

 ![rDSN Architecture](doc/imgs/viz.png)

 * **auto-handled distributed system challenges**: built-in frameworks to achieve scalability, reliability, availability, and consistency etc. for the applications

 ![rDSN service model](doc/imgs/rdsn-layer2.jpg)

### <a name="papers"> Research papers </a>

rDSN borrows the idea in many research work, from both our own and the others, and tries to make them real in production in a coherent way; we greatly appreciate the researchers who did these work.

 * [Failure Recovery: When the Cure Is Worse Than the Disease](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/FailureRecoveryBeEvil.pdf), HotOS'13
 * [SEDA: An Architecture for Well-Conditioned, Scalable Internet Services](https://www.eecs.harvard.edu/~mdw/papers/seda-sosp01.pdf), SOSP'01
 * [PacificA: replication in log-based distributed storage systems](https://www.microsoft.com/en-us/research/wp-content/uploads/2008/02/tr-2008-25.pdf), MSR Tech Report
 * [Rex: Replication at the Speed of Multi-core](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/ppaxos.pdf), Eurosys'14
 * [Arming Cloud Services with Task Aspects](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/zion.techreport.pdf), MSR Tech Report
 * [D3S: Debugging Deployed Distributed Systems](https://www.microsoft.com/en-us/research/wp-content/uploads/2008/02/d3s_nsdi08.pdf), NSDI'08
 * [MoDIST: Transparent Model Checking of Unmodified Distributed Systems](http://www.cs.columbia.edu/~junfeng/papers/modist-nsdi09.pdf), NSDI'09
 * [G2: A Graph Processing System for Diagnosing Distributed Systems](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/G2-cr.pdf), USENIX ATC'11
 * [R2: An Application-Level Kernel for Record and Replay](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/r2-osdi08.pdf), OSDI'08
 * [WiDS: an Integrated Toolkit for Distributed System Development](https://www.microsoft.com/en-us/research/wp-content/uploads/2005/06/wids.pdf), HotOS'05

### License and Support

rDSN is provided on Windows and Linux, with the MIT open source license. You can use the "issues" tab in GitHub to report bugs.

