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
# include <sys/time.h>
# include <sys/types.h>
# include <time.h>
# include <unistd.h>

# define MAX_IP_LEN 60
# define MAX_ICMP_LEN 76
# define PING_PKT_S 64
# define PING_DATALEN (PING_PKT_S - ICMP_MINLEN)
# define PING_MAX_DATALEN (USHRT_MAX - MAX_IP_LEN - MAX_ICMP_LEN)

# define PING_DEFAULT_INTERVAL 1000
# define PING_PRECISION 1000
# define PING_MIN_USER_INTERVAL (2000000 / PING_PRECISION)

# define RECV_TIMEOUT 1

extern int ping_loop;

enum {
	OPT_VERBOSE		= 0x001,
	OPT_QUIET		= 0x002
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
	char msg[1];
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
} t_ping_args;

typedef struct s_ping_info {
	char *ip_addr;
	char *hostname;
	t_sockaddr_in addr_con;
} t_ping_info;

typedef struct s_ping_stats {
	long double rtt_min;
	long double rtt_max;
	long double rtt_sum;
	long double rtt_sum_sq;
	int rtt_count;
} t_ping_stats;

// PARSING
t_ping_args parse_args(int argc, char *argv[]);
t_ping_info parse_ping_info(char *target, char *program_name);

// PARSING_UTILS
bool is_valid_ipv4(const char *ip);
char *resolve_hostname_to_ip(char *addr_host, t_sockaddr_in *addr_con);
size_t parse_number(const char *optarg, size_t max_val, bool allow_zero);

// PING
void send_ping(int ping_sockfd, t_ping_info *info, t_ping_args *args);

// HELP
void show_try(char *program_name);
void show_help(char *program_name);
void show_usage(char *program_name);

#endif
