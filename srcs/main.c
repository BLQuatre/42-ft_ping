#include "ft_ping.h"

int ping_loop = 1;

static void show_args(t_ping_args args) {
	printf("Args:\n");
	for (int i = 0; args.adresses && args.adresses[i]; i++) {
		printf("  address[%d]: %s\n", i, args.adresses[i]);
	}
	printf("  verbose: %d\n", args.options & OPT_VERBOSE);
	printf("  quiet: %d\n", args.options & OPT_QUIET);
	printf("  interval: %ld\n", args.interval);
	printf("  count: %ld\n", args.count);
	printf("  tos: %d\n", args.tos);
	printf("  timeout: %d\n", args.timeout);
	printf("  linger: %d\n", args.linger);
	printf("  size: %ld\n", args.size);
	printf("  flood: %d\n", args.options & OPT_FLOOD);
	printf("  preload: %ld\n", args.preload);
}

static void show_info(t_ping_info info) {
	printf("Info:\n");
	printf("  hostname: %s\n", info.hostname);
	printf("  ip_addr: %s\n", info.ip_addr);
	printf("  addr_con.sin_family: %d\n", info.addr_con.sin_family);
	printf("  addr_con.sin_port: %d\n", ntohs(info.addr_con.sin_port));
	printf("  addr_con.sin_addr.s_addr: %u\n", ntohl(info.addr_con.sin_addr.s_addr));
}

static void intHandler(int dummy) {
	(void) dummy;
	ping_loop = 0;
}

int main(int argc, char *argv[]) {
	t_ping_args args;
	int sockfd;
	t_ping_info info;

	argv[0] = basename(argv[0]);

	args = parse_args(argc, argv);
	show_args(args);

	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0) {
		if (errno == EPERM || errno == EACCES) {
			fprintf(stderr, "ping: Lacking privilege for icmp socket.\n");
		} else {
			fprintf (stderr, "ping: %s\n", strerror(errno));
		}
		free(args.adresses);
		return EXIT_FAILURE;
	}

	info = parse_ping_info(args.adresses[0], argv[0]);
	show_info(info);

	printf("PING %s (%s): %ld data bytes\n", info.hostname, info.ip_addr, args.size);

	signal(SIGINT, intHandler);

	send_ping(sockfd, &info, &args);

	free(args.adresses);
	free(info.ip_addr);
	free(info.hostname);

	return 0;
}
