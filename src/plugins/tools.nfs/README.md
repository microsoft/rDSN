
**tools.nfs** is a simple implementation for a network file system (remote file read only) based on RPC and disk AIO, which is used a common utility to move files across machines. Developers can implement a new provider with much higher performance (e.g., using ```sendfile``` on Linux and ```TransmitFile``` on Windows).

## How to use 

Users load the module in the configuration file, and set target toggle as follows.

```
[modules]
dsn.tools.nfs

[core]
start_nfs = true
```

The following service API is provided for programming in C.

```
dsn_file_copy_remote_directory
dsn_file_copy_remote_files
```

And C++. Check out the API reference for details.
```
dsn::file::copy_remote_file
dsn::file::copy_remote_directory
```

## Build new nfs providers

For the current tools, including ```nativerun```, ```emulator```, and ```fastrun```, the default provider for nfs is exactly this module, so no further configurations are required. If you want to plugin your own, you may try the following.

Register the new module into rDSN.
```
MODULE_INIT_BEGIN(nfs_node_simple) 
     dsn::tools::register_component_provider< ::dsn::service::nfs_node_simple>("my-new-nfs-provider"); 
MODULE_INIT_END 
```

Use it at runtime in configuration.

```
[core]
nfs_factory_name = my-new-nfs-provider
```
