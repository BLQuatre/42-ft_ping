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

static bool setup_socket_options(int ping_sockfd, t_ping_args *args, t_timeval *tv_linger) {
	int ttl_val = 64;

	// Set socket options at IP to TTL and value to 64
	if (setsockopt(ping_sockfd, SOL_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) < 0) {
		if (args->options == OPT_VERBOSE) {
			perror("setsockopt(IP_TTL)");
		}
		return false;
	}

	// Set TOS (Type of Service) if specified
	if (args->tos >= 0) {
		if (setsockopt(ping_sockfd, SOL_IP, IP_TOS, &args->tos, sizeof(args->tos)) < 0) {
			if (args->options == OPT_VERBOSE) {
				perror("setsockopt(IP_TOS)");
			}
			return false;
		}
	}

	// Setting timeout of receive setting
	tv_linger->tv_sec = (args->linger > 0) ? args->linger : RECV_TIMEOUT;
	tv_linger->tv_usec = 0;
	if (setsockopt(ping_sockfd, SOL_SOCKET, SO_RCVTIMEO, tv_linger, sizeof(*tv_linger)) < 0) {
		if (args->options == OPT_VERBOSE) {
			perror("setsockopt(SO_RCVTIMEO)");
			return false;
		}
	}

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

static bool send_icmp_packet(int ping_sockfd, char *packet_buffer, size_t packet_size, t_ping_info *info, t_timespec *time_start, bool verbose) {
	clock_gettime(CLOCK_MONOTONIC, time_start);

	if (sendto(ping_sockfd, packet_buffer, packet_size, 0, (t_sockaddr *)&info->addr_con, sizeof(info->addr_con)) <= 0) {
		if (verbose) {
			printf("sendto error: %s\n", strerror(errno));
		}
		return false;
	}

	return true;
}

static bool receive_ping_reply(int ping_sockfd, t_ping_info *info, t_timespec *time_start, t_timeval *tv_out, t_ping_stats *stats, bool quiet, bool verbose) {
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
			if (verbose) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					printf("Receive timeout occurred\n");
				} else {
					printf("Receive error: %s\n", strerror(errno));
				}
			}
			break;
		}

		t_ip *ip_hdr = (t_ip *)rbuffer;
		t_icmphdr *recv_hdr = (t_icmphdr *)(rbuffer + (ip_hdr->ip_hl * 4));

		if (recv_hdr->type == ICMP_ECHO) {
			if (verbose) {
				printf("Received ICMP echo request (ignoring)\n");
			}
			continue;
		}

		if (recv_hdr->type == ICMP_ECHOREPLY && recv_hdr->code == 0) {
			if (recv_hdr->un.echo.id == getpid()) {
				clock_gettime(CLOCK_MONOTONIC, &time_end);

				double timeElapsed = ((double)(time_end.tv_nsec - time_start->tv_nsec)) / 1000000.0;
				long double rtt_msec = (time_end.tv_sec - time_start->tv_sec) * 1000.0 + timeElapsed;

				int icmp_payload_size = bytes_received - (ip_hdr->ip_hl * 4);
				if (!quiet) {
					printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3Lf ms\n",
						icmp_payload_size, info->ip_addr, recv_hdr->un.echo.sequence, ip_hdr->ip_ttl, rtt_msec);
				}

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
			} else {
				if (verbose) {
					printf("Received ICMP echo reply with wrong ID (expected %d, got %d)\n", getpid(), recv_hdr->un.echo.id);
				}
			}
		} else {
			if (verbose) {
				printf("Received ICMP packet: type=%d, code=%d\n", recv_hdr->type, recv_hdr->code);
				if (recv_hdr->type == ICMP_DEST_UNREACH) {
					printf("Destination unreachable: ");
					switch (recv_hdr->code) {
						case ICMP_NET_UNREACH:
							printf("Network unreachable\n");
							break;
						case ICMP_HOST_UNREACH:
							printf("Host unreachable\n");
							break;
						case ICMP_PROT_UNREACH:
							printf("Protocol unreachable\n");
							break;
						case ICMP_PORT_UNREACH:
							printf("Port unreachable\n");
							break;
						default:
							printf("Code %d\n", recv_hdr->code);
							break;
					}
				} else if (recv_hdr->type == ICMP_TIME_EXCEEDED) {
					printf("Time exceeded: ");
					switch (recv_hdr->code) {
						case ICMP_EXC_TTL:
							printf("TTL exceeded in transit\n");
							break;
						case ICMP_EXC_FRAGTIME:
							printf("Fragment reassembly time exceeded\n");
							break;
						default:
							printf("Code %d\n", recv_hdr->code);
							break;
					}
				}
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
	t_ping_stats stats = {0, 0, 0, 0, 0};
	int msg_count = 0, msg_received_count = 0;
	size_t ping_count = 0;
	size_t packet_size = sizeof(t_icmphdr) + args->size;

	if (args->interval <= 0) {
		error(EXIT_FAILURE, 0, "sending packet: No buffer space available");
	}

	char *packet_buffer = malloc(packet_size);
	if (!packet_buffer) {
		fprintf(stderr, "ft_ping: malloc: Memory allocation failed\n");
		return;
	}

	t_timeval tv_out;
	if (!setup_socket_options(ping_sockfd, args, &tv_out)) {
		free(packet_buffer);
		return;
	}

	t_timespec timeout_end;
	if (args->timeout > 0) {
		clock_gettime(CLOCK_MONOTONIC, &timeout_end);
		timeout_end.tv_sec += args->timeout;
	}

	while (ping_loop && (args->count == 0 || ping_count < args->count)) {
		struct timeval ping_start, ping_end;
		long elapsed_ms;

		create_icmp_packet(packet_buffer, packet_size, msg_count);
		msg_count++;

		t_timespec time_start;
		bool packet_sent = send_icmp_packet(ping_sockfd, packet_buffer, packet_size, info, &time_start, args->options == OPT_VERBOSE);

		gettimeofday(&ping_start, NULL);

		if (args->timeout > 0) {
			t_timespec now;
			clock_gettime(CLOCK_MONOTONIC, &now);
			if (now.tv_sec >= timeout_end.tv_sec) {
				break;
			}
		}

		if (packet_sent) {
			if (receive_ping_reply(ping_sockfd, info, &time_start, &tv_out, &stats, args->options & OPT_QUIET, args->options & OPT_VERBOSE)) {
				msg_received_count++;
			}
		} else {
			if (args->options == OPT_VERBOSE) {
				fprintf(stderr, "Failed to send ICMP packet for sequence %d\n", msg_count - 1);
			}
		}

		gettimeofday(&ping_end, NULL);
		elapsed_ms = (ping_end.tv_sec - ping_start.tv_sec) * 1000;
		elapsed_ms += (ping_end.tv_usec - ping_start.tv_usec) / 1000;

		ping_count++;
		if (args->count == 0 || ping_count < args->count) {
			int remaining_time = args->interval - elapsed_ms;
			if (remaining_time > 0) {
				usleep(remaining_time * 1000);
			}
		}
	}

	print_statistics(info, msg_count, msg_received_count, &stats);
	free(packet_buffer);
}
