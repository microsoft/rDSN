<?php
require_once($argv[1]); // type.php
require_once($argv[2]); // program.php
$file_prefix = $argv[3];
$idl_type = $argv[4];
$idl_format = $argv[5];

$default_serialize_format = "DSF";
if ($idl_type == "thrift")
{
    $default_serialize_format = $default_serialize_format."_THRIFT";
} else
{
    $default_serialize_format = $default_serialize_format."_PROTOC";
}
$default_serialize_format = $default_serialize_format."_".strtoupper($idl_format);

?>
[modules]
dsn.tools.common
dsn.tools.hpc
dsn.tools.nfs
dsn.dist.uri.resolver
<?=$_PROG->name?> 

<?php 
foreach ($_PROG->services as $svc) 
{
    echo "[apps.client.perf.".$svc->name."]".PHP_EOL;
    echo "type = ".$_PROG->name.".".$svc->name.".client.perf".PHP_EOL;
    echo "arguments = dsn://mycluster/%target_service%".PHP_EOL;
    echo "pools = THREAD_POOL_DEFAULT".PHP_EOL;
    echo PHP_EOL;
    echo "[".$_PROG->name.".".$svc->name.".perf-test.case.1]".PHP_EOL;
    echo "perf_test_seconds  = 360000".PHP_EOL;
    echo "perf_test_key_space_size = 100000".PHP_EOL;
    echo "perf_test_concurrency = 1".PHP_EOL;
    echo "perf_test_payload_bytes = 128".PHP_EOL;
    echo "perf_test_timeouts_ms = 10000".PHP_EOL;
    echo "perf_test_hybrid_request_ratio = ";
    foreach ($svc->functions as $f) echo "1,";
    echo PHP_EOL;
    echo PHP_EOL;
} ?>

[uri-resolver.dsn://mycluster]
factory = partition_resolver_simple
arguments = %meta_server_list%

[core]
start_nfs = true

tool = nativerun
;toollets = tracer,profiler,fault_injector
toollets = %toollets%

logging_start_level = LOG_LEVEL_WARNING
logging_factory_name = dsn::tools::hpc_logger

[network]
; how many network threads for network library(used by asio)
io_service_worker_count = 2

; specification for each thread pool
[threadpool..default]
worker_count = 4
worker_priority = THREAD_xPRIORITY_LOWEST

[threadpool.THREAD_POOL_DEFAULT]
name = default
partitioned = false
max_input_queue_length = 1024
worker_priority = THREAD_xPRIORITY_LOWEST

[task..default]
is_trace = true
is_profile = true
allow_inline = false
rpc_call_channel = RPC_CHANNEL_TCP
rpc_message_header_format = dsn
rpc_timeout_milliseconds = 5000

disk_write_fail_ratio = 0.0
disk_read_fail_ratio = 0.00001

perf_test_seconds = 30
perf_test_payload_bytes = 1,128,1024

[task.LPC_AIO_IMMEDIATE_CALLBACK]
is_trace = false
allow_inline = false
disk_write_fail_ratio = 0.0

[task.LPC_RPC_TIMEOUT]
is_trace = false

[task.LPC_CHECKPOINT_REPLICA]
;execution_extra_delay_us_max = 10000000

[task.LPC_LEARN_REMOTE_DELTA_FILES]
;execution_extra_delay_us_max = 10000000

[task.RPC_FD_FAILURE_DETECTOR_PING]
is_trace = false
rpc_call_channel = RPC_CHANNEL_UDP

[task.RPC_FD_FAILURE_DETECTOR_PING_ACK]
is_trace = false
rpc_call_channel = RPC_CHANNEL_UDP

[task.LPC_BEACON_CHECK]
is_trace = false

[task.RPC_PREPARE]
