#ifndef FT_PING_H
# define FT_PING_H

# define _DEFAULT_SOURCE

# include <arpa/inet.h>
# include <errno.h>
# include <error.h>
# include <getopt.h>
# include <libgen.h>
# include <limits.h>
# include <math.h>
# include <netinet/icmp6.h>
# include <netinet/ip_icmp.h>
# include <netdb.h>
# include <signal.h>
# include <stdarg.h>
# include <stdbool.h>
# include <stdio.h>
# include <stdlib.h>
# include <sys/types.h>
# include <time.h>
# include <unistd.h>

# define PING_PKT_S 64
# define PING_DATALEN (PING_PKT_S - ICMP_MINLEN)
# define PING_MAX_DATALEN (65535 - sizeof (struct icmp6_hdr))

# define PING_DEFAULT_INTERVAL 1000
# define PING_PRECISION 1000
# define PING_MIN_USER_INTERVAL (2000000 / PING_PRECISION)

# define PORT_NO 0
# define RECV_TIMEOUT 1
# define DEFAULT_PING_COUNT 0
# define MAX_WAIT 10

extern int ping_loop;

enum {
	OPT_VERBOSE		= 0x001,
	OPT_QUIET		= 0x002,
	OPT_FLOOD		= 0x004,
	OPT_INTERVAL	= 0x008
};

enum {
	ARG_USAGE = 256
};

typedef struct sockaddr t_sockaddr;
typedef struct sockaddr_in t_sockaddr_in;
typedef struct ip t_ip;
typedef struct icmphdr t_icmphdr;
typedef struct hostent t_hostent;
typedef struct in_addr t_in_addr;
typedef struct timespec t_timespec;
typedef struct timeval t_timeval;

typedef struct s_ping_pkt {
	t_icmphdr hdr;
	char msg[64 - sizeof(t_icmphdr)];
} t_ping_pkt;

typedef struct s_ping_args {
	char **adresses;
	size_t interval;
	size_t count;
	int tos;
	int timeout;
	int linger;
	size_t size;
	unsigned int options;
	unsigned long preload;
} t_ping_args;

typedef struct s_ping_info {
	char *ip_addr;
	char *hostname;
	t_sockaddr_in addr_con;
} t_ping_info;

// PARSING
t_ping_args parse_args(int argc, char *argv[]);
t_ping_info parse_ping_info(char *target, char *program_name);

// PING
void send_ping(int ping_sockfd, t_ping_info *info, t_ping_args *args);

// HELP
void show_missing(char *program_name);
void show_help(char *program_name);
void show_usage(char *program_name);

// ERRORS
void show_error(int code, char *msg, ...);

#endif
