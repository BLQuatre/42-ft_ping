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

void send_ping(int ping_sockfd, t_ping_info *info, t_ping_args *args) {
	int ttl_val = 64, msg_count = 0, flag = 1, msg_received_count = 0;
	size_t i, ping_count = 0;
	socklen_t addr_len;
	char rbuffer[128];
	t_ping_pkt pckt;
	t_sockaddr_in r_addr;
	t_timespec time_start, time_end, tfs, tfe, timeout_end;
	long double rtt_msec = 0;
	t_timeval tv_out;

	long double rtt_min = 0, rtt_max = 0, rtt_sum = 0, rtt_sum_sq = 0;
	int rtt_count = 0;

	// Use linger timeout if specified, otherwise default
	tv_out.tv_sec = (args->linger > 0) ? args->linger : RECV_TIMEOUT;
	tv_out.tv_usec = 0;

	clock_gettime(CLOCK_MONOTONIC, &tfs);

	// Set timeout end time if timeout is specified
	if (args->timeout > 0) {
		timeout_end = tfs;
		timeout_end.tv_sec += args->timeout;
	}

	// Set socket options at IP to TTL and value to 64
	if (setsockopt(ping_sockfd, SOL_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) != 0) {
		printf("ft_ping: setsockopt: TTL failed\n");
		if (args->options & OPT_VERBOSE) {
			perror("ft_ping: setsockopt TTL");
		}
		return;
	}

	// Set TOS (Type of Service) if specified
	if (args->tos >= 0) {
		if (setsockopt(ping_sockfd, SOL_IP, IP_TOS, &args->tos, sizeof(args->tos)) != 0) {
			if (args->options & OPT_VERBOSE) {
				printf("ft_ping: warning: setsockopt IP_TOS failed\n");
				perror("ft_ping: setsockopt IP_TOS");
			}
		} else if (args->options & OPT_VERBOSE) {
			printf("ft_ping: TOS set to %d\n", args->tos);
		}
	}

	// Setting timeout of receive setting
	if (setsockopt(ping_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_out, sizeof tv_out) != 0) {
		if (args->options & OPT_VERBOSE) {
			printf("ft_ping: warning: setsockopt SO_RCVTIMEO failed\n");
			perror("ft_ping: setsockopt SO_RCVTIMEO");
		}
	}

	if (args->options & OPT_VERBOSE) {
		printf("ft_ping: socket configured, TTL=%d, timeout=%ds", ttl_val, (int)tv_out.tv_sec);
		if (args->tos >= 0) {
			printf(", TOS=%d", args->tos);
		}
		printf("\n");
	}

	// Send ICMP packet in an infinite loop
	while (ping_loop && (args->count == 0 || ping_count < args->count)) {
		if (args->timeout > 0) {
			t_timespec current_time;
			clock_gettime(CLOCK_MONOTONIC, &current_time);
			if (current_time.tv_sec >= timeout_end.tv_sec) {
				if (args->options & OPT_VERBOSE) {
					printf("ft_ping: timeout reached after %d seconds\n", args->timeout);
				}
				break;
			}
		}

		flag = true;
		ping_count++;

		bzero(&pckt, sizeof(pckt));
		pckt.hdr.type = ICMP_ECHO;
		pckt.hdr.un.echo.id = getpid();

		// Fill data payload up to specified size (limited by packet structure)
		size_t data_size = (args->size < sizeof(pckt.msg)) ? args->size : sizeof(pckt.msg) - 1;
		for (i = 0; i < data_size; i++)
			pckt.msg[i] = (char)(i + '0');

		pckt.msg[data_size] = 0;
		pckt.hdr.un.echo.sequence = msg_count++;
		pckt.hdr.checksum = 0;
		pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));

		if (args->options & OPT_VERBOSE && msg_count == 1) {
			printf("ft_ping: sending ICMP packets with ID=%d, data size=%zu bytes\n", getpid(), args->size);
		}

		usleep(PING_PRECISION * args->interval);

		clock_gettime(CLOCK_MONOTONIC, &time_start);
		size_t packet_size = sizeof(t_icmphdr) + ((args->size < sizeof(pckt.msg)) ? args->size : sizeof(pckt.msg));
		if (sendto(ping_sockfd, &pckt, packet_size, 0, (t_sockaddr *)&info->addr_con, sizeof(info->addr_con)) <= 0) {
			printf("ft_ping: sendto: Packet sending failed\n");
			if (args->options & OPT_VERBOSE) {
				perror("ft_ping: sendto");
			}
			flag = false;
		}

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

					rtt_count++;
					rtt_sum += rtt_msec;
					rtt_sum_sq += rtt_msec * rtt_msec;

					if (rtt_count == 1) {
						rtt_min = rtt_max = rtt_msec;
					} else {
						if (rtt_msec < rtt_min) rtt_min = rtt_msec;
						if (rtt_msec > rtt_max) rtt_max = rtt_msec;
					}
				}
			}
		}
	}
	clock_gettime(CLOCK_MONOTONIC, &tfe);

	printf("--- %s ping statistics ---\n", info->hostname);
	printf("%d packets transmitted, %d packets received, %.0f%% packet loss\n",
		msg_count, msg_received_count, ((msg_count - msg_received_count) / (double)msg_count) * 100.0);

	if (rtt_count > 0) {
		long double rtt_avg = rtt_sum / rtt_count;
		long double rtt_stddev = 0;

		if (rtt_count > 1) {
			// Formula: sqrt((sum_sq - (sum^2)/n) / (n-1))
			long double variance = (rtt_sum_sq - (rtt_sum * rtt_sum) / rtt_count) / (rtt_count - 1);
			rtt_stddev = sqrtl(variance);
		}

		printf("round-trip min/avg/max/stddev = %.3Lf/%.3Lf/%.3Lf/%.3Lf ms\n",
			rtt_min, rtt_avg, rtt_max, rtt_stddev);
	}
}
