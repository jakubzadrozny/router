#ifndef UTILS
#define UTILS

#include "net_utils.h"
#include "consts.h"

struct pair_hash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1,T2> &p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);

        return h1 ^ h2;
    }
};

struct interface {
    cidr_addr_t addr;
    distance_t  dist;

    interface() {}
    interface(ip_addr_t ip, prefix_t pref, distance_t d) : addr({ip, pref}), dist(d) {}
    interface(cidr_addr_t a, distance_t d) : addr(a), dist(d) {}

    ip_addr_t net_ip () {
        auto ip     = addr.first;
        auto pref   = addr.second;
        auto mask   = generate_mask(pref);
        return ip & mask;
    }

    prefix_t pref () {
        return addr.second;
    }

    cidr_addr_t net_cidr () {
        auto ip = net_ip();
        auto p  = addr.second;
        return {ip, p};
    }

    cidr_addr_t interface_cidr () {
        return addr;
    }

    ip_addr_t interface_ip () {
        return addr.first;
    }

    ip_addr_t broadcast () {
        auto ip     = addr.first;
        auto p      = addr.second;
        auto mask   = ~generate_mask(p);
        return ip | mask;
    }
};

void        int_as_bytes    (u_int32_t value, u_int8_t *buf);
u_int32_t   bytes_to_int    (u_int8_t *buf);
void        print_vector    ();
void        set_dist        (cidr_addr_t addr, distance_t d, ip_addr_t via);
interface   read_line       ();
int         match_interface (ip_addr_t ip);
void        mark_dead       (size_t i);

#endif
