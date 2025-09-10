#include "ft_ping.h"

unsigned short checksum(void *b, int len) {
	unsigned short *buf = b;
	unsigned int sum = 0;
	unsigned short result;

	for (sum = 0; len > 1; len -= 2)
		sum += *buf++;
	if (len == 1)
		sum += *(unsigned char *)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}

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
