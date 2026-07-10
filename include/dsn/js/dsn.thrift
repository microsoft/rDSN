namespace cpp dsn

struct error_code
{
    1: string code;
}

struct task_code
{
    1: string code;
}

// dsn_types.js supplies the binary read/write implementation for this wire type.
struct blob
{
}

struct gpid
{
    1: i64 id;
}

struct rpc_address
{
    1: string host;
    2: i32 port;
}
