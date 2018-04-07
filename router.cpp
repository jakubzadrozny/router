#include <cstdlib>
#include <cstring>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

#include "consts.h"
#include "handler.h"
#include "net_utils.h"
#include "utils.h"

int sockfd;
size_t n;

std::unordered_map<cidr_addr_t, std::pair<distance_t, ip_addr_t>, pair_hash> dist;
std::unordered_map<cidr_addr_t, int, pair_hash> time_left;

std::unordered_map<ip_addr_t, int>  ifaceM[IP_ADDRLEN + 1];

interface   iface[MAX_INTERFACES];
int         silent[MAX_INTERFACES];
bool        heard[MAX_INTERFACES];

void read_input() {
    std::cin >> n;
    for(size_t i = 0; i < n; i++) {
        interface _iface = read_line();
        iface[i] = _iface;

        auto ip     = _iface.net_ip();
        auto pref   = _iface.pref();
        ifaceM[pref][ip] = i;

        dist[_iface.net_cidr()] = {_iface.dist, 0};
    }
}

void send_packets() {
    for(size_t i = 0; i < n; i++) {
        auto dest = iface[i].broadcast();
        if(dist.size() > 0) {
            for(auto x : dist) {
                auto target = x.first;
                auto d      = x.second.first;
                if(send_packet(dest, target, d) < 0) {
                    silent[i] = ASSUME_DEAD_AFTER;
                }
            }
        } else {
            auto target = iface[i].net_cidr();
            if(send_packet(dest, target, INF) < 0) {
                silent[i] = ASSUME_DEAD_AFTER;
            }
            // Bardzo dziwny graniczny przypadek
        }
    }
}

void read_packets() {
    packet p;
    while(fetch_packet(p) > 0) {

        int i = match_interface(p.sender);
        if(i < 0 or p.sender == iface[i].interface_ip()) {
            continue;
        }

        heard[i] = true;
        distance_t neigh_d = iface[i].dist;

        if(dist.count(p.addr) == 0) {
            set_dist(p.addr, p.d + neigh_d, p.sender);
        } else {
            ip_addr_t via = dist[p.addr].second;
            distance_t prev_d = dist[p.addr].first;

            if(p.d + neigh_d < prev_d or via == p.sender) {
                set_dist(p.addr, p.d + neigh_d, p.sender);
            }
        }
    }
}

void process_info() {
    std::vector<cidr_addr_t>to_erase;
    for(auto x : dist) {
        auto addr = x.first;
        auto d = x.second.first;

        if(d == INF) {
            int t = time_left[addr];
            if(t == 0) {
                time_left.erase(addr);
                to_erase.push_back(addr);
            } else {
                time_left[addr] = t - 1;
            }
        }
    }
    while(to_erase.size()) {
        dist.erase(to_erase.back());
        to_erase.pop_back();
    }

    for(size_t i = 0; i < n; i++) {
        if(!heard[i]) {
            if(silent[i] == ASSUME_DEAD_AFTER) continue;
            if(++silent[i] == ASSUME_DEAD_AFTER) {
                auto addr = iface[i].net_cidr();
                set_dist(addr, INF, 0);

                for(auto x : dist) {
                    auto via = x.second.second;
                    if(in_range(via, addr)) {
                        auto target = x.first;
                        set_dist(target, INF, 0);
                    }
                }
            }
        } else {
            if(silent[i] == ASSUME_DEAD_AFTER) {
                auto addr = iface[i].net_cidr();
                auto d    = iface[i].dist;
                if(dist.count(addr) == 0 or dist[addr].first >= d) {
                    dist[addr] = {d, 0};
                }
            }
            silent[i] = 0;
        }
        heard[i] = false;
    }
}

int main () {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        handle_error("socket error", true);
	}
    int broadcastPermission = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST,
        (void *) &broadcastPermission, sizeof(broadcastPermission)) < 0) {
        handle_error("socket error", true);
    }

	sockaddr_in server_address;
	bzero (&server_address, sizeof(server_address));
	server_address.sin_family      = AF_INET;
	server_address.sin_port        = htons(PORT);
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind (sockfd, (sockaddr*)&server_address, sizeof(server_address)) < 0) {
		handle_error("bind error", true);
	}

    read_input();

    timeval turn, start, curr, elapsed, left;
    turn.tv_sec = TURN_TIME;
    turn.tv_usec = 0;
    while(true) {

        left = turn;
        gettimeofday(&start, NULL);

        while(left.tv_sec > 0 or (left.tv_sec == 0 and left.tv_usec > 0)) {
            fd_set descriptors;
            FD_ZERO (&descriptors);
            FD_SET (sockfd, &descriptors);

            int ready = select (sockfd+1, &descriptors, NULL, NULL, &left);
            if (ready < 0) {
                handle_error("select error", true);
            } else if (ready > 0) {
                read_packets();
            }

            gettimeofday(&curr, NULL);
            timersub(&curr, &start, &elapsed);
            timersub(&turn, &elapsed, &left);
        }

        process_info();
        print_vector();
        send_packets();
    }
}
