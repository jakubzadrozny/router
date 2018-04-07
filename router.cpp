#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

#include "consts.h"
#include "handler.h"
#include "net_utils.h"
#include "utils.h"

int sockfd;
size_t n;

std::unordered_map<cidr_addr_t, std::pair<distance_t, ip_addr_t>, pair_hash> dist;
std::unordered_map<cidr_addr_t, int, pair_hash> time_left;

std::unordered_map<ip_addr_t, int>  interfaces[IP_ADDRLEN];

cidr_addr_t interface_addr[MAX_INTERFACES];
distance_t  interface_dist[MAX_INTERFACES];
int         not_heard[MAX_INTERFACES];
bool        heard[MAX_INTERFACES];

void read_input() {
    std::cin >> n;
    for(size_t i = 0; i < n; i++) {
        line l = read_line();

        interface_addr[i] = l.addr;
        interface_dist[i] = l.d;

        auto ip     = l.addr.first;
        auto pref   = l.addr.second;
        interfaces[pref][ip] = i;

        dist[l.addr] = {l.d, 0};
    }
}

void send_packets() {
    for(size_t i = 0; i < n; i++) {
        auto dest = interface_addr[i];
        if(dist.size() > 0) {
            for(auto x : dist) {
                auto target = x.first;
                auto d      = x.second.first;
                send_packet(dest, target, d);
                // TO-DO: zatruwanie sciezek?
            }
        } else {
            send_packet(dest, dest, INF);
            // Bardzo dziwny graniczny przypadek
        }
    }
}

void read_packets() {
    packet p;
    while(fetch_packet(p) > 0) {

        int neigh = match_interface(p.sender);
        if(neigh < 0) {
            continue;
        }

        heard[neigh] = true;
        distance_t neigh_d = interface_dist[neigh];

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
            if(not_heard[i] == ASSUME_DEAD_AFTER) continue;
            if(++not_heard[i] == ASSUME_DEAD_AFTER) {
                auto addr   = interface_addr[i];
                set_dist(addr, INF, 0);

                for(auto x : dist) {
                    auto via    = x.second.second;
                    if(in_range(via, addr)) {
                        auto target = x.first;
                        set_dist(target, INF, 0);
                    }
                }
            }
        } else {
            if(not_heard[i] == ASSUME_DEAD_AFTER) {
                auto addr = interface_addr[i];
                if(dist.count(addr) == 0 or
                    dist[addr].first >= interface_dist[i]) {
                    dist[addr] = {interface_dist[i], 0};
                }
            }
            not_heard[i] = 0;
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
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST,
        (void *) &broadcastPermission, sizeof(broadcastPermission));

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
