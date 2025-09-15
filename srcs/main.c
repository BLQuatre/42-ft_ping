#include "ft_ping.h"

int ping_loop = 1;

static void show_args(t_ping_args args) {
	printf("adrs: [");
	for (int i = 0; args.adresses && args.adresses[i]; i++) {
		printf("%s%s", i > 0 ? ", " : "", args.adresses[i]);
	}
	printf("]");

	printf(" |v: %d", args.options & OPT_VERBOSE);
	printf(" |q: %d", args.options & OPT_QUIET);
	printf(" |i: %ld", args.interval);
	printf(" |c: %ld", args.count);
	printf(" |tos: %d", args.tos);
	printf(" |tout: %d", args.timeout);
	printf(" |linger: %d", args.linger);
	printf(" |s: %ld", args.size);
	printf("\n");
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

	printf("PING %s (%s): %ld data bytes", info.hostname, info.ip_addr, args.size);
	if (args.options & OPT_VERBOSE) {
		printf(", id 0x%x = %d", getpid(), getpid());
	}
	printf("\n");

	signal(SIGINT, intHandler);

	send_ping(sockfd, &info, &args);

	free(args.adresses);
	free(info.ip_addr);
	free(info.hostname);

	return 0;
}
