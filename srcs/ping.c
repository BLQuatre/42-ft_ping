#include "ft_ping.h"

static unsigned short checksum(void *b, int len) {
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

static bool setup_socket_options(int ping_sockfd, t_ping_args *args, t_timeval *tv_out) {
	int ttl_val = 64;

	// Use linger timeout if specified, otherwise default
	tv_out->tv_sec = (args->linger > 0) ? args->linger : RECV_TIMEOUT;
	tv_out->tv_usec = 0;

	// Set socket options at IP to TTL and value to 64
	if (setsockopt(ping_sockfd, SOL_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) != 0) {
		printf("ft_ping: setsockopt: TTL failed\n");
		return false;
	}

	// Set TOS (Type of Service) if specified
	if (args->tos >= 0) {
		setsockopt(ping_sockfd, SOL_IP, IP_TOS, &args->tos, sizeof(args->tos));
	}

	// Setting timeout of receive setting
	setsockopt(ping_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)tv_out, sizeof(*tv_out));

	return true;
}

static void create_icmp_packet(char *packet_buffer, size_t packet_size, int msg_count) {
	size_t i;
	t_ping_pkt *pckt = (t_ping_pkt *) packet_buffer;
	size_t data_size = packet_size - sizeof(t_icmphdr);

	bzero(packet_buffer, packet_size);
	pckt->hdr.type = ICMP_ECHO;
	pckt->hdr.un.echo.id = getpid();

	for (i = 0; i < data_size; i++)
		pckt->msg[i] = (char)((i % 95) + 32);

	pckt->hdr.un.echo.sequence = msg_count;
	pckt->hdr.checksum = 0;
	pckt->hdr.checksum = checksum(pckt, packet_size);
}

static bool send_icmp_packet(int ping_sockfd, char *packet_buffer, size_t packet_size, t_ping_info *info, t_timespec *time_start) {
	clock_gettime(CLOCK_MONOTONIC, time_start);

	if (sendto(ping_sockfd, packet_buffer, packet_size, 0, (t_sockaddr *)&info->addr_con, sizeof(info->addr_con)) <= 0) {
		printf("ft_ping: sendto: Packet sending failed\n");
		return false;
	}
	return true;
}

static bool receive_ping_reply(int ping_sockfd, t_ping_info *info, t_timespec *time_start, t_timeval *tv_out, t_ping_stats *stats) {
	char rbuffer[USHRT_MAX];
	t_sockaddr_in r_addr;
	socklen_t addr_len;
	bool got_reply = false;
	time_t start_recv_time = time(NULL);
	t_timespec time_end;

	while (!got_reply && (time(NULL) - start_recv_time) < tv_out->tv_sec) {
		addr_len = sizeof(r_addr);
		ssize_t bytes_received = recvfrom(ping_sockfd, rbuffer, sizeof(rbuffer), 0, (t_sockaddr *)&r_addr, &addr_len);

		if (bytes_received <= 0) {
			break;
		}

		t_ip *ip_hdr = (t_ip *)rbuffer;
		t_icmphdr *recv_hdr = (t_icmphdr *)(rbuffer + (ip_hdr->ip_hl * 4));

		if (recv_hdr->type == ICMP_ECHO) {
			continue;
		}

		if (recv_hdr->type == ICMP_ECHOREPLY && recv_hdr->code == 0) {
			if (recv_hdr->un.echo.id == getpid()) {
				clock_gettime(CLOCK_MONOTONIC, &time_end);

				double timeElapsed = ((double)(time_end.tv_nsec - time_start->tv_nsec)) / 1000000.0;
				long double rtt_msec = (time_end.tv_sec - time_start->tv_sec) * 1000.0 + timeElapsed;

				int icmp_payload_size = bytes_received - (ip_hdr->ip_hl * 4);
				printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3Lf ms\n",
					icmp_payload_size, info->ip_addr, recv_hdr->un.echo.sequence, ip_hdr->ip_ttl, rtt_msec);

				stats->rtt_count++;
				stats->rtt_sum += rtt_msec;
				stats->rtt_sum_sq += rtt_msec * rtt_msec;

				if (stats->rtt_count == 1) {
					stats->rtt_min = stats->rtt_max = rtt_msec;
				} else {
					if (rtt_msec < stats->rtt_min) stats->rtt_min = rtt_msec;
					if (rtt_msec > stats->rtt_max) stats->rtt_max = rtt_msec;
				}
				got_reply = true;
			}
		}
	}

	return got_reply;
}

