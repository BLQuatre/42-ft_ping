#include "ft_ping.h"

t_ping_info parse_ping_info(char *target, char *program_name) {
	t_ping_info info;
	bool is_ip_input;

	is_ip_input = is_valid_ipv4(target);

	if (is_ip_input) {
		info.ip_addr = malloc(strlen(target) + 1);
		strcpy(info.ip_addr, target);
		info.hostname = malloc(strlen(target) + 1);
		strcpy(info.hostname, target);

		info.addr_con.sin_family = AF_INET;
		info.addr_con.sin_port = htons(PORT_NO);
		info.addr_con.sin_addr.s_addr = inet_addr(target);
	} else {
		info.ip_addr = resolve_hostname_to_ip(target, &info.addr_con);
		if (info.ip_addr == NULL) {
			show_error(1, "%s: unknown host\n", program_name);
		}

		info.hostname = malloc(strlen(target) + 1);
		strcpy(info.hostname, target);
	}

	return info;
}



// void send_ping(int ping_sockfd, t_sockaddr_in *ping_addr, char *ping_dom, char *ping_ip, int verbose) {
void send_ping(int ping_sockfd, t_ping_info *info, t_ping_args *args) {
	int ttl_val = 64, msg_count = 0, flag = 1, msg_received_count = 0;
	size_t i;
	socklen_t addr_len;
	char rbuffer[128];
	t_ping_pkt pckt;
	t_sockaddr_in r_addr;
	t_timespec time_start, time_end, tfs, tfe;
	long double rtt_msec = 0, total_msec = 0;
	t_timeval tv_out;
	tv_out.tv_sec = RECV_TIMEOUT;
	tv_out.tv_usec = 0;

	clock_gettime(CLOCK_MONOTONIC, &tfs);

	// Set socket options at IP to TTL and value to 64
	if (setsockopt(ping_sockfd, SOL_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) != 0) {
		printf("ft_ping: setsockopt: TTL failed\n");
		if (args->options & OPT_VERBOSE) {
			perror("ft_ping: setsockopt TTL");
		}
		return;
	}

	// Setting timeout of receive setting
	if (setsockopt(ping_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_out, sizeof tv_out) != 0) {
		if (args->options & OPT_VERBOSE) {
			printf("ft_ping: warning: setsockopt SO_RCVTIMEO failed\n");
			perror("ft_ping: setsockopt SO_RCVTIMEO");
		}
	}

	if (args->options & OPT_VERBOSE) {
		printf("ft_ping: socket configured, TTL=%d, timeout=%ds\n", ttl_val, RECV_TIMEOUT);
	}

	// Send ICMP packet in an infinite loop
	while (ping_loop) {
		// Flag to check if packet was sent or not
		flag = 1;

		// Fill the packet
		bzero(&pckt, sizeof(pckt));
		pckt.hdr.type = ICMP_ECHO;
		pckt.hdr.un.echo.id = getpid();

		for (i = 0; i < sizeof(pckt.msg) - 1; i++)
			pckt.msg[i] = (char)(i + '0');

		pckt.msg[i] = 0;
		pckt.hdr.un.echo.sequence = msg_count++;
		pckt.hdr.checksum = 0; // Reset checksum before calculation
		pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));

		if (args->options & OPT_VERBOSE && msg_count == 1) {
			printf("ft_ping: sending ICMP packets with ID=%d\n", getpid());
		}

		usleep(PING_SLEEP_RATE);

		// Send packet
		clock_gettime(CLOCK_MONOTONIC, &time_start);
		if (sendto(ping_sockfd, &pckt, sizeof(pckt), 0, (t_sockaddr *)&info->addr_con, sizeof(info->addr_con)) <= 0) {
			printf("ft_ping: sendto: Packet sending failed\n");
			if (args->options & OPT_VERBOSE) {
				perror("ft_ping: sendto");
			}
			flag = 0;
		}

		// Receive packet
		addr_len = sizeof(r_addr);
		ssize_t bytes_received = recvfrom(ping_sockfd, rbuffer, sizeof(rbuffer), 0, (t_sockaddr *)&r_addr, &addr_len);

		if (bytes_received <= 0) {
			if (msg_count > 1) {
				if (args->options & OPT_VERBOSE) {
					printf("ft_ping: recvfrom: timeout or error receiving packet\n");
					perror("ft_ping: recvfrom");
				}
				printf("Request timeout for icmp_seq %d\n", msg_count - 1);
			}
		} else {
			clock_gettime(CLOCK_MONOTONIC, &time_end);

			double timeElapsed = ((double)(time_end.tv_nsec - time_start.tv_nsec)) / 1000000.0;
			rtt_msec = (time_end.tv_sec - time_start.tv_sec) * 1000.0 + timeElapsed;

			if (args->options & OPT_VERBOSE) {
				printf("ft_ping: received %zd bytes from %s\n", bytes_received, inet_ntoa(r_addr.sin_addr));
			}

			// If packet was not sent, don't receive
			if (flag) {
				// Skip IP header to get to ICMP header
				t_ip *ip_hdr = (t_ip *)rbuffer;
				t_icmphdr *recv_hdr = (t_icmphdr *)(rbuffer + (ip_hdr->ip_hl * 4));

				if (args->options & OPT_VERBOSE) {
					printf("ft_ping: ICMP type=%d, code=%d, id=%d, seq=%d\n",
						recv_hdr->type, recv_hdr->code, recv_hdr->un.echo.id, recv_hdr->un.echo.sequence);
				}

				if (!(recv_hdr->type == ICMP_ECHOREPLY && recv_hdr->code == 0)) {
					if (args->options & OPT_VERBOSE || recv_hdr->type != ICMP_ECHOREPLY) {
						printf("ft_ping: ICMP %d/%d from %s\n", recv_hdr->type, recv_hdr->code, inet_ntoa(r_addr.sin_addr));
					}
				} else {
					printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3Lf ms\n",
						(int)bytes_received, info->hostname, recv_hdr->un.echo.sequence, ip_hdr->ip_ttl, rtt_msec);
					msg_received_count++;
				}
			}
		}
	}
	clock_gettime(CLOCK_MONOTONIC, &tfe);
	double timeElapsed = ((double)(tfe.tv_nsec - tfs.tv_nsec)) / 1000000.0;
	total_msec = (tfe.tv_sec - tfs.tv_sec) * 1000.0 + timeElapsed;

	printf("\n--- %s ping statistics ---\n", info->hostname);
	printf("%d packets transmitted, %d received, %.0f%% packet loss, time %.0Lfms\n",
		msg_count, msg_received_count, ((msg_count - msg_received_count) / (double)msg_count) * 100.0, total_msec);

	if (args->options & OPT_VERBOSE) {
		printf("ft_ping: statistics - transmitted=%d, received=%d, loss=%.1f%%, time=%.0Lfms\n",
			msg_count, msg_received_count, ((msg_count - msg_received_count) / (double)msg_count) * 100.0, total_msec);
	}
}
