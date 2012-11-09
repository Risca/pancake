#include <pancake.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#ifdef _WIN32
    #include <windows.h>
    #include <memory.h>
#else
    #include <pthread.h>
#endif
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <helpers.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BACKLOG 5
#define MYPORT "3490"

int clientfd;

static PANCSTATUS linux_sockets_init_func(void *dev_data);
static PANCSTATUS linux_sockets_write_func(void *dev_data, struct pancake_ieee_addr *dest, uint8_t *data, uint16_t length);
static PANCSTATUS linux_sockets_destroy_func(void *dev_data);
static void linux_sockets_read_func(uint8_t *data, int16_t length);

struct pancake_port_cfg linux_sockets_cfg = {
	.init_func = linux_sockets_init_func,
    .write_func = linux_sockets_write_func,
	.destroy_func = linux_sockets_destroy_func,
};

void pancake_print_raw_bits(FILE *out, uint8_t *bytes, size_t length)
{
	uint8_t bit;
	uint16_t i;
	uint8_t j;

	if (bytes == NULL) {
		return;
	}

	if (out == NULL) {
		out = stdout;
	}

	fputs("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n", out);
	for (i=0; i < length; i++) {
		fprintf(out, "|");
		for (j=0; j < 8; j++) {
			bit = ( (*bytes) >> (7-j%8) ) & 0x01;
			fprintf(out, "%i", bit);

			/* Place a space between every bit except last */
			if (j != 7) {
				fprintf(out, " ");
			}
		}

		if ( (i+1) % 4 == 0 ) {
			fputs("|\n", out);
			fputs("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n", out);
		}
		bytes++;
	}
	if (length % 4 != 0) {
		fputs("|\n+", out);
		for (i=0; i < length % 4; i++) {
			fputs("-+-+-+-+-+-+-+-+", out);
		}
		fputs("\n", out);
	}
}

static void populate_dummy_ipv6_header(struct ip6_hdr *hdr, uint16_t payload_length)
{
	/* Loopback (::1/128) */
	struct in6_addr addr = {
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 1};

	hdr->ip6_flow	=	htonl(6 << 28);
	hdr->ip6_plen	=	htons(payload_length);
	hdr->ip6_nxt	=	254;
	hdr->ip6_hops	=	2;

    // Add next bytes
	memcpy((uint8_t *)hdr + 8, &addr, 16);
	memcpy((uint8_t *)hdr + 24, &addr, 16);
}

#ifdef _WIN32
HANDLE win_thread;
#else // Linux
pthread_t my_thread;
#endif

static void reuse_addr(int listener)
{
	int yes=1;
	//char yes='1'; // Solaris people use this

	// lose the pesky "Address already in use" error message
	if (setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		perror("setsockopt");
	} 
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

static void linux_sockets_thread(void *dev_data)
{
	uint8_t i, timeout = 1;
	int status;
	uint8_t buf[102];
	PANCSTATUS ret;
	int numbytes;
	int sockfd, new_fd;
	char            ipaddr[INET6_ADDRSTRLEN];
	char            s[INET6_ADDRSTRLEN];
	char            *ipstr      = (char *)dev_data;
	struct addrinfo hints, *res, *p;
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	struct pancake_ieee_addr ieee_addr = { 
        .ieee_ext = { 
            0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x01,
        },  
        .addr_mode = PANCAKE_IEEE_ADDR_MODE_EXTENDED,
    };

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

	getaddrinfo(NULL, MYPORT, &hints, &res);

	if (strcmp(ipstr, "::") != 0) {
		/* We are not listening */
		return;
	}

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		fprintf(stderr, "Failed to create socket!\n");
	}
	reuse_addr(sockfd);

	status = bind(sockfd, res->ai_addr, res->ai_addrlen);
	if (status < 0) {
		fprintf(stderr, "Failed to bind!\n");
	}

	status = listen(sockfd, BACKLOG);
	if (status < 0) {
		fprintf(stderr, "Failed to listen!\n");
	}

	addr_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

	inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
    printf("server: got connection from %s\n", s);

	while (1) {
		numbytes = recv(new_fd, buf, 102, 0);
		if (numbytes < 0) {
			perror("Failed to receive");
		}

		if (numbytes == 0) {
			printf("Connection closed\n");
			break;
		}
		pancake_process_data(dev_data, &ieee_addr, &ieee_addr, buf, numbytes);
	}

	close(new_fd);
	close(sockfd);
}

static int linux_sockets_connect(void *dev_data)
{
	uint8_t i, timeout = 0;
	int status;
	uint8_t data[127*3];
	uint16_t length = 1 + 40 + 2;
	PANCSTATUS ret;
	int numsent;
	int sockfd, new_fd;
	char            ipaddr[INET6_ADDRSTRLEN];
	char            s[INET6_ADDRSTRLEN];
	char            *ipstr      = (char *)dev_data;
	struct ip6_hdr 	*hdr 		= (struct ip6_hdr *)(data+1);
	uint8_t			*payload	= data + 1 + 40;
	struct addrinfo hints, *res, *p;
	struct sockaddr_storage their_addr;
	socklen_t addr_size;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(ipstr, MYPORT, &hints, &res);

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		fprintf(stderr, "Failed to create socket!\n");
		goto err_out;
	}

	do {
		status = connect(sockfd, res->ai_addr, res->ai_addrlen);
		if (status < 0) {
			fprintf(stderr, "Failed to connect!\n");
			sleep(2);
			timeout++;
		}
	} while (status < 0 && timeout < 5);

	if (status < 0) {
		goto err_out;
	}

	return sockfd;
err_out:
	return -1;
}

static PANCSTATUS linux_sockets_init_func(void *dev_data)
{
	char *ipstr = (char *)dev_data;
    #ifdef _WIN32
	win_thread = CreateThread(NULL, 0, &linux_sockets_thread, dev_data, 0, NULL);
	#else // Linux
	pthread_create (&my_thread, NULL, (void *) &linux_sockets_thread, dev_data);
	#endif

	if (strcmp(ipstr, "::") == 0) {
		return PANCSTATUS_OK;
	}
	clientfd = linux_sockets_connect(dev_data);
	if (clientfd < 0) {
		goto err_out;
	}

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}

static PANCSTATUS linux_sockets_write_func(void *dev_data, struct pancake_ieee_addr *dest, uint8_t *data, uint16_t length)
{
	ssize_t numbytes;

	printf("Sending packet:\n");
	printf("length: %u\n", length);
	pancake_print_raw_bits(NULL, data, length);
	numbytes = send(clientfd, data, length, 0);
	if (numbytes < 0) {
		perror("Failed to receive");
	}

	if (numbytes == 0) {
		printf("Connection closed\n");
		return;
	}

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}

static PANCSTATUS linux_sockets_destroy_func(void *dev_data)
{
    #ifdef _WIN32
    WaitForSingleObject(win_thread, INFINITE);
	#else // Linux
	pthread_join(my_thread, NULL);
	#endif

	close(clientfd);
	return PANCSTATUS_OK;
}

