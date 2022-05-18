#include "context.h"
#include "ftp_errors.h"
#include "ftp_methods.h"
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <stdio.h>

void	create_socket(t_ftp_ctx *ctx) {
	ctx->sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (ctx->sock == -1) {
		// TODO exit with proper error: socket creation failed
		perror("socket");
		exit(1);
	}
}

void	populate_context(t_ftp_ctx *ctx) {
	// size of icmp packet header + the size of payload + the size of ip header
	// (ip header is needed for receiving packets, it includes the ip header)
	ctx->packet_size = sizeof(struct icmphdr) + ctx->payload_size + IP_HDR_MAX;
	ctx->packet_buffer = malloc(ctx->packet_size);
	if (ctx->packet_buffer == NULL) {
		// TODO exit with proper error: ran out of memory
		perror("malloc");
		exit(1);
	}
	ctx->payload_ptr = ctx->packet_buffer + sizeof(struct icmphdr);

	ctx->id = getpid();
	ctx->addr.sin_addr = ctx->ip;
	ctx->addr.sin_port = 0; // TODO parse args
	ctx->addr.sin_family = AF_INET;
}

void	setup_handlers() {
	signal(SIGINT, handle_interupt);
	signal(SIGALRM, handle_alarm);
}

void	free_context(t_ftp_ctx *ctx) {
	close(ctx->sock);
	free(ctx->packet_buffer);
}

void	ensure_root() {
	uid_t uid = getuid();
	if (uid != 0) {
		// TODO proper error: need to run as root
		perror("root");
		exit(1);
	}
}
