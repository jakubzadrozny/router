// Jakub Zadrozny 290920
#include "net_utils.h"
#include "utils.h"
#include "consts.h"
#include "handler.h"

#include <climits>
#include <cerrno>
#include <cstring>
#include <netinet/ip.h>
#include <arpa/inet.h>

extern int sockfd;

sockaddr_in sender;
socklen_t   sender_len = sizeof(sender);
u_int8_t    buf[DGRAM_SIZE];
char        ip_str[INET_ADDRSTRLEN];

bool in_range (ip_addr_t ip, cidr_addr_t net) {
    auto net_ip = net.first;
    auto pref   = net.second;
    auto mask   = generate_mask(pref);

    return (ip & mask) == net_ip;
}

bool fetch_packet (packet &p) {
    while(true) {
        auto dg_len = recvfrom(sockfd, buf, DGRAM_SIZE, MSG_DONTWAIT,
            (sockaddr*) &sender, &sender_len);

        if(dg_len == DGRAM_SIZE) {
            break;
        } else if(dg_len < 0) {
            if(errno == EWOULDBLOCK) {
                return false;
            } else {
                handle_error("socket error", true);
            }
        }
    }

    ip_addr_t   ip_addr = ntohl(bytes_to_int(buf));
    prefix_t    pref    = buf[4];

    p.addr      = {ip_addr, pref};
    p.d         = ntohl(bytes_to_int(buf + 5));
    p.sender    = ntohl(sender.sin_addr.s_addr);

    if(p.d == UINT_MAX) {
        p.d = INF;
    }

    return true;
}

char* format_ip (ip_addr_t ip) {
    sender.sin_addr.s_addr = htonl(ip);
    inet_ntop(AF_INET, &(sender.sin_addr), ip_str, INET_ADDRSTRLEN);
    return ip_str;
}

ssize_t send_packet (ip_addr_t dest, cidr_addr_t target, distance_t d) {
    sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(dest);
    addr.sin_port = htons(PORT);

    if(d == INF) {
        d = UINT_MAX;
    }

    int_as_bytes(htonl(target.first), buf);
    int_as_bytes(htonl(d), buf + 5);
    buf[4] = target.second;

    return sendto(sockfd, buf, DGRAM_SIZE, 0, (sockaddr*) &addr, sizeof(addr));
}

cidr_addr_t str_to_cidr (const std::string &cidr) {
    auto slash = cidr.find('/');
    auto addr = cidr.substr(0, slash);
    auto pref = cidr.substr(slash + 1);

    prefix_t p = atoi(pref.c_str());

    sockaddr_in sa;
    inet_pton(AF_INET, addr.c_str(), &(sa.sin_addr));
    ip_addr_t comp_addr = ntohl(sa.sin_addr.s_addr);

    return {comp_addr, p};
}
