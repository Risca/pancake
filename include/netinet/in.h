#ifndef _NETINET_IN_H
#define _NETINET_IN_H	1

#include<inttypes.h>

// Define struct for ip6 address
struct in6_addr {
    uint8_t s6_addr[16];
};

#endif //_NETINET_IN_H
