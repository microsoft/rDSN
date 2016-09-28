<?php
require_once($argv[1]); // type.php
require_once($argv[2]); // program.php
$file_prefix = $argv[3];
$idl_type = $argv[4];
?>
// apps
# include "<?=$file_prefix?>.app.example.h"
# include <dsn/utility/module_init.cpp.h>
// if replicated, please implement and use
// # include "<?=$_PROG->name?>.server.impl.h"

void dsn_app_registration_<?=$_PROG->name?>()
{
    // register all possible service apps
    dsn::register_app< <?=$_PROG->get_cpp_namespace().$_PROG->name?>_server_app>("<?=$_PROG->name?>");
    // if replicated, using 
    // dsn::register_app_with_type_1_replication_support< <?=$_PROG->get_cpp_namespace().$_PROG->name?>_service_impl>("<?=$_PROG->name?>");
    
    dsn::register_app< <?=$_PROG->get_cpp_namespace().$_PROG->name?>_client_app>("<?=$_PROG->name?>.client");
    dsn::register_app< <?=$_PROG->get_cpp_namespace().$_PROG->name?>_perf_test_client_app>("<?=$_PROG->name?>.client.perf");
}

# if 1

MODULE_INIT_BEGIN(<?=$_PROG->name?>)
    dsn_app_registration_<?=$_PROG->name?>();
MODULE_INIT_END

# else

int main(int argc, char** argv)
{
    dsn_app_registration_<?=$_PROG->name?>();
    
    // specify what services and tools will run in config file, then run
    dsn_run(argc, argv, true);
    return 0;
}

# endif
