#ifndef IN6_ADDR
#define IN6_ADDR

#include <netinet/in.h>

void pancake_print_addr(struct in6_addr *addr);
void pancake_format_addr(struct in6_addr *addr, char *str);

#endif //IN6_ADDR
