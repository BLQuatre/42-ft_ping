#include "ft_ping.h"

void show_args(t_ping_args args) {
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
	printf("\n");
}

int main(int argc, char *argv[]) {
	t_ping_args args;
	args = parse_args(argc, argv);
	show_args(args);

	free(args.adresses);

	return 0;
}
