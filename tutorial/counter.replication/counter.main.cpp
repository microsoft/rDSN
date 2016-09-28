// apps
# include "counter.app.example.h"
# include <dsn/utility/module_init.cpp.h>
// if replicated, please implement and use
# include "counter.server.impl.h"

void dsn_app_registration_counter()
{
    // register all possible service apps
    // dsn::register_app< ::dsn::example::counter_server_app>("counter");
    // if replicated, using 
    dsn::register_app_with_type_1_replication_support< ::dsn::example::counter_service_impl>("counter");
    
    dsn::register_app< ::dsn::example::counter_client_app>("counter.client");
    dsn::register_app< ::dsn::example::counter_perf_test_client_app>("counter.client.perf");
}

# if 1

MODULE_INIT_BEGIN(counter)
    dsn_app_registration_counter();
MODULE_INIT_END

# else

int main(int argc, char** argv)
{
    dsn_app_registration_counter();
    
    // specify what services and tools will run in config file, then run
    dsn_run(argc, argv, true);
    return 0;
}

# endif
