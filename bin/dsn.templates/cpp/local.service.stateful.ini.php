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
dsn.tools.emulator
dsn.tools.nfs
dsn.dist.uri.resolver
dsn.dist.service.meta_server
dsn.dist.service.stateful.type1
<?=$_PROG->name?> 

[apps.client]
type = <?=$_PROG->name?>.client 
;arguments = localhost:34888
arguments = dsn://mycluster/<?=$_PROG->name?>.c1
pools = THREAD_POOL_DEFAULT

[apps.client.perf]
type = <?=$_PROG->name?>.client.perf 
arguments = localhost:34888
;arguments = dsn://mycluster/<?=$_PROG->name?>.c1
pools = THREAD_POOL_DEFAULT
run = false

<?php 
foreach ($_PROG->services as $svc) 
{
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
    foreach ($svc->functions as $f) { 
        if ($f->is_write)
        {   
            echo "[task.". $f->get_rpc_code(). "]".PHP_EOL;
            echo "rpc_request_is_write_operation = true".PHP_EOL;
            echo PHP_EOL;
        }
    }
} ?>

[apps.replica]
type = replica
arguments = 
ports = 34801
pools = THREAD_POOL_DEFAULT,THREAD_POOL_REPLICATION,THREAD_POOL_FD,THREAD_POOL_LOCAL_APP,THREAD_POOL_REPLICATION_LONG
count = 3

[apps.meta]
type = meta
arguments = 
ports = 34601
pools = THREAD_POOL_DEFAULT,THREAD_POOL_META_SERVER,THREAD_POOL_FD,THREAD_POOL_META_STATE

[meta_server]
server_list = localhost:34601
min_live_node_count_for_unfreeze = 1

[uri-resolver.dsn://mycluster]
factory = partition_resolver_simple
arguments = localhost:34601

[replication.app]
app_name = <?=$_PROG->name?>.c1
app_type = <?=$_PROG->name?> 
partition_count = 1
max_replica_count = 3
stateful = true

[core]
start_nfs = true

tool = nativerun
;tool = emulator
toollets = tracer
;toollets = tracer,profiler,fault_injector
pause_on_start = false

;logging_start_level = LOG_LEVEL_WARNING
;logging_factory_name = dsn::tools::screen_logger
;logging_factory_name = dsn::tools::simple_logger
;logging_factory_name = dsn::tools::hpc_tail_logger
;logging_factory_name = dsn::tools::hpc_logger
;aio_factory_name = dsn::tools::empty_aio_provider

[network]
; how many network threads for network library(used by asio)
io_service_worker_count = 2

; specification for each thread pool
[threadpool..default]
worker_count = 2


[threadpool.THREAD_POOL_DEFAULT]
name = default
partitioned = false
max_input_queue_length = 1024


[threadpool.THREAD_POOL_REPLICATION]
name = replication
partitioned = true
max_input_queue_length = 2560


[threadpool.THREAD_POOL_META_STATE]
worker_count = 1

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
rpc_request_resend_timeout_milliseconds = 8000

[replication]

prepare_timeout_ms_for_secondaries = 10000
prepare_timeout_ms_for_potential_secondaries = 20000

learn_timeout_ms = 30000
staleness_for_commit = 20
staleness_for_start_prepare_for_potential_secondary = 110
mutation_max_size_mb = 15
mutation_max_pending_time_ms = 20
mutation_2pc_min_replica_count = 2

prepare_list_max_size_mb = 250
request_batch_disabled = false
group_check_internal_ms = 100000
group_check_disabled = false
fd_disabled = false
fd_check_interval_seconds = 5
fd_beacon_interval_seconds = 3
fd_lease_seconds = 14
fd_grace_seconds = 15
working_dir = .
log_buffer_size_mb = 1
log_pending_max_ms = 100
log_file_size_mb = 32
log_batch_write = true

log_enable_shared_prepare = true
log_enable_private_commit = false

config_sync_interval_ms = 60000
