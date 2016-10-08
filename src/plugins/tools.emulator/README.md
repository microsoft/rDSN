
**dsn.tools.emulator** is an emulation platform for rDSN, with controlled envrionment to ease the testing, debugging, and reasoning of upper distributed systems. It controls all the non-deterministic behavior in a distributed system and allows tools to monitor as well as manipulate them for certain purposes, e.g., debugging without worrying about timeout, deterministic replay to reproduce the bugs, asserting the state across nodes (called global checking), and systematically try different scheduling & failure combinations to expose the possible bugs early (called model checking).

## How to use 

Start with the following configurations, note fault-injector can be optionally added since it is a toollet.

```
[modules]
dsn.tools.common 
dsn.tools.emulator

[core]
tool = emulator
toollets = 
;toollets = fault_injector
```

More configurations are required for different purposes during emulation. 

#### emulation with virtual time, network etc. 

The goal is to avoid unncessary timeout, network failure etc. during debugging. In this case, the above setting is enough.


#### deterministic execution

The goal is to deterministically reproduce an execution sequence for easier debugging. In this case, we need to set a non-zero random seed as follows which generates the pesudo random sequence for all the scheduling decisions in the system. The previous emulation setting above sets this seed to 0, which generates a random random-seed. Using the same seed will generate the same execution sequence.  

```
[tools.emulator]
random_seed = 12345
```

#### global checking 

Global checking is allowed as long as: (1) all the app and framework instances are set in a single rDSN process; (2) using the emulator tool. In this case, developers use the checker related APIs for global checking. More advanced tools, such as [declarative distributed testing](https://github.com/imzhenyu/rDSN.dist.service/tree/master/src/test/simple_kv), have been built atop of this facility. 


#### model checking

Model checkers systematically explore the scheduling and failure combinations in a distributed systems, so as to early expose the possible bugs that **may** happen in a real data center environment. To be published. 


## Call for more tools atop of emulator

Given all the non-determinisms controlled and manipulatable, it is possible to build more fancy and powerful tools ahead (e.g., the declarative distributed testing example above is done by Xiaomi). Feel free to have a try and contact us if necessary. 