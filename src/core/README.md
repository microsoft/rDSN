
This directory contains the source code for rDSN service microkernel.

* src - source code, including the unit tests
* dev.cpp.core.use - reference to the dev/cpp module

***dsn.core*** is the service kernel in rDSN. It defines [Service API](http://imzhenyu.github.io/rDSN/documents/v1/html/group__service-api.html) and [Tool API](http://imzhenyu.github.io/rDSN/documents/v1/html/group__tool-api.html), with which users can develop and plugin various modules, including distributed frameworks, development & operation tools, local runtime, and applications (see [examples](https://github.com/Microsoft/rDSN#existing-pluggable-modules-and-growing-) here). dsn.core takes charge of interconnecting these components, and makes sure they can benefit each other transparently (while developed independently).

### Build tools and local runtime libraries

[Tool API](http://imzhenyu.github.io/rDSN/documents/v1/html/group__tool-api.html) is mainly for this purpose, with wich developers can plugin, for example, new network providers (e.g,. a RDMA network provider with better performance, a [virtual network](https://github.com/Microsoft/rDSN/blob/master/src/plugins/tools.emulator/network.sim.h) for emulation), toollets for capturing how the requests are processed in the system (e.g., [tracer](https://github.com/Microsoft/rDSN/blob/master/src/plugins/tools.common/tracer.h)), tools for driving the execution of the whole distributed system (e.g., [emulator](https://github.com/Microsoft/rDSN/tree/master/src/plugins/tools.emulator)). 

For all cases, developers use ```dsn::tools::register_component_provider```, ```dsn::tools::register_component_aspect```, ```dsn::tools::register_toollet```, ```dsn::tools::register_tool``` to plugin the modules into **dsn.core**.

### Build frameworks and applications

[Service API](http://imzhenyu.github.io/rDSN/documents/v1/html/group__service-api.html) provides the basic C APIs for building these components, and **dsn.dev.xxx** provides language wrappers atop (e.g., **dsn.dev.cpp**) to ease the development. The frameworks are considered advanced applications, and both are registered into **dsn.core** through ```dsn_register_app```. See [here](http://imzhenyu.github.io/rDSN/documents/v1/html/group__service-api-model.html) for more details.




