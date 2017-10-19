[![Build Status](https://travis-ci.org/imzhenyu/rDSN.dist.service.svg?branch=master)](https://travis-ci.org/imzhenyu/rDSN.dist.service)

This repo contains the following framework plugins for rDSN, and you may go through [tutorial](https://github.com/Microsoft/rDSN/wiki/Tutorial:-one-box-cluster) to have a try.

| Pluggable modules | Description | 
|--------|-------------|
| [dsn.dist.service.stateless](https://github.com/imzhenyu/rDSN.dist.service/tree/master/src/app_daemon)      | scale-out and fail-over for stateless services (e.g., HA micro-services) |
| [dsn.dist.service.stateful.type1](https://github.com/imzhenyu/rDSN.dist.service/tree/master/src/replica_server) | scale-out, replicate, and fail-over for stateful services (e.g., Spanner) | 
| [dsn.dist.service.meta_server](https://github.com/imzhenyu/rDSN.dist.service/tree/master/src/meta_server)    | membership, load balance, and machine pool management for the above dist.service.xxx modules | 


##### Build

You firstly need to have installed [rDSN](https://github.com/Microsoft/rDSN).

```
git clone https://github.com/imzhenyu/rDSN.dist.service.git xyz
cd xyz
dsn.run.sh build
dsn.run.sh install 
dsn.run.sh test
```

##### License and Support

rDSN is provided on Windows and Linux, with the MIT open source license. You can use the "issues" tab in GitHub to report bugs.

