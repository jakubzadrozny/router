// Jakub Zadrozny 290920
#include <iostream>
#include <unordered_map>

#include "net_utils.h"
#include "utils.h"
#include "consts.h"

extern std::unordered_map<cidr_addr_t, std::pair<distance_t, ip_addr_t>, pair_hash> dist;
extern std::unordered_map<cidr_addr_t, int, pair_hash> time_left;

extern std::unordered_map<ip_addr_t, int>   ifaceM[IP_ADDRLEN];
extern interface                            iface[MAX_INTERFACES];

extern size_t n;

int match_interface(ip_addr_t ip) {
    int         match   = -1;
    distance_t  d       = INF;

    for(prefix_t p = 1; p <= IP_ADDRLEN; p++) {
        auto mask       = generate_mask(p);
        auto ip_pref    = ip & mask;

        auto it = ifaceM[p].find(ip_pref);
        if(it != ifaceM[p].end()) {
            int cand = (*it).second;
            if(iface[cand].dist < d) {
                d = iface[cand].dist;
                match = cand;
            }
        }
    }

    return match;
}

void int_as_bytes(u_int32_t value, u_int8_t *buf) {
    buf[0] = value >> 24;
    buf[1] = value >> 16;
    buf[2] = value >> 8;
    buf[3] = value;
}

u_int32_t bytes_to_int (u_int8_t *buf) {
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

void set_dist(cidr_addr_t addr, distance_t d, ip_addr_t via) {
    if(d < INF) {
        time_left.erase(addr);
        dist[addr] = {d, via};
        return;
    } else {
        if(dist.count(addr) == 1 and dist[addr].first == INF) {
            return;
        } else {
            dist[addr] = {INF, via};
            time_left[addr] = BROADCAST_INF_FOR;
        }
    }
}

interface read_line () {
    std::string addr_s, ignore;
    distance_t  d;

    std::cin >> addr_s >> ignore >> d;

    auto addr = str_to_cidr(addr_s);
    return interface(addr, d);
}

void print (cidr_addr_t net, distance_t d, ip_addr_t via) {
    auto ip     = net.first;
    auto pref   = net.second;
    auto ip_str = format_ip(ip);

    std::cout << ip_str << "/" << (unsigned int) pref;
    if(d < INF) {
        std::cout << " distance " << d;
        if(via > 0) {
            ip_str = format_ip(via);
            std::cout << " via " << ip_str << "\n";
        } else {
            std::cout << " connected directly\n";
        }
    } else {
        std::cout << " unreachable";
        if(via == 0) {
            std::cout << " connected directly";
        }
        std::cout << "\n";
    }
}

void print_neighbors () {
    for(size_t i = 0; i < n; i++) {
        auto net = iface[i].net_cidr();
        if(dist.count(net) == 0) {
            print(net, INF, 0);
        }
    }
}

void print_vector () {
    for(auto x : dist) {
        auto net    = x.first;
        auto d      = x.second.first;
        auto via    = x.second.second;
        print(net, d, via);
    }
    print_neighbors();
    std::cout << "\n";
}
