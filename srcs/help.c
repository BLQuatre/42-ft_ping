#include "ft_ping.h"

void show_missing(char *program_name) {
	printf("%s: missing host operand\n", program_name);
	printf("Try '%s --help' or '%s --usage' for more information.\n", program_name, program_name);
}

void show_help(char *program_name) {
	printf("Usage: %s [OPTION...] HOST\n", program_name);
	printf("Send ICMP ECHO_REQUEST packets to network hosts.\n");
	printf("\n");
	printf(" Options controlling ICMP request types:\n");
	printf("      --echo                 send ICMP_ECHO packets (default)\n");
	printf("\n");
	printf(" Options valid for all request types:\n");
	printf("\n");
	printf("  -c, --count=NUMBER         stop after sending NUMBER packets\n");
	printf("  -i, --interval=NUMBER      wait NUMBER seconds between sending each packet\n");
	printf("  -T, --tos=NUM              set type of service (TOS); to NUM\n");
	printf("  -v, --verbose              verbose output\n");
	printf("  -w, --timeout=N            stop after N seconds\n");
	printf("  -W, --linger=N             number of seconds to wait for response\n");
	printf("\n");
	printf(" Options valid for --echo requests:\n");
	printf("\n");
	printf("  -f, --flood                flood ping (root only);\n");
	printf("  -l, --preload=NUMBER       send NUMBER packets as fast as possible before\n");
	printf("                             falling into normal mode of behavior (root only);\n");
	printf("  -q, --quiet                quiet output\n");
	printf("  -s, --size=NUMBER          send NUMBER data octets\n");
	printf("\n");
	printf("  -?, --help                 give this help list\n");
	printf("\n");
	printf("Mandatory or optional arguments to long options are also mandatory or optional\n");
	printf("for any corresponding short options.\n");
	printf("\n");
	printf("Options marked with (root only); are available only to superuser.\n");
	printf("\n");
	printf("Report bugs to the maintaner\n");
}

void show_usage(char *program_name) {
	printf("Usage: %s [-vfq?] [-c NUMBER] [-i NUMBER] [-T NUM]\n", program_name);
	printf("               [-w N] [-W N] [-l NUMBER] [-s NUMBER] [--echo]\n");
	printf("               [--count=NUMBER] [--interval=NUMBER] [--tos=NUM]\n");
	printf("               [--verbose] [--timeout=N] [--linger=N] [--flood]\n");
	printf("               [--preload=NUMBER] [--quiet] [--size=NUMBER]\n");
	printf("               [--help] [--usage] HOST ...\n");
}
