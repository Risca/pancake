#include <in6_addr.h>
#include <netinet/in.h>
#include <stdio.h>

/**
 * Prints an IPv6 address
 */
void pancake_print_addr(struct in6_addr *addr)
{
    char str[32];
    pancake_format_addr(addr, str);
    printf("%s", str);
}

/**
 * Format an IPv6 address
 */
void pancake_format_addr(struct in6_addr *addr, char *str)
{
    int i;
    uint8_t *addr_p = (uint8_t*)addr;
    uint16_t temp;

    temp = (*addr_p << 8) | *(addr_p+1);
    addr_p += 2;
    sprintf(str, "%.4x", temp);

    for (i = 2; i < 16; i+=2) {
        temp = (*addr_p << 8) | *(addr_p+1);
        addr_p += 2;
        sprintf(str, "%s:%.4x", str, temp);
    }
}

/**
 * Print IPv6 address error position
 */
void pancake_addr_error_line(uint8_t byte_position)
{

    for(; byte_position > 0; byte_position -= 1) {

        if (byte_position % 2 == 0) {
            printf("%s", " ");
        }

        printf("%s", "  ");
    }
    printf("%s", "^^\n");
}
