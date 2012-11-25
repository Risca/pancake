#ifndef IN6_ADDR
#define IN6_ADDR

#include <in.h>

void pancake_print_addr(struct in6_addr *addr);
void pancake_format_addr(struct in6_addr *addr, char *str);
void pancake_addr_error_line(uint8_t byte_position);

#endif //IN6_ADDR
