#ifndef FT_PING_H
# define FT_PING_H

# define _DEFAULT_SOURCE

# include <stdio.h>
# include <unistd.h>
# include <getopt.h>
# include <stdlib.h>
# include <stdbool.h>
# include <error.h>
# include <limits.h>
# include <netinet/icmp6.h>
# include <netinet/ip_icmp.h>
# include <sys/types.h>
# include <libgen.h>

# define PING_DATALEN (64 - ICMP_MINLEN)
# define PING_MAX_DATALEN (65535 - sizeof (struct icmp6_hdr))
# define DEFAULT_PING_COUNT 0
# define MAX_WAIT 10
# define PING_PRECISION 1000
# define PING_MIN_USER_INTERVAL (2000000 / PING_PRECISION)

enum {
	OPT_VERBOSE		= 0x001,
	OPT_QUIET		= 0x002,
	OPT_FLOOD		= 0x004,
	OPT_INTERVAL	= 0x008
};

enum {
	ARG_USAGE = 256
};

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

// MAIN
t_ping_args parse_args(int argc, char *argv[]);

// HELP
void show_missing(char *program_name);
void show_help(char *program_name);
void show_usage(char *program_name);

#endif
