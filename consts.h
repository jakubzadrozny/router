#ifndef CONSTS
#define CONSTS

#include <cstdlib>
#include <utility>

typedef u_int32_t ip_addr_t;
typedef u_int32_t distance_t;
typedef u_int8_t prefix_t;
typedef std::pair<ip_addr_t, prefix_t> cidr_addr_t;

const distance_t INF = 16;
const u_int16_t PORT = 54321;
const int TURN_TIME = 5;
const int IP_ADDRLEN = 32;
const int DGRAM_SIZE = 9;
const int BROADCAST_INF_FOR = 3;
const int ASSUME_DEAD_AFTER = 3;
const int MAX_INTERFACES = 1000;

#endif
