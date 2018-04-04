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

#include "handler.h"

using std::cout;
using std::cin;
using std::string;

typedef u_int32_t ip_addr_t;
typedef u_int32_t distance_t;
typedef u_int8_t prefix_t;
typedef std::pair<ip_addr_t, prefix_t> cidr_addr_t;

const distance_t INF = 16;
const u_int16_t PORT = 54321;
const int TURN_TIME = 5;
const int IP_ADDRLEN = 32;
const int DGRAM_SIZE = 9;
const int BROADCAST_INF_FOR = 5;
const int ASSUME_DEAD_AFTER = 5;
const int MAX_NEIGHBORS = 100;

int sockfd;
size_t n;

struct pair_hash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1,T2> &p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);

        return h1 ^ h2;
    }
};

std::unordered_map<cidr_addr_t, std::pair<distance_t, ip_addr_t>, pair_hash> dist;
std::unordered_map<cidr_addr_t, int, pair_hash> time_left;

std::map<ip_addr_t, int> order;
ip_addr_t neighbor_ip[MAX_NEIGHBORS];
cidr_addr_t neighbor_net[MAX_NEIGHBORS];
distance_t neighbor_dist[MAX_NEIGHBORS];
int not_heard[MAX_NEIGHBORS];
bool heard[MAX_NEIGHBORS];

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

void intAsBytes(u_int32_t value, u_int8_t *buf) {
    buf[0] = value >> 24;
    buf[1] = value >> 16;
    buf[2] = value >> 8;
    buf[3]= value;
}

u_int32_t bytesToInt (u_int8_t *buf) {
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

cidr_addr_t str_to_cidr (const string &cidr) {
    auto slash = cidr.find('/');
    auto addr = cidr.substr(0, slash);
    auto pref = cidr.substr(slash + 1);

    prefix_t p = atoi(pref.c_str());

    sockaddr_in sa;
    inet_pton(AF_INET, addr.c_str(), &(sa.sin_addr));
    ip_addr_t comp_addr = ntohl(sa.sin_addr.s_addr);

    return {comp_addr, p};
}

void read_input() {
    distance_t d;
    string addr_s, ignore;
    cin >> n;
    for(size_t i = 0; i < n; i++) {
        cin >> addr_s >> ignore >> d;
        auto addr = str_to_cidr(addr_s);
        auto comp_addr = addr.first;
        auto p = addr.second;
        ip_addr_t mask = ((1 << (p + 1)) - 1) << (IP_ADDRLEN - p);
        ip_addr_t net_addr = comp_addr & mask;
        addr = {net_addr, p};

        order[comp_addr] = i;
        neighbor_ip[i] = comp_addr;
        neighbor_net[i] = addr;
        // Co dokładnie oznaczają adresy na wejściu?
        neighbor_dist[i] = d;

        dist[addr] = {d, 0};
    }
}

void send_packet (cidr_addr_t dest, cidr_addr_t t, distance_t d) {
    auto f = dest.first;
    auto p = dest.second;

    ip_addr_t dest_ip = f + (1 << (IP_ADDRLEN - p)) - 1;

    sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(dest_ip);
    addr.sin_port = htons(PORT);

    u_int8_t buf[DGRAM_SIZE];
    intAsBytes(htonl(t.first), buf);
    intAsBytes(htonl(d), buf + 5);
    buf[4] = t.second;

    sendto(sockfd, buf, DGRAM_SIZE, 0, (sockaddr*) &addr, sizeof(addr));
}

void send_packets() {
    for(size_t i = 0; i < n; i++) {
        auto dest = neighbor_net[i];
        if(dist.size() > 0) {
            for(auto x : dist) {
                auto t = x.first;
                auto d = x.second.first;
                send_packet(dest, t, d);
                // TO-DO: zatruwanie sciezek?
            }
        } else {
            send_packet(dest, dest, INF);
            // Bardzo dziwny graniczny przypadek
        }
    }
}

void read_packets() {
    sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);
    u_int8_t buf[DGRAM_SIZE];

    while(true) {
        auto dg_len = recvfrom(sockfd, buf, DGRAM_SIZE, MSG_DONTWAIT,
            (sockaddr*) &sender, &sender_len);

        if(dg_len < 0) return;

        ip_addr_t comp_addr = ntohl(bytesToInt(buf));
        prefix_t p = buf[4];
        distance_t d = ntohl(bytesToInt(buf + 5));

        cidr_addr_t net_addr = {comp_addr, p};

        ip_addr_t neighbor_addr = ntohl(sender.sin_addr.s_addr);
        int ord = order[neighbor_addr];
        distance_t neighbor_d = neighbor_dist[ord];

        heard[ord] = true;

        if(dist.count(net_addr) == 0) {
            set_dist(net_addr, d + neighbor_d, neighbor_addr);
        } else {
            ip_addr_t via = dist[net_addr].second;
            distance_t prev_d = dist[net_addr].first;

            if(d + neighbor_d < prev_d or via == neighbor_addr) {
                set_dist(net_addr, d + neighbor_d, neighbor_addr);
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
                auto addr = neighbor_net[i];
                set_dist(addr, INF, 0);
                auto ip = neighbor_ip[i];

                for(auto x : dist) {
                    auto via = x.second.second;
                    if(via == ip) {
                        set_dist(x.first, INF, 0);
                    }
                }
            }
        } else {
            if(not_heard[i] == ASSUME_DEAD_AFTER) {
                auto addr = neighbor_net[i];
                if(dist.count(addr) == 0 or
                    dist[addr].first >= neighbor_dist[i]) {
                    dist[addr] = {neighbor_dist[i], 0};
                }
            }
            not_heard[i] = 0;
        }
    }
    heard[i] = false;
}

void print_info () {
    struct sockaddr_in sa;
    char ip_str[INET_ADDRSTRLEN];
    for(auto x : dist) {
        auto ip = x.first.first;
        auto p = x.first.second;
        auto d = x.second.first;
        auto via = x.second.second;

        sa.sin_addr.s_addr = htonl(ip);
        inet_ntop(AF_INET, &(sa.sin_addr), ip_str, INET_ADDRSTRLEN);
        cout << ip_str << "/" << (unsigned int) p << " distance " << d;
        if(via > 0) {
            sa.sin_addr.s_addr = htonl(via);
            inet_ntop(AF_INET, &(sa.sin_addr), ip_str, INET_ADDRSTRLEN);
            cout << " via " << ip_str << "\n";
        } else {
            cout << " connected directly\n";
        }
    }
    cout << "\n";
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

    fd_set descriptors;
    FD_ZERO (&descriptors);
    FD_SET (sockfd, &descriptors);

    timeval turn, start, curr, elapsed, left;
    turn.tv_sec = TURN_TIME;
    turn.tv_usec = 0;
    while(true) {

        left = turn;
        gettimeofday(&start, NULL);

        while(left.tv_sec > 0 or (left.tv_sec == 0 and left.tv_usec > 0)) {
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
        print_info();
        send_packets();
    }
}
