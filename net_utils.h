#ifndef NET_UTILS
#define NET_UTILS

#include <string>
#include "consts.h"

struct packet {
    cidr_addr_t addr;
    distance_t  d;
    ip_addr_t   sender;
};

void        send_packet     (cidr_addr_t dest, cidr_addr_t t, distance_t d);
cidr_addr_t str_to_cidr     (const std::string &cidr);
ssize_t     fetch_packet    (packet &p);
char*       str_to_ip       (ip_addr_t ip);
bool        in_range        (ip_addr_t ip, cidr_addr_t net);

inline ip_addr_t   generate_mask   (prefix_t pref) {
    return ((1 << (pref + 1)) - 1) << (IP_ADDRLEN - pref);
}

#endif
