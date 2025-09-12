#include "ft_ping.h"

bool is_valid_ipv4(const char *ip) {
	t_sockaddr_in sa;
	return inet_pton(AF_INET, ip, &(sa.sin_addr)) == 1;
}

char *resolve_hostname_to_ip(char *addr_host, t_sockaddr_in *addr_con) {
	t_hostent *host_entity;
	char *ip = (char *) malloc(NI_MAXHOST * sizeof(char));

	if ((host_entity = gethostbyname(addr_host)) == NULL) {
		free(ip);
		return NULL;
	}

	strcpy(ip, inet_ntoa(*(t_in_addr *)host_entity->h_addr_list[0]));
	(*addr_con).sin_family = host_entity->h_addrtype;
	(*addr_con).sin_port = htons(PORT_NO);
	(*addr_con).sin_addr.s_addr = *(long *)host_entity->h_addr_list[0];

	return ip;
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
