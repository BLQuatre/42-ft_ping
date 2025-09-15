#include "ft_ping.h"

int ping_loop = 1;

static void intHandler(int dummy) {
	(void) dummy;
	ping_loop = 0;
}

int main(int argc, char *argv[]) {
	t_ping_args args;
	int sockfd;
	t_ping_info info;

	args = parse_args(argc, argv);

	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0) {
		if (errno == EPERM || errno == EACCES) {
			fprintf(stderr, "ping: Lacking privilege for icmp socket.\n");
		} else {
			fprintf(stderr, "ping: %s\n", strerror(errno));
		}
		free(args.adresses);
		return EXIT_FAILURE;
	}

	info = parse_ping_info(args.adresses[0], basename(argv[0]));
	if (info.ip_addr == NULL) {
		free(args.adresses);
		close(sockfd);
		return EXIT_FAILURE;
	}

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
	close(sockfd);

	return 0;
}
