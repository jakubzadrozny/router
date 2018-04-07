#ifndef UTILS
#define UTILS

#include "consts.h"

struct pair_hash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1,T2> &p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);

        return h1 ^ h2;
    }
};

struct line {
    cidr_addr_t addr;
    distance_t  d;
};

void        int_as_bytes    (u_int32_t value, u_int8_t *buf);
u_int32_t   bytes_to_int    (u_int8_t *buf);
void        print_vector    ();
void        set_dist        (cidr_addr_t addr, distance_t d, ip_addr_t via);
line        read_line       ();
int         match_interface (ip_addr_t ip);

#endif
