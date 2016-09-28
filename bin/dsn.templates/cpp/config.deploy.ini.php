<?php
require_once($argv[1]); // type.php
require_once($argv[2]); // program.php
$file_prefix = $argv[3];
$idl_type = $argv[4];
$idl_format = $argv[5];
?>

@include %DSN_DEPLOYMENT_PATH%/config.common.ini

[config.args]
service_type = <?=$_PROG->name?> 

[modules]
<?=$_PROG->name?>  

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
} ?>

<?php 
foreach ($_PROG->services as $svc) 
{
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
