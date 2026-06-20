@ECHO OFF
@MKDIR perf-result
@RMDIR /Q /S data

REM %replica_count% - how many replica servers we want in this test
FOR %%R IN (1,2,3) DO (
    REM %tcp_network_provider% - what kind of tcp network providers we use
    FOR %%T IN (dsn::tools::sim_network_provider dsn::tools::asio_network_provider dsn::tools::hpc_network_provider) DO (
        REM %udp_network_provider% - what kind of udp network providers we use
        FOR %%U IN (dsn::tools::sim_network_provider dsn::tools::asio_udp_provider) DO (
            REM %aio_provider% - what kind of aio provider we use
            FOR %%A IN (dsn::tools::empty_aio_provider dsn::tools::native_aio_provider) DO (
                CALL dsn.app.simple_kv perf-config.ini -cargs replica_count=%%R;tcp_network_provider=%%T;udp_network_provider=%%U;aio_provider=%%A
                XCOPY /Y data\client.perf.test\perf-result-* .\perf-result\
                @RMDIR /Q /S data
            )
        )
    )
)

:exit


