#pragma once

#include <dsn/service_api_c.h>
#include <dsn/internal/task.h>

namespace dsn {

class service_node;

class zookeeper_provider {
public:
    template<typename T>
    static zookeeper_provider* create(service_node* node, zookeeper_provider* inner_provider){
        return new T(node, inner_provider);
    }
public:
    zookeeper_provider(service_node* node, zookeeper_provider* /*inner_provider*/)
    {
        _node = node;
    }
    service_node* node() const { return _node; }

    virtual dsn_handle_t connect(const char* zoo_hosts, int timeout_ms, dsn_task_t timeout_cb) = 0;
    virtual void disconnect(dsn_handle_t zoo_handle) = 0;
    virtual dsn_error_t async_visit(dsn_handle_t zoo_handle, dsn_task_t zoo_task) = 0;
    virtual zoo_visitor* prepare_zoo_visitor(zoo_task* zoo_tsk) = 0;
    virtual void destroy_zoo_visitor(zoo_visitor* visitor) = 0;
protected:
    service_node* _node;
};

}
