/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Microsoft Corporation
 * 
 * -=- Robust Distributed System Nucleus (rDSN) -=- 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * Description:
 *     What is this file about?
 *
 * Revision history:
 *     xxxx-xx-xx, author, first version
 *     xxxx-xx-xx, author, fix bug about xxx
 */


# if defined(_WIN32)

# define _WINSOCK_DEPRECATED_NO_WARNINGS 1

# include <winsock2.h>
# include <ws2tcpip.h>
# include <windows.h>

# else
# include <sys/socket.h>
# include <netdb.h>
# include <ifaddrs.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <sys/ioctl.h>
# include <net/if.h>
# include <unistd.h>

# if defined(__FreeBSD__)
# include <netinet/in.h>
# endif

# endif

# include <dsn/utility/ports.h>
# include <dsn/service_api_c.h>
# include <dsn/cpp/address.h>
# include <dsn/tool-api/task.h>
# include "group_address.h"
# include "uri_address.h"

namespace dsn
{
    const rpc_address rpc_group_address::_invalid;
}

# if defined(_WIN32)
static void net_init()
{
    static std::once_flag flag;
    static bool flag_inited = false;
    if (!flag_inited)
    {
        std::call_once(flag, [&]()
        {
            WSADATA wsaData;
            WSAStartup(MAKEWORD(2, 2), &wsaData);
            flag_inited = true;
        });
    }
}
# endif

// name to ip etc.
DSN_API uint32_t dsn_ipv4_from_host(const char* name)
{
    if (name == nullptr || name[0] == '\0')
    {
        derror("dsn_ipv4_from_host got null or empty name");
        return 0;
    }

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    if ((addr.sin_addr.s_addr = inet_addr(name)) == (unsigned int)(-1))
    {
        hostent* hp = ::gethostbyname(name);
        int err =
# if defined(_WIN32)
            (int)::WSAGetLastError()
# else
            h_errno
# endif
            ;

        if (hp == nullptr)
        {
            derror("gethostbyname failed, name = %s, err = %d.", name, err);
            return 0;
        }
        else
        {
            // gethostbyname() should only return IPv4 addresses (4 bytes) here; guard against an
            // unexpected address family/length so a longer h_addr can not overflow s_addr (4 bytes).
            if (hp->h_addrtype != AF_INET || hp->h_length != static_cast<int>(sizeof(addr.sin_addr.s_addr)))
            {
                derror("gethostbyname returned unexpected address for name = %s (addrtype = %d, length = %d).",
                    name, hp->h_addrtype, hp->h_length);
                return 0;
            }

            memcpy(
                (void*)&(addr.sin_addr.s_addr),
                (const void*)hp->h_addr,
                (size_t)hp->h_length
                );
        }
    }

    // converts from network byte order to host byte order
    return (uint32_t)ntohl(addr.sin_addr.s_addr);
}

static bool interface_has_prefix(const char* network_interface, const char* prefix)
{
    return strncmp(network_interface, prefix, strlen(prefix)) == 0;
}

# if !defined(_WIN32)
static bool is_default_interface(const struct ifaddrs* ifa)
{
    if ((ifa->ifa_flags & IFF_UP) == 0 ||
        (ifa->ifa_flags & IFF_LOOPBACK) != 0 ||
        ifa->ifa_addr->sa_family != AF_INET)
    {
        return false;
    }

# if defined(__APPLE__)
    return interface_has_prefix(ifa->ifa_name, "en");
# elif defined(__FreeBSD__)
    return interface_has_prefix(ifa->ifa_name, "em") ||
           interface_has_prefix(ifa->ifa_name, "igb") ||
           interface_has_prefix(ifa->ifa_name, "ix") ||
           interface_has_prefix(ifa->ifa_name, "re") ||
           interface_has_prefix(ifa->ifa_name, "bge") ||
           interface_has_prefix(ifa->ifa_name, "vtnet") ||
           interface_has_prefix(ifa->ifa_name, "vmx") ||
           interface_has_prefix(ifa->ifa_name, "iwn") ||
           interface_has_prefix(ifa->ifa_name, "iwm") ||
           interface_has_prefix(ifa->ifa_name, "urtwn");
# else
    return interface_has_prefix(ifa->ifa_name, "eth") ||
           interface_has_prefix(ifa->ifa_name, "en") ||
           interface_has_prefix(ifa->ifa_name, "wl") ||
           interface_has_prefix(ifa->ifa_name, "ww") ||
           interface_has_prefix(ifa->ifa_name, "ib") ||
           interface_has_prefix(ifa->ifa_name, "sl");
# endif
}
# endif

