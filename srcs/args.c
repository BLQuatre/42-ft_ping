#include "ft_ping.h"

#define OPT_STR "vqc:i:w:W:s:fl:T:"

static struct option long_options[] = {
	{ "help",			no_argument,		NULL,	'?' },
	{ "usage",			no_argument,		NULL,	ARG_USAGE },

	{ "verbose",		no_argument,		NULL,	'v' },
	{ "quiet",			no_argument,		NULL,	'q' },
	{ "count",			required_argument,	NULL,	'c' },
	{ "interval",		required_argument,	NULL,	'i' },
	{ "tos",			required_argument,	NULL,	'T' },
	{ "timeout",		required_argument,	NULL,	'w' },
	{ "linger",			required_argument,	NULL,	'W' },
	{ "size",			required_argument,	NULL,	's' },

	// NOT SURE
	{ "flood",			no_argument,		NULL,	'f' },
	{ "preload",		required_argument,	NULL,	'l' },

	// END
	{ NULL,				0,					NULL,	0 }
};

static t_ping_args default_args() {
	t_ping_args args = {
		.count = DEFAULT_PING_COUNT,
		.interval = PING_DEFAULT_INTERVAL,
		.linger = MAX_WAIT,
		.preload = 0,
		.size = PING_DATALEN,
		.timeout = -1,
		.tos = -1
	};
	return args;
}

size_t parse_number(const char *optarg, size_t max_val, bool allow_zero) {
	char *p;
	unsigned long int n;

	n = strtoul(optarg, &p, 0);
	if (*p)
		error(EXIT_FAILURE, 0, "invalid value (`%s' near `%s')", optarg, p);
	if (n == 0 && !allow_zero)
		error(EXIT_FAILURE, 0, "option value too small: %s", optarg);
	if (max_val && n > max_val)
		error(EXIT_FAILURE, 0, "option value too big: %s", optarg);

	return n;
}

t_ping_args parse_args(int argc, char *argv[]) {
	t_ping_args args = default_args();

	char *endptr;
	int c;
	int opt_index;
	double tmp_interval;
	bool is_root = false;

	if (getuid() == 0)
		is_root = true;

	while (1) {
		opt_index = 0;

		c = getopt_long(argc, argv, OPT_STR, long_options, &opt_index);
		if (c == -1)
			break;

		switch (c) {
			case 'v':
				args.options |= OPT_VERBOSE;
				break;
			case 'q':
				args.options |= OPT_QUIET;
				break;
			case 'c':
				args.count = parse_number(optarg, 0, true);
				break;
			case 'i':
				tmp_interval = strtod(optarg, &endptr);
				if (*endptr)
					printf("invalid interval value (`%s' near `%s')\n", optarg, endptr);
				args.options |= OPT_INTERVAL;
				args.interval = tmp_interval * PING_PRECISION;
				if (!is_root && args.interval < PING_MIN_USER_INTERVAL)
					error(EXIT_FAILURE, 0, "option value too small: %s", optarg);
				break;
			case 'w':
				args.timeout = parse_number(optarg, INT_MAX, false);
				break;
			case 'W':
				args.linger = parse_number(optarg, INT_MAX, false);
				break;
			case 's':
				args.size = parse_number(optarg, PING_MAX_DATALEN, true);
				break;
			case 'f':
				args.options |= OPT_FLOOD;
				break;
			case 'l':
				args.preload = strtoul (optarg, &endptr, 0);
				if (*endptr || args.preload > INT_MAX)
					error (EXIT_FAILURE, 0, "invalid preload value (%s)", optarg);
				break;
			case 'T':
				args.tos = parse_number(optarg, 255, true);
				break;

			case '?':
				if (optopt == '?' || optopt == 0) {
					show_help(argv[0]);
					exit(0);
				} else {
					printf("Try '%s --help' or '%s --usage' for more information.\n", argv[0], argv[0]);
					exit(64);
				}
				break;

			case ARG_USAGE:
				show_usage(argv[0]);
				exit(0);
				break;
		}
	}

	args.adresses = malloc(sizeof(char *) * (argc - optind + 1));

	int addr_index = 0;
	while (optind < argc) {
		args.adresses[addr_index] = argv[optind];
		addr_index++;
		optind++;
	}
	args.adresses[addr_index] = NULL;

	if (addr_index == 0) {
		show_missing(argv[0]);
		exit(64);
	}

	return args;
}
