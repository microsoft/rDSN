# pragma once
# include <dsn/service_api_cpp.h>
# include <dsn/cpp/serialization.h>


# include <dsn/cpp/serialization_helper/dsn.layer2_types.h>

# include <dsn/utility/enum_helper.h>

namespace dsn { 
    GENERATED_TYPE_SERIALIZATION(partition_configuration, THRIFT)
    GENERATED_TYPE_SERIALIZATION(configuration_query_by_index_request, THRIFT)
    GENERATED_TYPE_SERIALIZATION(configuration_query_by_index_response, THRIFT)
    GENERATED_TYPE_SERIALIZATION(app_info, THRIFT)

        
    ENUM_BEGIN2(app_status::type, app_status, app_status::AS_INVALID)
        ENUM_REG(app_status::AS_AVAILABLE)
        ENUM_REG(app_status::AS_CREATING)
        ENUM_REG(app_status::AS_CREATE_FAILED)
        ENUM_REG(app_status::AS_DROPPING)
        ENUM_REG(app_status::AS_DROP_FAILED)
        ENUM_REG(app_status::AS_DROPPED)
    ENUM_END2(app_status::type, app_status)
}