// If network_interface is empty, this returns the first hinted non-loopback IPv4 address.
// The original Linux-centric "eth" prefix misses modern Linux names and common
// macOS and FreeBSD interface names, such as "en0", "em0", "vtnet0", "igb0", and "re0";
// callers may then fall back to hostname resolution, which can choose
// an address that is inconsistent with localhost-based tests.
DSN_API uint32_t dsn_ipv4_local(const char* network_interface)
{
    uint32_t ret = 0;

# if !defined(_WIN32)
    const bool use_default_interface = network_interface == nullptr || network_interface[0] == '\0';
    static const char loopback[4] = { 127, 0, 0, 1 };
    struct ifaddrs* ifa = nullptr;
    if (getifaddrs(&ifa) == 0)
    {
        struct ifaddrs* i = ifa;
        while (i != nullptr)
        {
            if (i->ifa_name != nullptr &&
                i->ifa_addr != nullptr 
                )
            {
                if ((!use_default_interface && strcmp(i->ifa_name, network_interface) == 0) ||
                    (use_default_interface && is_default_interface(i)))
                {
                    // Existing loopback detection checks IPv4 address bytes; IFF_LOOPBACK
                    // would be a better signal for interface-level loopback status.
                    if (i->ifa_addr->sa_family == AF_INET && 
                        (!use_default_interface || strncmp((const char*)&((struct sockaddr_in *)i->ifa_addr)->sin_addr.s_addr, loopback, 4) != 0))
                    {
                        ret = (uint32_t)ntohl(((struct sockaddr_in *)i->ifa_addr)->sin_addr.s_addr);
                        break;
                    }

                    // sometimes the sa_family is not AF_INET but we can still try 
                    else
                    {
                        int fd = socket(AF_INET, SOCK_DGRAM, 0);
                        if (fd < 0)
                        {
                            i = i->ifa_next;
                            continue;
                        }
                        struct ifreq ifr;
                        
                        ifr.ifr_addr.sa_family = AF_INET;
                        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", i->ifa_name);

                        auto err = ioctl(fd, SIOCGIFADDR, &ifr);
                        close(fd);
                        if (err == 0 &&
                            (!use_default_interface || strncmp((const char*)&((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr, loopback, 4) != 0))
                        {
                            ret = (uint32_t)ntohl(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr);
                            break;
                        }
                    }
                }
            }
            i = i->ifa_next;
        }

        if (i == nullptr)
        {
            derror("get local ip from network interfaces failed, network_interface = %s", network_interface);
        }

        if (ifa != nullptr)
        {
            // remember to free it
            freeifaddrs(ifa);
        }
    }
# endif

    return ret;
}

DSN_API const char*   dsn_address_to_string(dsn_address_t addr)
{
    char* p = dsn::tls_dsn.scratch_next();
    auto sz = sizeof(dsn::tls_dsn.scratch_buffer[0]);
    struct in_addr net_addr;
# if defined(_WIN32)
    char* ip_str;
# else
    int ip_len;
# endif

    switch (addr.u.v4.type)
    {
    case HOST_TYPE_IPV4:
        net_addr.s_addr = htonl((uint32_t)addr.u.v4.ip);
# if defined(_WIN32)
        ip_str = inet_ntoa(net_addr);
        snprintf_p(p, sz, "%s:%hu", ip_str, (uint16_t)addr.u.v4.port);
# else
        inet_ntop(AF_INET, &net_addr, p, sz);
        ip_len = strlen(p);
        snprintf_p(p + ip_len, sz - ip_len, ":%hu", (uint16_t)addr.u.v4.port);
# endif
        break;
    case HOST_TYPE_URI:
        if (addr.u.uri.uri == 0)
        {
            p = (char*)"invalid address";
        }
        else
        {
            p = (char*)(uintptr_t)addr.u.uri.uri;
        }
        break;
    case HOST_TYPE_GROUP:
        if (addr.u.group.group == 0)
        {
            p = (char*)"invalid address";
        }
        else
        {
            p = (char*)(((dsn::rpc_group_address*)(uintptr_t)(addr.u.group.group))->name());
        }
        break;
    default:
        p = (char*)"invalid address";
        break;
    }

    return (const char*)p;
}

DSN_API dsn_address_t dsn_address_build(
    const char* host,
    uint16_t port
    )
{
    if (host == nullptr || host[0] == '\0')
    {
        derror("dsn_address_build got null or empty host");
        return dsn::rpc_address().c_addr();
    }

    dsn::rpc_address addr(host, port);
    return addr.c_addr();
}

DSN_API dsn_address_t dsn_address_build_ipv4(
    uint32_t ipv4,
    uint16_t port
    )
{
    dsn::rpc_address addr(ipv4, port);
    return addr.c_addr();
}

DSN_API dsn_address_t dsn_address_build_group(
    dsn_group_t g
    )
{
    if (g == nullptr)
    {
        derror("dsn_address_build_group got null group");
        return dsn::rpc_address().c_addr();
    }

    dsn::rpc_address addr;
    addr.assign_group(g);
    return addr.c_addr();
}

DSN_API dsn_address_t dsn_address_build_uri(
    dsn_uri_t uri
    )
{
    if (uri == nullptr)
    {
        derror("dsn_address_build_uri got null uri");
        return dsn::rpc_address().c_addr();
    }

    dsn::rpc_address addr;
    addr.assign_uri(uri);
    return addr.c_addr();
}

DSN_API dsn_group_t dsn_group_build(const char* name) // must be paired with release later
{
    if (name == nullptr || name[0] == '\0')
    {
        derror("dsn_group_build got null or empty name");
        return nullptr;
    }

    auto g = new ::dsn::rpc_group_address(name);
    return g;
}

DSN_API int dsn_group_count(dsn_group_t g)
{
    if (g == nullptr)
    {
        derror("dsn_group_count got null group");
        return 0;
    }

    auto grp = (::dsn::rpc_group_address*)(g);
    return grp->count();
}

DSN_API bool dsn_group_add(dsn_group_t g, dsn_address_t ep)
{
    if (g == nullptr)
    {
        derror("dsn_group_add got null group");
        return false;
    }

    auto grp = (::dsn::rpc_group_address*)(g);
    ::dsn::rpc_address addr(ep);
    return grp->add(addr);
}

DSN_API void dsn_group_set_leader(dsn_group_t g, dsn_address_t ep)
{
    if (g == nullptr)
    {
        derror("dsn_group_set_leader got null group");
        return;
    }

    auto grp = (::dsn::rpc_group_address*)(g);
    ::dsn::rpc_address addr(ep);
    grp->set_leader(addr);
}

DSN_API dsn_address_t dsn_group_get_leader(dsn_group_t g)
{
    if (g == nullptr)
    {
        derror("dsn_group_get_leader got null group");
        return dsn::rpc_address().c_addr();
    }

    auto grp = (::dsn::rpc_group_address*)(g);
    return grp->leader().c_addr();
}

DSN_API bool dsn_group_is_leader(dsn_group_t g, dsn_address_t ep)
{
    if (g == nullptr)
    {
        derror("dsn_group_is_leader got null group");
        return false;
    }

    auto grp = (::dsn::rpc_group_address*)(g);
    return grp->leader() == ep;
}

DSN_API bool dsn_group_is_update_leader_automatically(dsn_group_t g)
{
    if (g == nullptr)
    {
        derror("dsn_group_is_update_leader_automatically got null group");
        return false;
    }

    auto grp = (::dsn::rpc_group_address*)(g);
    return grp->is_update_leader_automatically();
}

DSN_API void dsn_group_set_update_leader_automatically(dsn_group_t g, bool v)
{
    if (g == nullptr)
    {
        derror("dsn_group_set_update_leader_automatically got null group");
        return;
    }

    auto grp = (::dsn::rpc_group_address*)(g);
    grp->set_update_leader_automatically(v);
}

DSN_API dsn_address_t dsn_group_next(dsn_group_t g, dsn_address_t ep)
{
    if (g == nullptr)
    {
        derror("dsn_group_next got null group");
        return dsn::rpc_address().c_addr();
    }

    auto grp = (::dsn::rpc_group_address*)(g);
    ::dsn::rpc_address addr(ep);
    return grp->next(addr).c_addr();
}

DSN_API dsn_address_t dsn_group_forward_leader(dsn_group_t g)
{
    if (g == nullptr)
    {
        derror("dsn_group_forward_leader got null group");
        return dsn::rpc_address().c_addr();
    }

    auto grp = (::dsn::rpc_group_address*)(g);
    grp->leader_forward();
    return grp->leader().c_addr();
}

DSN_API bool dsn_group_remove(dsn_group_t g, dsn_address_t ep)
{
    if (g == nullptr)
    {
        derror("dsn_group_remove got null group");
        return false;
    }

    auto grp = (::dsn::rpc_group_address*)(g);
    ::dsn::rpc_address addr(ep);
    return grp->remove(addr);
}

DSN_API void dsn_group_destroy(dsn_group_t g)
{
    if (g == nullptr)
    {
        derror("dsn_group_destroy got null group");
        return;
    }

    auto grp = (::dsn::rpc_group_address*)(g);
    delete grp;
}

DSN_API dsn_uri_t dsn_uri_build(const char* url) // must be paired with destroy later
{
    if (url == nullptr || url[0] == '\0')
    {
        derror("dsn_uri_build got null or empty url");
        return nullptr;
    }

    return (dsn_uri_t)new ::dsn::rpc_uri_address(url);
}

DSN_API void dsn_uri_destroy(dsn_uri_t uri)
{
    if (uri == nullptr)
    {
        derror("dsn_uri_destroy got null uri");
        return;
    }

    delete (::dsn::rpc_uri_address*)(uri);
}
