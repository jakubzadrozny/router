#ifndef NET_UTILS
#define NET_UTILS

#include <string>
#include "consts.h"

struct packet {
    cidr_addr_t addr;
    distance_t  d;
    ip_addr_t   sender;
};

ssize_t     send_packet     (ip_addr_t dest, cidr_addr_t t, distance_t d);
cidr_addr_t str_to_cidr     (const std::string &cidr);
bool        fetch_packet    (packet &p);
char*       format_ip       (ip_addr_t ip);
bool        in_range        (ip_addr_t ip, cidr_addr_t net);

inline ip_addr_t generate_mask (prefix_t pref) {
    return ((1 << pref) - 1) << (IP_ADDRLEN - pref);
}

#endif
