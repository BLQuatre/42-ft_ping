#include "ft_ping.h"

void show_try(char *program_name) {
	fprintf(stderr, "Try '%s --help' or '%s --usage' for more information.\n", program_name, program_name);
}

void show_help(char *program_name) {
	printf("Usage: %s [OPTION...] HOST ...\n", program_name);
	printf("Send ICMP ECHO_REQUEST packets to network hosts.\n");
	printf("\n");
	printf(" Options controlling ICMP request types:\n");
	printf("      --echo                 send ICMP_ECHO packets (default)\n");
	printf("\n");
	printf(" Options valid for all request types:\n");
	printf("\n");
	printf("  -c, --count=NUMBER         stop after sending NUMBER packets\n");
	printf("  -i, --interval=NUMBER      wait NUMBER seconds between sending each packet\n");
	printf("  -T, --tos=NUM              set type of service (TOS) to NUM\n");
	printf("  -v, --verbose              verbose output\n");
	printf("  -w, --timeout=N            stop after N seconds\n");
	printf("  -W, --linger=N             number of seconds to wait for response\n");
	printf("\n");
	printf(" Options valid for --echo requests:\n");
	printf("\n");
	printf("  -q, --quiet                quiet output\n");
	printf("  -s, --size=NUMBER          send NUMBER data octets\n");
	printf("\n");
	printf("  -?, --help                 give this help list\n");
	printf("\n");
	printf("Mandatory or optional arguments to long options are also mandatory or optional\n");
	printf("for any corresponding short options.\n");
	printf("\n");
	printf("Report bugs to the maintainer.\n");
}

void show_usage(char *program_name) {
	printf("Usage: %s [-vq?] [-c NUMBER] [-i NUMBER] [-T NUM] [-w N] [-W N] [-s NUMBER] \n", program_name);
	printf("               [--echo] [--count=NUMBER] [--interval=NUMBER] [--tos=NUM] \n");
	printf("               [--verbose] [--timeout=N] [--linger=N] [--quiet] \n");
	printf("               [--size=NUMBER] [--help] [--usage]\n");
	printf("               HOST ...\n");
}