static void print_statistics(t_ping_info *info, int msg_count, int msg_received_count, t_ping_stats *stats) {
	printf("--- %s ping statistics ---\n", info->hostname);
	printf("%d packets transmitted, %d packets received, %.0f%% packet loss\n",
		msg_count, msg_received_count, ((msg_count - msg_received_count) / (double)msg_count) * 100.0);

	if (stats->rtt_count > 0) {
		long double rtt_avg = stats->rtt_sum / stats->rtt_count;
		long double rtt_stddev = 0;

		if (stats->rtt_count > 1) {
			// Formula: sqrt((sum_sq - (sum^2)/n) / (n-1))
			long double variance = (stats->rtt_sum_sq - (stats->rtt_sum * stats->rtt_sum) / stats->rtt_count) / (stats->rtt_count - 1);
			rtt_stddev = sqrtl(variance);
		}

		printf("round-trip min/avg/max/stddev = %.3Lf/%.3Lf/%.3Lf/%.3Lf ms\n",
			stats->rtt_min, rtt_avg, stats->rtt_max, rtt_stddev);
	}
}

void send_ping(int ping_sockfd, t_ping_info *info, t_ping_args *args) {
	int msg_count = 0, msg_received_count = 0;
	size_t ping_count = 0;
	t_timespec time_start, tfs, tfe, timeout_end;
	t_timeval tv_out;
	t_ping_stats stats = {0, 0, 0, 0, 0};

	size_t data_size = (args->size > 0) ? args->size : 56;
	size_t packet_size = sizeof(t_icmphdr) + data_size;

	char *packet_buffer = malloc(packet_size);
	if (!packet_buffer) {
		printf("ft_ping: malloc: Memory allocation failed\n");
		return;
	}

	if (!setup_socket_options(ping_sockfd, args, &tv_out)) {
		free(packet_buffer);
		return;
	}

	clock_gettime(CLOCK_MONOTONIC, &tfs);

	if (args->timeout > 0) {
		timeout_end = tfs;
		timeout_end.tv_sec += args->timeout;
	}

	// Send ICMP packet in an infinite loop
	while (ping_loop && (args->count == 0 || ping_count < args->count)) {
		if (args->timeout > 0) {
			t_timespec current_time;
			clock_gettime(CLOCK_MONOTONIC, &current_time);
			if (current_time.tv_sec >= timeout_end.tv_sec) {
				break;
			}
		}

		ping_count++;

		create_icmp_packet(packet_buffer, packet_size, msg_count);
		msg_count++;

		bool packet_sent = send_icmp_packet(ping_sockfd, packet_buffer, packet_size, info, &time_start);

		bool got_reply = false;
		if (packet_sent) {
			got_reply = receive_ping_reply(ping_sockfd, info, &time_start, &tv_out, &stats);
			if (got_reply) {
				msg_received_count++;
			}
		}

		// if (!got_reply && packet_sent) {
		// 	printf("Request timeout for icmp_seq %d\n", msg_count - 1);
		// }

		if (args->count == 0 || ping_count < args->count) {
			usleep(PING_PRECISION * args->interval);
		}
	}
	clock_gettime(CLOCK_MONOTONIC, &tfe);

	print_statistics(info, msg_count, msg_received_count, &stats);

	free(packet_buffer);
}
