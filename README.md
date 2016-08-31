[![Build Status](https://travis-ci.org/Microsoft/rDSN.svg?branch=master)](https://travis-ci.org/Microsoft/rDSN) [![Build status](https://ci.appveyor.com/api/projects/status/hkryxjf16uhyqie7?svg=true)](https://ci.appveyor.com/project/Microsoft/rdsn)

**Robust Distributed System Nucleus (rDSN)** is a framework for quickly building robust distributed systems. It has a microkernel for pluggable components, including applications, distributed frameworks, devops tools, and local runtime/resource providers, enabling their independent development and seamless integration. The project was originally developed for Microsoft Bing, and now has been adopted in production both inside and outside Microsoft. 

* [How does rDSN build robustness?](#novel)
* [What I can do with rDSN?](#cando)
* [What are the existing modules I can immediately use?](#existing)
* [Related papers](#papers)


### Top Links
 * [[Case](https://github.com/imzhenyu/rocksdb)] RocksDB made replicated using rDSN!
 * [[Tutorial](https://github.com/Microsoft/rDSN/wiki/Tutorial:-Build-A-Single-Node-Counter-Service)] Build a counter service with built-in tools (e.g., codegen, auto-test, fault injection, bug replay, tracing)
 * [[Tutorial](https://github.com/Microsoft/rDSN/wiki/Tutorial:-Build-A-Scalable-and-Reliable-Counter-Service)] Build a scalable and reliable counter service with built-in replication support
 * [[Tutorial](https://github.com/Microsoft/rDSN/wiki/Tutorial:-Perfect-Failure-Detector)] Build a perfect failure detector with progressively added system complexity
 * [[Tutorial](https://github.com/Microsoft/rDSN/wiki/Tutorial:-Plugin-A-New-Network-Implementation)] Plugin my own network implementation for higher performance
 * [Latest documentation](http://imzhenyu.github.io/rDSN/documents/v1/html/index.html)
 * [Installation](https://github.com/Microsoft/rDSN/wiki/Installation)

### <a name="cando"> What I can do with rDSN? </a>

 * turn legacy local components (e.g., a local storage, or a micro service) into highly available and reliable service with [service frameworks](https://github.com/imzhenyu/rDSN.dist.service)
 * develop services with an event-driven service lib similar to libevent, Thrift, and GRPC, with additional:
    * a rich set of [service API](http://imzhenyu.github.io/rDSN/documents/v1/html/group__service-api.html) in addition to RPC 
    * built-in [tools](#tools) for testing, debuging, monitoring, and operation
    * built-in [service frameworks](#frameworks) for scaliability, availabity, and reliability
 * build new frameworks with strong tooling support, benefiting the service applications immediately 
 * build new tools with dedicated [Tool API](http://imzhenyu.github.io/rDSN/documents/v1/html/group__tool-api.html), benefiting the services and frameworks transparently
 * more as you can imagine.

### <a name="novel"> How does rDSN build robustness? </a> 

 * **reduced system complexity via microkernel architecture**: applications, frameworks (e.g., replication, scale-out, fail-over), local runtime libraries (e.g., network libraries, locks), and tools are all pluggable modules into a microkernel to enable independent development and seamless integration (therefore modules are reusable and transparently benefit each other) 
 
 ![rDSN Architecture](doc/imgs/arch.png)
 
 * **flexible configuration with global deploy-time view**: tailor the module instances and their connections on demand with configurable system complexity and resource allocation (e.g., run all nodes in one simulator for testing, allocate CPU resources appropriately for avoiding resource contention, debug with progressively added system complexity) 
 
 ![rDSN Configuration](doc/imgs/config.png)
 
 * **transparent tooling support**: dedicated tool API for tool development; built-in plugged tools for understanding, testing, debugging, and monitoring the upper applications and frameworks 

 ![rDSN Architecture](doc/imgs/viz.png)
 
 * **auto-handled distributed system challenges**: built-in frameworks to achieve scalability, reliability, availability, and consistency etc. for the applications
 
 ![rDSN service model](doc/imgs/rdsn-layer2.jpg) 
 
### <a name="existing">Existing pluggable modules (and growing) </a>

##### <a name="frameworks"> Distributed frameworks </a>

 * [dist.service.stateful.type1](https://github.com/imzhenyu/rDSN.dist.service): a production Paxos framework to quickly turn a local component (e.g., rocksdb) into an online service with replication, partition, failure recovery, and reconfiguration supports
 * [dist.service.stateless](https://github.com/imzhenyu/rDSN.dist.service): a scale-out and fail-over framework for stateless services such as Memcached

##### <a name="locallibs"> Local runtime libraries </a> 

 * [tools.common](https://github.com/imzhenyu/rDSN/tree/master/src/plugins/tools.common)
   * network libraries on Linux/Windows supporting rDSN/Thrift/HTTP messages at the same time
   * asynchronous disk IO on Linux/Windows
   * locks, rwlocks, semaphores
   * task queues 
   * timer services
   * performance counters
   * loggers (screen, simple)
 * [tools.hpc](https://github.com/imzhenyu/rDSN.tools.hpc): high performance counterparts for the above modules

##### <a name="tools"> Devops tools </a>

 * [tools.common](https://github.com/imzhenyu/rDSN/tree/master/src/plugins/tools.common)
   * simulator debugs multiple nodes in one single process without worry about timeout
   * tracer dumps logs for how requests are processed across tasks/nodes
   * profiler shows detailed task-level performance data (e.g., queue-time, exec-time)
   * fault-injector mimics data center failures to expose bugs early
   * global-checker enables cross-node assertion 
   * replayer reproduces the bugs for easier root cause analysis
 * [tools.explorer](https://github.com/imzhenyu/rDSN.tools.explorer): extracts task-level dependencies automatically 
 * [Web studio](https://github.com/imzhenyu/rDSN/tree/master/src/tools/webstudio) to visualize task-level performance and dependency information [live](http://imzhenyu.github.io/rDSN/webstudio/setting.html) [Demo](https://www.youtube.com/watch?v=FKNNg3Yzu6o) 

##### Other distributed providers, libraries, and tools

 * [tools.nfs](https://github.com/imzhenyu/rDSN/tree/master/src/plugins/tools.nfs): remote file copy 
 * [dist.service.fd](https://github.com/imzhenyu/rDSN.dist.service/tree/master/src/fd): perfect failure detector
 * [dist.service.fd.multimaster](https://github.com/imzhenyu/rDSN.dist.service/tree/master/src/fd): multi-master perfect failure detector
 * [dist.deployment](https://github.com/imzhenyu/rDSN.dist.deployment): a deployment service for Windows/Linux/Kubernets  

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

