#ifndef _NETINET_IN_H
#define _NETINET_IN_H	1

#include <inttypes.h>
#ifdef __linux__
	#include <sys/socket.h>
#endif

/* rfc3493 */
#define INET_ADDRSTRLEN    16
#define INET6_ADDRSTRLEN   46

// Define struct for ip6 address
struct in6_addr {
    uint8_t s6_addr[16];
};

/* Type to represent a port.  */
typedef uint16_t in_port_t;

#ifdef __linux__
	/* Structure describing an Internet socket address.  */
	struct sockaddr_in6 {
	uint8_t         sin6_len;       /* length of this struct */
	sa_family_t     sin6_family;    /* AF_INET6 */
	in_port_t       sin6_port;      /* transport layer port # */
	uint32_t        sin6_flowinfo;  /* IPv6 flow information */
	struct in6_addr sin6_addr;      /* IPv6 address */
	uint32_t        sin6_scope_id;  /* set of interfaces for a scope */
	};
#endif

typedef uint32_t in_addr_t;

#ifndef _WIN32
struct in_addr
{
	in_addr_t s_addr;
};

#define panc_bswap_16(x) \
     ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))
#define panc_bswap_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
 
#if PANC_BIG_ENDIAN != 0
/* The host byte order is the same as network byte order,
   so these functions are all just identity.  */
# define ntohl(x)	(x)
# define ntohs(x)	(x)
# define htonl(x)	(x)
# define htons(x)	(x)
#else
# define ntohl(x)	panc_bswap_32 (x)
# define ntohs(x)	panc_bswap_16 (x)
# define htonl(x)	panc_bswap_32 (x)
# define htons(x)	panc_bswap_16 (x)
#endif
#endif /* _WIN32 */

#endif /* _NETINET_IN_H */
