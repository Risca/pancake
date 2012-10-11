#ifndef IP6_ADDR
#define IP6_ADDR

#include<inttypes.h>

// Define struct for ip6 address
struct pancake_in6_addr {
    uint8_t s6_addr[16];
};

void pancake_print_addr(struct pancake_in6_addr *addr);
void pancake_format_addr(struct pancake_in6_addr *addr, char *str);

#endif //IP6_ADDR
