#include "partition_resolver_simple.h"

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

namespace dsn {
namespace dist {

class partition_resolver_simple_test : public ::testing::Test
{
protected:
    static void publish(partition_resolver_simple &resolver,
                        int app_id,
                        const rpc_address &primary)
    {
        std::unique_ptr<partition_resolver_simple::partition_info> info(
            new partition_resolver_simple::partition_info());
        info->timeout_count = 0;
        info->config.pid = gpid(app_id, 0);
        info->config.primary = primary;

        service::zauto_write_lock lock(resolver._config_lock);
        resolver._app_id = app_id;
        resolver._app_partition_count = 1;
        resolver._app_is_stateful = true;
        resolver._config_cache[0] = std::move(info);
    }
};

TEST_F(partition_resolver_simple_test, concurrent_snapshot_is_coherent)
{
    rpc_address meta_server;
    meta_server.assign_group(dsn_group_build("partition_resolver_simple.test"));
    partition_resolver_simple resolver(meta_server, "partition_resolver_simple.test");
    const rpc_address primary_a("127.0.0.1", 21001);
    const rpc_address primary_b("127.0.0.1", 21002);
    publish(resolver, 1001, primary_a);

    constexpr int update_count = 20000;
    constexpr int reader_count = 4;
    std::atomic<bool> start(false);
    std::atomic<bool> failed(false);
    std::thread publisher([&]() {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        for (int i = 0; i < update_count; ++i) {
            publish(resolver, (i & 1) == 0 ? 1001 : 1002, (i & 1) == 0 ? primary_a : primary_b);
        }
    });

    std::vector<std::thread> readers;
    for (int reader_index = 0; reader_index < reader_count; ++reader_index) {
        readers.emplace_back([&]() {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            for (int i = 0; i < update_count; ++i) {
                bool callback_called = false;
                resolver.resolve(
                    0,
                    [&](partition_resolver::resolve_result &&result) {
                        callback_called = true;
                        const int app_id = result.pid.u.app_id;
                        const uint16_t port = result.address.port();
                        if (result.err != ERR_OK ||
                            !((app_id == 1001 && port == 21001) ||
                              (app_id == 1002 && port == 21002))) {
                            failed.store(true, std::memory_order_relaxed);
                        }
                    },
                    1000);
                if (!callback_called) {
                    failed.store(true, std::memory_order_relaxed);
                    return;
                }
            }
        });
    }

    start.store(true, std::memory_order_release);
    publisher.join();
    for (auto &reader : readers) {
        reader.join();
    }

    EXPECT_FALSE(failed.load(std::memory_order_relaxed));
}

} // namespace dist
} // namespace dsn
