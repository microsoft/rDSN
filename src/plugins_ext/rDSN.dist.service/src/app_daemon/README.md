
**dsn.dist.service.stateless** is part of the service framework for stateless applications such as micro services to acheive scalability and automatic failure recovery. This module is a daemon service on a single machine which receives commands from the [meta service](https://github.com/imzhenyu/rDSN.dist.service/tree/master/src/meta_server) and manages the life-time of service instances.

***ATTENTION: all commands below have counterparts on Windows using dsn.run.cmd***  

## How to use

This module is pre-deployed on a set of machine as a daemon service. An example is as follows:

```
git clone Microsoft/rDSN
git clone imzhenyu/rDSN.dist.service

pushd rDSN 
./run.sh build
./run.sh install
popd

pushd rDSN.dist.service
dsn.run.sh build
dsn.run.sh install
popd 

cd rDSN
dsn.run.sh onecluster
```

Now you can see that a 8-machine (each represetned by a daemon which runs ```dsn.dist.service.stateless```) cluster is deployed on you dev machine with one meta server and 8 daemon servers. You may use ```dsn.run.sh deploy|start|stop|cleanup``` to deploy them in a real cluster. The cluster also has a web portal (http://localhost:8088), and you can go to **Store** => **Register** => **Deploy** to deploy the target services, with the following preconditions - done automatically by the code generator in rDSN (i.e., using ```dsn.cg.sh```).

- application service is developed using rDSN and built as a dynamic linked library, e.g., ```counter.so```.

- a config.deploy.ini file is given together with the library, e.g.

```
@include %DSN_DEPLOYMENT_PATH%/config.common.ini

[config.args]
service_type = counter 

[modules]
counter  
``` 

```DSN_DEPLOYMENT_PATH``` is an environment variable that is set by the daemons when they are deployed.

- the library, the configuration file, as well as other necessary resource files (e.g., those required by ```libcounter.so```) are put into a directory named application type, e.g., ```counter```, and zipped into one tar ball, i.e., counter.tar.gz (or counter.zip on Windows). 

  - counter.tar.gz
    - counter (directory)
        - libcounter.so 
        - config.deploy.ini   

## Service scalability 

In order to scale out the target service, develoeprs can specify how many replicas to create upon deployment, and the service framework will automatically create the instances on a set of machines as hosted by the daemon service. The clients access the service using a service URL, i.e., ```dsn://mycluster/service-name```, with the following configuration in the configuration file to resolve the concrete target machine for service access.

```
[apps.client]
type = %service_type%.client 
arguments = dsn://mycluster/<target-service>
pools = THREAD_POOL_DEFAULT

[uri-resolver.dsn://mycluster]
factory = partition_resolver_simple
arguments = <meta-server1,meta-server2>
``` 

In addition, the framework also allows routing the requests to certain service instances. This is done by specifying: (i). partition number upon deployment, and (ii). request hash on client request submission. The requests are then dispatched by the framework using these numbers.   

## Service failure recovery and availability

When the machine is down or the service instance crashes, the framework will initiate a new instance to keep the service alive. The policy is implemented in meta service and can be customized. 


